/*
 * \brief  Depot remove
 * \author Alice Domage
 */

/*
 * Copyright (C) 2023 Genode Labs GmbH
 * Copyright (C) 2023 gapfruit AG
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <base/attached_rom_dataspace.h>
#include <base/component.h>
#include <base/heap.h>
#include <base/signal.h>
#include <depot/archive.h>
#include <os/reporter.h>
#include <util/reconstructible.h>
#include <util/xml_node.h>
#include <util/xml_generator.h>
#include <os/vfs.h>


namespace Depot_remove {

	using namespace Genode;

	struct Main;
	class  Archive_remover;

}

class Depot_remove::Archive_remover
{
	public:

		using Archive_path = Depot::Archive::Path;

	private:

		Heap                              &_heap;
		Directory                         &_depot;
		Constructible<Expanding_reporter> &_reporter;
		String<32>                         _arch {};


/********************************
 ** Deleted archives reporter. **
 ********************************/

		struct Path_list_element: List<Path_list_element>::Element
		{
		private:

			List<Path_list_element> &_list;

		public:

			Archive_path path;

			Path_list_element(List<Path_list_element> &list, Archive_path const &path)
			:
				_list { list },
				path  { path }
			{ _list.insert(this); }

			~Path_list_element() { _list.remove(this); }
		};

		Genode::List<Path_list_element> _deleted_archives_list {};

		inline void add_archive_to_report(Archive_path const &path)
		{
			if (_reporter.constructed()) {
				new (_heap) Path_list_element(_deleted_archives_list, path);
			}
		}

		inline void generate_report()
		{
			if (_reporter.constructed()) {
				_reporter->generate([&](Reporter::Xml_generator &xml) {
					for (Path_list_element const *elem = _deleted_archives_list.first();
					     elem;
					     elem = elem->next())
					{
						xml.node("removed", [&]() {
							xml.attribute("path", elem->path);
						});
					}
				});
				while (auto *elem = _deleted_archives_list.first()) {
					destroy(_heap, elem);
				}
			}
		}


 /************************
  ** Depot walkthrough. **
  ************************/

		void remove_directory(Directory::Path path)
		{
			Directory dir { _depot, path };

			List<Path_list_element> dirent_file_list;

			dir.for_each_entry([&] (auto const &entry) {
				if (entry.name() == ".." || entry.name() == ".") return;
				else if (entry.type() == Vfs::Directory_service::Dirent_type::DIRECTORY) {
					remove_directory(Directory::join(path, entry.name()));
				}
				else {
					/*
					 * Deleting file within the for_each_entry() confuses lx_fs dirent
					 * offset computation and some files, such as 'README', is consitently
					 * omitted, thus the unlink operation fails. Thus create a list
					 * to delete file out of the lambda.
					 */
					new (_heap) Path_list_element(dirent_file_list, Directory::join(path, entry.name()));
				}
			});
			while (auto *elem = dirent_file_list.first()) {
				_depot.unlink(elem->path);
				destroy(_heap, elem);
			}
			_depot.unlink(path);
		}

		template<typename FUNC = void(Directory::Path const &)>
		void for_each_subdir(Directory::Path const &parent_dir, FUNC fn)
		{
			Directory pkg_dir { _depot, parent_dir };
			pkg_dir.for_each_entry([&fn, &parent_dir](auto const &entry){
				if (entry.name() == ".." || entry.name() == ".") return;
				Directory::Path subdir_path { Directory::join(parent_dir, entry.name()) };
				fn(subdir_path);
			});
		}

		template<typename FUNC = void(Path_list_element *)>
		void for_each_path_list_elem(List<Path_list_element> &list, FUNC fn)
		{
			Path_list_element *elem = list.first();
			while (elem) {
				Path_list_element *next = elem->next();
				fn(elem);
				elem = next;
			}
		}

		template<typename FUNC = void(Directory::Path const &)>
		void for_each_pkg(FUNC fn)
		{
			_depot.for_each_entry([&](auto const &entry) {
				Directory::Path depot_user_pkg_path { entry.name(), "/pkg" };
				if (_depot.directory_exists(depot_user_pkg_path)) {
					for_each_subdir(depot_user_pkg_path, [&] (Directory::Path const &pkg_path) {
						for_each_subdir(pkg_path, [&] (Directory::Path const &pkg_version_path) {
							fn(pkg_version_path);
						});
					});
				}
			});
		}

		void autoremove_pkg_and_dependencies() {

			/* Collect all archive dependencies to delete. */
			for_each_path_list_elem(_pkg_to_delete, [&](Path_list_element *elem) {
				Directory::Path pkg_version_path { elem->path };
				Directory::Path archive_file_path { Directory::join(pkg_version_path, "archives") };
				Genode::File_content archives { _heap, _depot, archive_file_path, { 8192 } };
				archives.for_each_line<Directory::Path>([&](auto const &dependency_path) {
					if (Depot::Archive::type(dependency_path) == Depot::Archive::Type::PKG) return;
					new (_heap) Path_list_element(_archive_to_delete, dependency_path);
				});
				remove_directory(pkg_version_path);
				/* Try to delete the parent if it is empty, if not empty the operation fails. */
				remove_directory(Genode::Directory::join(pkg_version_path, ".."));
				add_archive_to_report(pkg_version_path);
			});

			/* Keep archive dependencies that are still referenced by another PKG. */
			for_each_pkg([&](Directory::Path const &pkg_version_path) {
				Directory::Path archive_file_path { Directory::join(pkg_version_path, "archives") };
				Genode::File_content archives { _heap, _depot, archive_file_path, { 8192 } };
				archives.for_each_line<Directory::Path>([&](auto const &dependency_path) {
					if (Depot::Archive::type(dependency_path) == Depot::Archive::Type::PKG) return;
					for_each_path_list_elem(_archive_to_delete, [&](Path_list_element *elem){
						if (dependency_path == elem->path) {
							destroy(_heap, elem);
						}
					});
				});
			});

			/* Delete archive dependencies. */
			for_each_path_list_elem(_archive_to_delete, [&](Path_list_element *elem){

				Directory::Path archive_path {};

				if (Depot::Archive::type(elem->path) == Depot::Archive::Type::SRC) {
					archive_path = Directory::join(Depot::Archive::user(elem->path), "bin");
					archive_path = Directory::join(archive_path, _arch);
					archive_path = Directory::join(archive_path, Depot::Archive::name(elem->path));
					archive_path = Directory::join(archive_path, Depot::Archive::version(elem->path));
				} else {
					archive_path = elem->path;
				}

				/* If directory does not exist, it might has been deleted before, return silently! */
				if (!_depot.directory_exists(archive_path)) return;

				remove_directory(archive_path);
				add_archive_to_report(archive_path);
				/* Try to delete the parent if it is empty, if not empty the operation fails. */
				remove_directory(Genode::Directory::join(archive_path, ".."));
			});

		}

		List<Path_list_element> _pkg_to_delete     { };
		List<Path_list_element> _archive_to_delete { };

	public:

		void configure_remove_pkgs(Xml_node const &config) {
			/* Iterate over PKGs to be deleted. */
			config.for_each_sub_node("remove", [&](Xml_node const &node) {
				if (node.has_attribute("user")) {
					Archive_path depot_user = node.attribute_value("user", Archive_path {}); 
					if (node.has_attribute("pkg")) {
						Archive_path pkg = node.attribute_value("pkg", Archive_path {}); 
						if (node.has_attribute("version")) {
							Archive_path version = node.attribute_value("version", Archive_path {}); 
							Archive_path pkg_version_path { depot_user, "/pkg/", pkg, "/", version };
							if (_depot.directory_exists(pkg_version_path)) {
								new (_heap) Path_list_element(_pkg_to_delete, pkg_version_path);
							} else {
								warning("package version: '", pkg_version_path, "' does not exist.");
							}
						} else {
							Archive_path pkg_path { depot_user, "/pkg/", pkg };
							if (_depot.directory_exists(pkg_path)) {
								for_each_subdir(pkg_path, [&](Directory::Path const &pkg_version_path){
									new (_heap) Path_list_element(_pkg_to_delete, pkg_version_path);
								});
							} else {
								warning("package: '", pkg_path, "' does not exist.");
							}
						}
					} else {
						Directory::Path depot_user_pkg_path { depot_user, "/pkg" };
						for_each_subdir(depot_user_pkg_path, [&](Directory::Path const &pkg_path){
							for_each_subdir(pkg_path, [&](Directory::Path const &pkg_version_path){
								new (_heap) Path_list_element(_pkg_to_delete, pkg_version_path);
							});
						});
					}
				}
			});
		}

		void configure_remove_all_pkgs(Xml_node const &config) {

			for_each_pkg([&] (Genode::Directory::Path const &pkg_path) {

				bool keep = false;

				config.for_each_sub_node("remove-all", [&](Xml_node const &remove_all_node) {
					remove_all_node.for_each_sub_node("keep", [&](Xml_node const &node) {
						if (node.has_attribute("user")) {
							if (Depot::Archive::user(pkg_path) == node.attribute_value("user", Archive_path {})) {
								if (node.has_attribute("pkg")) {
									if (Depot::Archive::name(pkg_path) == node.attribute_value("pkg", Archive_path {})) {
										if (node.has_attribute("version")) {
											if (Depot::Archive::version(pkg_path) == node.attribute_value("version", Archive_path {})) {
												keep = true;
											}
										} else {
											keep = true;
										}
									}
								} else {
									keep = true;
								}
							}
						}
					});
				});

				if (!keep)
					new (_heap) Path_list_element(_pkg_to_delete, pkg_path);
			});
		}


		Archive_remover(Heap                              &heap,
		                Directory                         &depot,
		                Xml_node const                    &config,
		                Constructible<Expanding_reporter> &reporter)
		:
			_heap     { heap },
			_depot    { depot },
			_reporter { reporter }
		{
			if (config.has_attribute("arch")) {
				_arch = config.attribute_value("arch", _arch);
			} else {
				error("Missing arch attribute.");
				return;
			}

			if (config.has_sub_node("remove") && config.has_sub_node("remove-all")) {
				error("<remove/> and <remove-all/> are mutually exclusive.");
				return;
			}

			if (config.has_sub_node("remove")) configure_remove_pkgs(config);
			if (config.has_sub_node("remove-all")) configure_remove_all_pkgs(config);

			autoremove_pkg_and_dependencies();

			generate_report();

			while (Path_list_element *elem = _pkg_to_delete.first()) destroy(_heap, elem);
			while (Path_list_element *elem = _archive_to_delete.first()) destroy(_heap, elem);
		}
};

