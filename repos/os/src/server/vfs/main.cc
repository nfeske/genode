/*
 * \brief  VFS File_system server
 * \author Emery Hemingway
 * \author Christian Helmuth
 * \author Norman Feske
 * \date   2015-08-16
 */

/*
 * Copyright (C) 2015-2019 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/component.h>
#include <base/registry.h>
#include <base/heap.h>
#include <base/attached_ram_dataspace.h>
#include <base/attached_rom_dataspace.h>
#include <file_system_session/rpc_object.h>
#include <root/component.h>
#include <os/session_policy.h>
#include <base/allocator_guard.h>
#include <util/fifo.h>
#include <vfs/simple_env.h>

/* Local includes */
#include "assert.h"
#include "node.h"


namespace Vfs_server {
	using namespace File_system;
	using namespace Vfs;

	class Session_resources;
	class Session_component;
	class Vfs_env;
	class Root;

	typedef Genode::Fifo<Session_component> Session_queue;

	/**
	 * Convenience utities for parsing quotas
	 */
	Genode::Ram_quota parse_ram_quota(char const *args) {
		return Genode::Ram_quota{
			Genode::Arg_string::find_arg(args, "ram_quota").ulong_value(0)}; }
	Genode::Cap_quota parse_cap_quota(char const *args) {
		return Genode::Cap_quota{
			Genode::Arg_string::find_arg(args, "cap_quota").ulong_value(0)}; }
};


/**
 * Base class to manage session quotas and allocations
 */
class Vfs_server::Session_resources
{
	protected:

		Genode::Ram_quota_guard           _ram_guard;
		Genode::Cap_quota_guard           _cap_guard;
		Genode::Constrained_ram_allocator _ram_alloc;
		Genode::Attached_ram_dataspace    _packet_ds;
		Genode::Heap                      _alloc;

		Session_resources(Genode::Ram_allocator &ram,
		                  Genode::Region_map    &region_map,
		                  Genode::Ram_quota     ram_quota,
		                  Genode::Cap_quota     cap_quota,
		                  Genode::size_t        buffer_size)
		:
			_ram_guard(ram_quota), _cap_guard(cap_quota),
			_ram_alloc(ram, _ram_guard, _cap_guard),
			_packet_ds(_ram_alloc, region_map, buffer_size),
			_alloc(_ram_alloc, region_map)
		{ }
};


