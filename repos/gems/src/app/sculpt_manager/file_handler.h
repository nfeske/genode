 /*
 * \brief  Utility for watching 'Node' content on a file system
 * \author Norman Feske
 * \date   2026-04-02
 */

/*
 * Copyright (C) 2026 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _FILE_HANDLER_H_
#define _FILE_HANDLER_H_

/* Genode includes */
#include <os/vfs.h>

namespace Genode {

	template <typename> class File_handler;
}


template <typename T>
struct Genode::File_handler
{
	private:

		using Path = Directory::Path;

		Entrypoint &_ep;
		Allocator  &_alloc;
		Directory  &_dir;
		Path  const _path;

		Constructible<Readonly_file> _file { };

		size_t _file_size = 0;;

		using Allocation = Allocator::Allocation;

		Allocator::Alloc_result _buffer = Alloc_error::DENIED;

		T &_obj;
		void (T::*_member) (Node const &);

		Constructible<Watch_handler<File_handler>> _dir_handler  { };
		Constructible<Watch_handler<File_handler>> _file_handler { };

		/*
		 * The construction and destruction of watch handlers must not
		 * be done from the context of their own handler contexts. Hence,
		 * we use a distinct signal handler for managing the subscription.
		 */
		Signal_handler<File_handler> _subscribe_handler {
			_ep, *this, &File_handler::_handle_subscribe };

		void _handle_subscribe()
		{
			/* file appeared */
			if (_dir.file_exists(_path) && !_file_handler.constructed()) {
				_dir_handler.destruct();
				_file_handler.construct(_ep, _dir, _path, *this, &File_handler::_handle);
				_file_handler->local_submit(); /* handle initial content */
				return;
			}

			/* file disappeared */
			if (!_dir.file_exists(_path) && _file_handler.constructed())
				_file_handler.destruct();

			/* file does not exist yet, watch inner-most directory */
			_path.with_span([&] (Span const &s) {
				char buf[s.num_bytes] { };
				memcpy(buf, s.start, s.num_bytes);

				for (size_t n = s.num_bytes; n > 0; ) {

					/* remove last path element */
					while (n && buf[n - 1] != '/') n--;

					Path const dir_path { Cstring(buf, n) };
					if (_dir.directory_exists(dir_path)) {
						_dir_handler.construct(_ep, _dir, dir_path,
						                        *this, &File_handler::_handle);
						return;
					}

					if (n) n--; /* skip slash */
				}
			});
		}

		void _handle()
		{
			/* start watching the file once it shows up */
			if (!_file_handler.constructed())
				_subscribe_handler.local_submit();

			unsigned i = 0; /* retry if file size changed during read */
			unsigned const MAX_ATTEMPTS = 10;

			bool disappeared = false;

			for (unsigned i = 0; i < MAX_ATTEMPTS; i++) {

				try { _file_size = _dir.file_size(_path); }
				catch (...) {
					disappeared = true;
					_file_size = 0;
					break;
				}

				size_t const buf_size = _buffer.convert<size_t>(
					[&] (Allocation const &a) { return a.num_bytes; },
					[&] (Alloc_error)         { return 0ul; });

				if (_file_size > buf_size)
					_buffer = _alloc.try_alloc(_file_size);

				if (!_file.constructed())
					try { _file.construct(_dir, _path); } catch (...) { }

				if (!_file.constructed()) {
					disappeared = true;
					break;
				}

				bool ok = false;
				_buffer.with_result(
					[&] (Allocation &a) {

						if (_file->read({ (char *)a.ptr, _file_size }) != _file_size)
							return;

						(_obj.*_member)(Node { Span { (char *)a.ptr, _file_size } });
						ok = true;
					},
					[&] (Alloc_error) {
						error("alloc error in file handler for ", _path); });
				if (ok)
					break;
			}
			if (i == MAX_ATTEMPTS)
				error("giving up reading ", _path, " after ", MAX_ATTEMPTS, " attempts");

			if (disappeared)
				_subscribe_handler.local_submit();
		}

	public:

		File_handler(Entrypoint &ep, Allocator &alloc, Directory &dir,
		             Path const &path, T &obj, void (T::*member)(Node const &))
		:
			_ep(ep), _alloc(alloc), _dir(dir), _path(path),
			_obj(obj), _member(member)
		{
			_handle_subscribe();
		}

		void with_node(auto const &fn) const
		{
			_buffer.with_result(
				[&] (Allocation const &a) {
					fn(Node { Span { (char *)a.ptr, _file_size } }); },
				[&] (Alloc_error) {
					fn(Node()); });
		}
};

#endif /* _FILE_HANDLER_H_ */
