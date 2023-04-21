/*
 * \brief  Sculpt deploy management
 * \author Norman Feske
 * \date   2018-04-30
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* local includes */
#include <deploy.h>
#include <string.h>


bool Sculpt::Deploy::update_child_conditions()
{
	/* return true if any condition changed */
	return _children.apply_condition([&] (Xml_node start, Xml_node launcher) {

		/* the child cannot run as long as any dependency is missing */
		bool condition = true;
		_for_each_missing_server(start, [&] (Start_name const &) {
			condition = false; });
		_for_each_missing_server(launcher, [&] (Start_name const &) {
			condition = false; });

		return condition;
	});
}


void Sculpt::Deploy::gen_child_diagnostics(Xml_generator &xml) const
{
	/*
	 * Collect messages in registry, avoiding duplicates
	 */
	typedef String<64> Message;
	typedef Registered_no_delete<Message> Registered_message;
	Registry<Registered_message> messages { };

	auto gen_missing_dependencies = [&] (Xml_node start, Start_name const &name)
	{
		_for_each_missing_server(start, [&] (Start_name const &server) {

			Message const new_message(Pretty(name), " requires ", Pretty(server));

			bool already_exists = false;
			messages.for_each([&] (Registered_message const &message) {
				if (message == new_message)
					already_exists = true; });

			if (!already_exists)
				new (_alloc) Registered_message(messages, new_message);
		});
	};

	_children.for_each_unsatisfied_child([&] (Xml_node start, Xml_node launcher,
	                                          Start_name const &name) {
		gen_missing_dependencies(start,    name);
		gen_missing_dependencies(launcher, name);
	});

	/*
	 * Generate dialog elements, drop consumed messages from the registry
	 */
	int count = 0;
	messages.for_each([&] (Registered_message &message) {
		gen_named_node(xml, "hbox", String<20>(count++), [&] () {
			gen_named_node(xml, "float", "left", [&] () {
				xml.attribute("west", "yes");
				xml.node("label", [&] () {
					xml.attribute("text", message);
					xml.attribute("font", "annotation/regular");
				});
			});
		});
		destroy(_alloc, &message);
	});
}


void Sculpt::Deploy::handle_deploy()
{
	Xml_node const managed_deploy = _managed_deploy_rom.xml();

	/* determine CPU architecture of deployment */
	Arch const orig_arch = _arch;
	_arch = managed_deploy.attribute_value("arch", Arch());
	if ((managed_deploy.type() != "empty") && !_arch.valid())
		warning("managed deploy config lacks 'arch' attribute");

	bool const arch_changed = (orig_arch != _arch);

	auto apply_config = [&]
	{
		try { return _children.apply_config(managed_deploy); }
		catch (...) {
			error("spurious exception during deploy update (apply_config)"); }
		return false;
	};

	bool const config_affected_child = apply_config();

	auto apply_launchers = [&]
	{
		bool any_child_affected = false;

		Xml_node const launcher_listing = _launcher_listing_rom.xml();
		launcher_listing.for_each_sub_node("dir", [&] (Xml_node dir) {

			typedef String<20> Path;
			Path const path = dir.attribute_value("path", Path());

			if (path != "/launcher")
				return;

			dir.for_each_sub_node("file", [&] (Xml_node file) {

				if (file.attribute_value("xml", false) == false)
					return;

				typedef Depot_deploy::Child::Launcher_name Name;
				Name const name = file.attribute_value("name", Name());

				file.for_each_sub_node("launcher", [&] (Xml_node launcher) {
					if (_children.apply_launcher(name, launcher))
						any_child_affected = true; });
			});
		});
		return any_child_affected;
	};

	bool const launcher_affected_child = apply_launchers();

	auto apply_blueprint = [&]
	{
		try {
			Xml_node const blueprint = _blueprint_rom.xml();

			/* apply blueprint, except when stale */
			typedef String<32> Version;
			Version const version = blueprint.attribute_value("version", Version());
			if (version == Version(_depot_query.depot_query_version().value))
				return _children.apply_blueprint(_blueprint_rom.xml());
		}
		catch (...) {
			error("spurious exception during deploy update (apply_blueprint)"); }
		return false;
	};

	bool const blueprint_affected_child = apply_blueprint();

	bool const progress = arch_changed
	                   || config_affected_child
	                   || launcher_affected_child
	                   || blueprint_affected_child;
	if (progress) {

		/* update query for blueprints of all unconfigured start nodes */
		if (!_download_queue.any_active_download())
			_depot_query.trigger_depot_query();

		/* feed missing packages to installation queue */
		update_installation();

		/* apply runtime condition checks */
		update_child_conditions();

		_dialog_generator.generate_dialog();
		_runtime_config_generator.generate_runtime_config();
	}
}


void Sculpt::Deploy::gen_runtime_start_nodes(Xml_generator  &xml,
                                             Prio_levels     prio_levels,
                                             Affinity::Space affinity_space) const
{
	/* depot-ROM instance for regular (immutable) depot content */
	xml.node("start", [&] () {
		gen_fs_rom_start_content(xml, "cached_fs_rom", "depot",
		                         cached_depot_rom_state); });

	/* depot-ROM instance for mutable content (/depot/local/) */
	xml.node("start", [&] () {
		gen_fs_rom_start_content(xml, "fs_rom", "depot",
		                         uncached_depot_rom_state); });

	xml.node("start", [&] () {
		gen_depot_query_start_content(xml); });

	Xml_node const managed_deploy = _managed_deploy_rom.xml();

	/* insert content of '<static>' node as is */
	if (managed_deploy.has_sub_node("static")) {
		Xml_node static_config = managed_deploy.sub_node("static");
		static_config.with_raw_content([&] (char const *start, size_t length) {
			xml.append(start, length); });
	}

	/* generate start nodes for deployed packages */
	if (managed_deploy.has_sub_node("common_routes"))
		_children.gen_start_nodes(xml, managed_deploy.sub_node("common_routes"),
		                          prio_levels, affinity_space,
		                          "depot_rom", "dynamic_depot_rom");
}