class Vfs_server::Session_component : private Session_resources,
                                      public ::File_system::Session_rpc_object,
                                      private Session_queue::Element
{
	friend Session_queue;

	private:

		Vfs::File_system &_vfs;

		Genode::Entrypoint &_ep;

		Packet_stream &_stream { *tx_sink() };

		/* nodes of this session with active jobs */
		Node_queue _active_nodes { };

		/* nodes with finished jobs that can be acknowledged */
		Node_queue _finished_nodes { };

		/* global queue of sessions with active jobs */
		Session_queue &_active_sessions;

		/* collection of open nodes local to this session */
		Node_space _node_space { };

		Genode::Signal_handler<Session_component> _packet_stream_handler {
			_ep, *this, &Session_component::_handle_packet_stream };

		/*
		 * The root node needs be allocated with the session struct
		 * but removeable from the id space at session destruction.
		 */
		Path const _root_path;

		Genode::Session_label const _label;

		bool const _writable;


		/****************************
		 ** Handle to node mapping **
		 ****************************/

		/**
		 * Apply functor to node
		 *
		 * \throw Invalid_handle
		 */
		template <typename FUNC>
		void _apply_node(Node_handle handle, FUNC const &fn)
		{
			Node_space::Id id { handle.value };

			try { return _node_space.apply<Node>(id, fn); }
			catch (Node_space::Unknown_id) { throw Invalid_handle(); }
		}

		/**
		 * Apply functor to typed node
		 *
		 * \throw Invalid_handle
		 */
		template <typename HANDLE_TYPE, typename FUNC>
		auto _apply(HANDLE_TYPE handle, FUNC const &fn)
		-> typename Genode::Trait::Functor<decltype(&FUNC::operator())>::Return_type
		{
			Node_space::Id id { handle.value };

			try { return _node_space.apply<Node>(id, [&] (Node &node) {
				typedef typename Node_type<HANDLE_TYPE>::Type Typed_node;
				Typed_node *n = dynamic_cast<Typed_node *>(&node);
				if (!n)
					throw Invalid_handle();
				return fn(*n);
			}); } catch (Node_space::Unknown_id) { throw Invalid_handle(); }
		}


		/******************************
		 ** Packet-stream processing **
		 ******************************/

		bool _try_import_jobs_from_submit_queue()
		{
			bool overall_progress = false;

			for (;;) {

				bool progress_in_iteration = false;

				if (!_stream.packet_avail())
					break;

				/* ensure that ack for one malformed packet can be returned */
				if (!_stream.ready_to_ack())
					break;

				Packet_descriptor packet = _stream.peek_packet();

				auto drop_packet_from_submit_queue = [&] ()
				{
					_stream.get_packet();

					overall_progress      = true;
					progress_in_iteration = true;
				};

				auto ack_invalid_packet = [&] ()
				{
					drop_packet_from_submit_queue();
					packet.succeeded(false);
					_stream.acknowledge_packet(packet);

					overall_progress      = true;
					progress_in_iteration = true;
				};

				/* test for invalid packet */
				if (packet.length() > packet.size()) {
					ack_invalid_packet();
					continue;
				}

				try {
					_apply(packet.handle(), [&] (Io_node &node) {

						/* each node can deal with only one job at a time */
						bool const job_acceptable = !node.enqueued();

						if (!job_acceptable)
							return;

						Payload_ptr payload_ptr { _stream.packet_content(packet) };

						switch (node.submit_job(packet, payload_ptr)) {

						case Node::Submit_result::ACCEPTED:
							_active_nodes.enqueue(node);
							drop_packet_from_submit_queue();
							break;

						case Node::Submit_result::DENIED:
							drop_packet_from_submit_queue();
							ack_invalid_packet();
							break;

						case Node::Submit_result::STALLED:
							/* keep request packet in submit queue */
							break;
						}
					});
				}
				catch (File_system::Invalid_handle) { ack_invalid_packet(); }

				if (!progress_in_iteration)
					break;
			}

			return overall_progress;
		}

		bool _try_execute_jobs()
		{
			bool progress = false;

			/* nodes with jobs that cannot make progress right now */
			Node_queue still_active_nodes { };

			_active_nodes.dequeue_all([&] (Node &node) {
				node.execute_job();
				if (node.acknowledgement_valid()) {
					_finished_nodes.enqueue(node);
					progress = true;
				} else {
					still_active_nodes.enqueue(node);
				}
			});

			_active_nodes = still_active_nodes;

			return progress;
		}

		bool _try_acknowledge_jobs()
		{
			bool progress = false;

			for (;;) {
				if (!_stream.ready_to_ack())
					break;

				if (_finished_nodes.empty())
					break;

				_finished_nodes.dequeue([&] (Node &node) {
					_stream.acknowledge_packet(node.acknowledgement_packet());
					progress = true;
				});
			}

			return progress;
		}

	protected:

		friend Vfs_server::Root;

		/**
		 * Called by the global Io_progress_handler as well as the
		 * session-local packet-stream handler
		 */
		void process_packets()
		{
			/**
			 * Process packets in batches, otherwise a client that
			 * submits packets as fast as they are processed will
			 * starve the signal handler.
			 */
			unsigned quantum = TX_QUEUE_SIZE;

			for (;;) {

				if (--quantum == 0) {
					/*
					 * Trigger re-execution of 'process_packets' for the
					 * remaining work of this session.
					 */
					Genode::Signal_transmitter(_packet_stream_handler).submit();
					break;
				}

				/* true if progress can be made in this iteration */
				bool progress = false;

				progress |= _try_import_jobs_from_submit_queue();
				progress |= _try_execute_jobs();
				progress |= _try_acknowledge_jobs();

				if (!progress)
					break;
			}
		}

		bool no_longer_active() const
		{
			bool const no_job = _active_nodes.empty()
			                 && _finished_nodes.empty();

			return Session_queue::Element::enqueued() && no_job;
		}

		bool no_longer_idle() const
		{
			bool const any_job = !_active_nodes.empty()
			                  || !_finished_nodes.empty();

			return !Session_queue::Element::enqueued() && any_job;
		}

	private:

		/**
		 * Signal handler called for session-local packet-stream signals
		 */
		void _handle_packet_stream()
		{
			process_packets();

			if (no_longer_active())
				_active_sessions.remove(*this);

			if (no_longer_idle())
				_active_sessions.enqueue(*this);
		}

		/**
		 * Check if string represents a valid path (must start with '/')
		 */
		static void _assert_valid_path(char const *path) {
			if (!path || path[0] != '/') throw Lookup_failed(); }

		/**
		 * Check if string represents a valid name (must not contain '/')
		 */
		static void _assert_valid_name(char const *name)
		{
			if (!*name) throw Invalid_name();
			for (char const *p = name; *p; ++p)
				if (*p == '/')
					throw Invalid_name();
		}

		/**
		 * Destroy an open node
		 */
		void _close(Node &node)
		{
			if (File *file = dynamic_cast<File*>(&node))
				destroy(_alloc, file);
			else if (Directory *dir = dynamic_cast<Directory*>(&node))
				destroy(_alloc, dir);
			else if (Symlink *link = dynamic_cast<Symlink*>(&node))
				destroy(_alloc, link);
			else if (Watch_node *watch = dynamic_cast<Watch_node*>(&node))
				destroy(_alloc, watch);
			else
				destroy(_alloc, &node);
		}

	public:

		/**
		 * Constructor
		 */
		Session_component(Genode::Env          &env,
		                  char           const *label,
		                  Genode::Ram_quota     ram_quota,
		                  Genode::Cap_quota     cap_quota,
		                  size_t                tx_buf_size,
		                  Vfs::File_system     &vfs,
		                  Session_queue        &active_sessions,
		                  char           const *root_path,
		                  bool                  writable)
		:
			Session_resources(env.pd(), env.rm(), ram_quota, cap_quota, tx_buf_size),
			Session_rpc_object(_packet_ds.cap(), env.rm(), env.ep().rpc_ep()),
			_vfs(vfs),
			_ep(env.ep()),
			_active_sessions(active_sessions),
			_root_path(root_path),
			_label(label),
			_writable(writable)
		{
			/*
			 * Register an I/O signal handler for
			 * packet-avail and ready-to-ack signals.
			 */
			_tx.sigh_packet_avail(_packet_stream_handler);
			_tx.sigh_ready_to_ack(_packet_stream_handler);
		}

		/**
		 * Destructor
		 */
		~Session_component()
		{
			/* flush and close the open handles */
			while (_node_space.apply_any<Node>([&] (Node &node) {
				_close(node); })) { }

			if (enqueued())
				_active_sessions.remove(*this);
		}

		/**
		 * Increase quotas
		 */
		void upgrade(Genode::Ram_quota ram) {
			_ram_guard.upgrade(ram); }
		void upgrade(Genode::Cap_quota caps) {
			_cap_guard.upgrade(caps); }


		/***************************
		 ** File_system interface **
		 ***************************/

		Dir_handle dir(::File_system::Path const &path, bool create) override
		{
			if (create && (!_writable))
				throw Permission_denied();

			char const *path_str = path.string();

			if (!strcmp(path_str, "/") && create)
				throw Node_already_exists();

			_assert_valid_path(path_str);
			Vfs_server::Path fullpath(_root_path);
			if (path_str[1] != '\0')
				fullpath.append(path_str);
			path_str = fullpath.base();

			if (!create && !_vfs.directory(path_str))
				throw Lookup_failed();

			Directory &dir = *new (_alloc)
				Directory(_node_space, _vfs, _alloc, path_str, create);

			return Dir_handle(dir.id().value);
		}

		File_handle file(Dir_handle dir_handle, Name const &name,
		                 Mode fs_mode, bool create) override
		{
			if ((create || (fs_mode & WRITE_ONLY)) && (!_writable))
				throw Permission_denied();

			return _apply(dir_handle, [&] (Directory &dir) {
				char const *name_str = name.string();
				_assert_valid_name(name_str);

				return File_handle {
					dir.file(_node_space, _vfs, _alloc, name_str, fs_mode, create).value
				};
			});
		}

		Symlink_handle symlink(Dir_handle dir_handle, Name const &name, bool create) override
		{
			if (create && !_writable) throw Permission_denied();

			return _apply(dir_handle, [&] (Directory &dir) {
				char const *name_str = name.string();
				_assert_valid_name(name_str);

				return Symlink_handle {dir.symlink(
					_node_space, _vfs, _alloc, name_str,
					_writable ? READ_WRITE : READ_ONLY, create).value
				};
			});
		}

		Node_handle node(::File_system::Path const &path) override
		{
			char const *path_str = path.string();

			_assert_valid_path(path_str);

			/* re-root the path */
			Path const sub_path(path_str + 1, _root_path.base());
			path_str = sub_path.base();
			if (sub_path != "/" && !_vfs.leaf_path(path_str))
				throw Lookup_failed();

			Node &node = *new (_alloc) Node(_node_space, path_str);

			return Node_handle { node.id().value };
		}

		Watch_handle watch(::File_system::Path const &path) override
		{
			char const *path_str = path.string();

			_assert_valid_path(path_str);

			/* re-root the path */
			Path const sub_path(path_str + 1, _root_path.base());
			path_str = sub_path.base();

			Vfs::Vfs_watch_handle *vfs_handle = nullptr;
			typedef Directory_service::Watch_result Result;
			switch (_vfs.watch(path_str, &vfs_handle, _alloc)) {
			case Result::WATCH_OK: break;
			case Result::WATCH_ERR_UNACCESSIBLE:
				throw Lookup_failed();
			case Result::WATCH_ERR_STATIC:
				throw Unavailable();
			case Result::WATCH_ERR_OUT_OF_RAM:
				throw Out_of_ram();
			case Result::WATCH_ERR_OUT_OF_CAPS:
				throw Out_of_caps();
			}

			Node &node = *new (_alloc)
				Watch_node(_node_space, path_str, *vfs_handle, _finished_nodes);

			return Watch_handle { node.id().value };
		}

		void close(Node_handle handle) override
		{
			/*
			 * Churn the packet queue so that any pending packets on this
			 * handle are processed.
			 */
			_handle_packet_stream();

			try {
				_apply_node(handle, [&] (Node &node) {

					if (node.enqueued()) {
						if (node.acknowledgement_valid()) {
							_finished_nodes.remove(node);
						} else {
							_active_nodes.remove(node);
						}
					}
					_close(node);
				});
			}
			catch (::File_system::Invalid_handle) { }
		}

		Status status(Node_handle node_handle) override
		{
			::File_system::Status fs_stat;

			_apply_node(node_handle, [&] (Node &node) {

				Directory_service::Stat vfs_stat;

				if (_vfs.stat(node.path(), vfs_stat) != Directory_service::STAT_OK)
					throw Invalid_handle();

				auto fs_node_type = [&] (Vfs::Node_type type)
				{
					using To = ::File_system::Node_type;

					switch (type) {
					case Vfs::Node_type::DIRECTORY:          return To::DIRECTORY;
					case Vfs::Node_type::SYMLINK:            return To::SYMLINK;
					case Vfs::Node_type::CONTINUOUS_FILE:    return To::CONTINUOUS_FILE;
					case Vfs::Node_type::TRANSACTIONAL_FILE: return To::TRANSACTIONAL_FILE;
					};
					return To::CONTINUOUS_FILE;
				};

				auto fs_node_size = [&] (Vfs::Directory_service::Stat const &vfs_stat)
				{
					switch (vfs_stat.type) {
					case Vfs::Node_type::DIRECTORY:
						return _vfs.num_dirent(node.path()) * sizeof(Directory_entry);

					case Vfs::Node_type::SYMLINK:
						return 0ULL;

					case Vfs::Node_type::CONTINUOUS_FILE:
					case Vfs::Node_type::TRANSACTIONAL_FILE:
						return vfs_stat.size;
					};
					return 0ULL;
				};

				fs_stat = ::File_system::Status {
					.size = fs_node_size(vfs_stat),
					.type = fs_node_type(vfs_stat.type),
					.rwx  = {
						.readable   = vfs_stat.rwx.readable,
						.writeable  = vfs_stat.rwx.writeable,
						.executable = vfs_stat.rwx.executable },

					.inode = vfs_stat.inode,
					.modification_time = { vfs_stat.modification_time.value }
				};
			});

			return fs_stat;
		}

		void unlink(Dir_handle dir_handle, Name const &name) override
		{
			if (!_writable) throw Permission_denied();

			_apply(dir_handle, [&] (Directory &dir) {
				char const *name_str = name.string();
				_assert_valid_name(name_str);

				Path path(name_str, dir.path());

				assert_unlink(_vfs.unlink(path.base()));
			});
		}

		void truncate(File_handle file_handle, file_size_t size) override
		{
			_apply(file_handle, [&] (File &file) {
				file.truncate(size); });
		}

		void move(Dir_handle from_dir_handle, Name const &from_name,
		          Dir_handle to_dir_handle,   Name const &to_name) override
		{
			if (!_writable)
				throw Permission_denied();

			char const *from_str = from_name.string();
			char const   *to_str =   to_name.string();

			_assert_valid_name(from_str);
			_assert_valid_name(  to_str);

			_apply(from_dir_handle, [&] (Directory &from_dir) {
				_apply(to_dir_handle, [&] (Directory &to_dir) {
					Path from_path(from_str, from_dir.path());
					Path   to_path(  to_str,   to_dir.path());

					assert_rename(_vfs.rename(from_path.base(), to_path.base()));
				});
			});
		}

		void control(Node_handle, Control) override { }
};