struct Depot_remove::Main
{
	Env                              &env;
	Heap                              heap            { env.ram(), env.rm() };
	Attached_rom_dataspace            config_rom      { env, "config" };
	Signal_handler<Main>              config_handler  { env.ep(), *this, &Main::handle_config };
	Constructible<Archive_remover>    archive_cleaner { };
	Constructible<Expanding_reporter> reporter        { };

	Main(Env &env)
	:
		env { env },
		config_handler { env.ep(), *this, &Main::handle_config }
	{
		config_rom.sigh(config_handler);
		handle_config();
	}

	void handle_config()
	{
		config_rom.update();
		Xml_node const &config { config_rom.xml() };

		if (config.attribute_value("report", false)) {
			if (!reporter.constructed())
				reporter.construct(env, "removed_archives", "archive_list");
		} else {
			reporter.destruct();
		}

		if (!config.has_sub_node("vfs")) {
			error("Configuration misses a <vfs> configuration node.");
			return;
		}

		Directory::Path depot_path { "depot" };
		Root_directory root_directory { env, heap, config.sub_node("vfs") };
		Directory depot { root_directory, depot_path };

		try {
			archive_cleaner.construct(heap, depot, config, reporter);
		} catch (...) {
			/* Catch any exceptions to prevent the component to abort. */
			error("Depot autoclean job finished with error(s).");
		}
		archive_cleaner.destruct();
	}
};


void Component::construct(Genode::Env &env)
{
	static Depot_remove::Main main(env);
}