class Vfs_server::Root : public Genode::Root_component<Session_component>
{
	private:

		Genode::Env &_env;

		Genode::Attached_rom_dataspace _config_rom { _env, "config" };

		Genode::Xml_node vfs_config()
		{
			try { return _config_rom.xml().sub_node("vfs"); }
			catch (...) {
				Genode::error("VFS not configured");
				_env.parent().exit(~0);
				throw;
			}
		}

		Genode::Signal_handler<Root> _config_handler {
			_env.ep(), *this, &Root::_config_update };

		void _config_update()
		{
			_config_rom.update();
			_vfs_env.root_dir().apply_config(vfs_config());
		}

		/**
		 * The VFS uses an internal heap that
		 * subtracts from the component quota
		 */
		Genode::Heap    _vfs_heap { &_env.ram(), &_env.rm() };
		Vfs::Simple_env _vfs_env  { _env, _vfs_heap, vfs_config() };


		/**
		 * Object for post-I/O-signal processing
		 *
		 * This allows packet and VFS backend signals to
		 * be dispatched quickly followed by a processing
		 * of sessions that might be unblocked.
		 */
		struct Io_progress_handler : Genode::Entrypoint::Io_progress_handler
		{
			/* sessions with active jobs */
			Session_queue active_sessions { };

			/**
			 * Post-signal hook invoked by entrypoint
			 */
			void handle_io_progress() override
			{
				Session_queue still_active_sessions { };

				active_sessions.dequeue_all([&] (Session_component &session) {
					session.process_packets();
					if (!session.no_longer_active())
						still_active_sessions.enqueue(session);
				});

				active_sessions = still_active_sessions;
			}
		} _progress_handler { };

	protected:

		Session_component *_create_session(const char *args) override
		{
			using namespace Genode;

			Session_label const label = label_from_args(args);
			Path session_root;
			bool writeable = false;

			/*****************
			 ** Quota check **
			 *****************/

			auto const initial_ram_usage = _env.pd().used_ram().value;
			auto const initial_cap_usage = _env.pd().used_caps().value;

			auto const ram_quota = parse_ram_quota(args).value;
			auto const cap_quota = parse_cap_quota(args).value;

			size_t tx_buf_size =
				Arg_string::find_arg(args, "tx_buf_size").aligned_size();

			if (!tx_buf_size)
				throw Service_denied();

			size_t session_size =
				max((size_t)4096, sizeof(Session_component)) +
				tx_buf_size;

			if (session_size > ram_quota) {
				error("insufficient 'ram_quota' from '", label, "' "
				      "got ", ram_quota, ", need ", session_size);
				throw Insufficient_ram_quota();
			}


			/**************************
			 ** Apply session policy **
			 **************************/

			/* pull in policy changes */
			_config_rom.update();

			typedef String<MAX_PATH_LEN> Root_path;

			Session_policy policy(label, _config_rom.xml());

			/* check and apply session root offset */
			if (!policy.has_attribute("root")) {
				error("policy lacks 'root' attribute");
				throw Service_denied();
			}
			Root_path const root_path = policy.attribute_value("root", Root_path());
			session_root.import(root_path.string(), "/");

			/*
			 * Determine if the session is writeable.
			 * Policy overrides client argument, both default to false.
			 */
			if (policy.attribute_value("writeable", false))
				writeable = Arg_string::find_arg(args, "writeable").bool_value(false);

			/* apply client's root offset. */
			{
				char tmp[MAX_PATH_LEN] { };
				Arg_string::find_arg(args, "root").string(tmp, sizeof(tmp), "/");
				if (Genode::strcmp("/", tmp, sizeof(tmp))) {
					session_root.append("/");
					session_root.append(tmp);
					session_root.remove_trailing('/');
				}
			}

			/* check if the session root exists */
			if (!((session_root == "/")
			 || _vfs_env.root_dir().directory(session_root.base()))) {
				error("session root '", session_root, "' not found for '", label, "'");
				throw Service_denied();
			}

			Session_component *session = new (md_alloc())
				Session_component(_env, label.string(),
				                  Genode::Ram_quota{ram_quota},
				                  Genode::Cap_quota{cap_quota},
				                  tx_buf_size, _vfs_env.root_dir(),
				                  _progress_handler.active_sessions,
				                  session_root.base(), writeable);

			auto ram_used = _env.pd().used_ram().value - initial_ram_usage;
			auto cap_used = _env.pd().used_caps().value - initial_cap_usage;

			if ((ram_used > ram_quota) || (cap_used > cap_quota)) {
				if (ram_used > ram_quota)
					Genode::warning("ram donation is ", ram_quota,
					                " but used RAM is ", ram_used, "B"
					                ", '", label, "'");
				if (cap_used > cap_quota)
					Genode::warning("cap donation is ", cap_quota,
					                " but used caps is ", cap_used,
					                ", '", label, "'");
			}

			return session;
		}

		/**
		 * Session upgrades are important for the VFS server,
		 * this allows sessions to open arbitrarily large amounts
		 * of handles without starving other sessions.
		 */
		void _upgrade_session(Session_component *session,
		                      char        const *args) override
		{
			Genode::Ram_quota more_ram = parse_ram_quota(args);
			Genode::Cap_quota more_caps = parse_cap_quota(args);

			if (more_ram.value > 0)
				session->upgrade(more_ram);
			if (more_caps.value > 0)
				session->upgrade(more_caps);
		}

	public:

		Root(Genode::Env &env, Genode::Allocator &md_alloc)
		:
			Root_component<Session_component>(&env.ep().rpc_ep(), &md_alloc),
			_env(env)
		{
			_env.ep().register_io_progress_handler(_progress_handler);
			_config_rom.sigh(_config_handler);
			env.parent().announce(env.ep().manage(*this));
		}
};


void Component::construct(Genode::Env &env)
{
	static Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };

	static Vfs_server::Root root { env, sliced_heap };
}
