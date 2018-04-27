/*
 * \brief  XML configuration for loading configuration data
 * \author Norman Feske
 * \date   2018-05-08
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _GEN_LOAD_H_
#define _GEN_LOAD_H_

#include "xml.h"

namespace Sculpt_manager {

	struct Load_version { unsigned value; };

	void gen_load_start_content(Xml_generator &, Storage_target const &, Load_version);
}


void Sculpt_manager::gen_load_start_content(Xml_generator        &xml,
                                            Storage_target const &target,
                                            Load_version          version)
{
	xml.attribute("version", version.value);

	gen_common_start_content(xml, "load", Cap_quota{500}, Ram_quota{100*1024*1024});

	gen_named_node(xml, "binary", "noux");

	char const * const script =
		"export VERSION=`cat /VERSION`\n"
		"cp /rw/config/$VERSION/*                   /config/\n"
		"cp /rw/config/$VERSION/subinit/*.config    /config/subinit/\n"
		"cp /rw/config/$VERSION/deploy/*            /config/deploy/\n"
		"cp /rw/config/$VERSION/leitzentrale/config /config/leitzentrale/\n"
		"cp /rw/config/$VERSION/runtime/*.config    /config/runtime/\n"
		"cp /rw/config/$VERSION/runtime/config      /config/runtime/\n";

	xml.node("config", [&] () {
		xml.attribute("stdout", "/dev/null");
		xml.attribute("stderr", "/dev/null");
		xml.attribute("stdin",  "/dev/null");

		xml.node("fstab", [&] () {

			gen_named_node(xml, "tar", "bash-minimal.tar");
			gen_named_node(xml, "tar", "coreutils-minimal.tar");

			gen_named_node(xml, "inline", ".bash_profile", [&] () {
				xml.append(script); });

			gen_named_node(xml, "dir", "dev", [&] () {
				xml.node("null",  [&] () {});
				xml.node("log",   [&] () {});
				xml.node("zero",  [&] () {}); });

			gen_named_node(xml, "dir", "rw", [&] () {
				xml.node("fs",  [&] () { xml.attribute("label", "target"); }); });

			gen_named_node(xml, "dir", "config", [&] () {
				xml.node("fs",  [&] () { xml.attribute("label", "config"); }); });

			gen_named_node(xml, "rom", "VERSION");
		});

		gen_named_node(xml, "start", "/bin/bash", [&] () {

			gen_named_node(xml, "env", "HOME", [&] () {
				xml.attribute("value", "/"); });

			gen_named_node(xml, "env", "TERM", [&] () {
				xml.attribute("value", "screen"); });

			xml.node("arg", [&] () { xml.attribute("value", "--login"); });
		});
	});

	xml.node("route", [&] () {
		gen_parent_rom_route(xml, "noux");
		gen_parent_rom_route(xml, "ld.lib.so");
		gen_parent_rom_route(xml, "bash-minimal.tar");
		gen_parent_rom_route(xml, "coreutils-minimal.tar");
		gen_parent_rom_route(xml, "vfs.lib.so");
		gen_parent_rom_route(xml, "libc.lib.so");
		gen_parent_rom_route(xml, "libc_noux.lib.so");
		gen_parent_rom_route(xml, "libm.lib.so");
		gen_parent_rom_route(xml, "posix.lib.so");
		gen_parent_route<Cpu_session>    (xml);
		gen_parent_route<Pd_session>     (xml);
		gen_parent_route<Log_session>    (xml);
		gen_parent_route<Rom_session>    (xml);
		gen_parent_route<Timer::Session> (xml);

		gen_service_node<::File_system::Session>(xml, [&] () {
			xml.attribute("label", "target");
			gen_named_node(xml, "child", String<32>(target.label(), ".fs")); });

		gen_service_node<::File_system::Session>(xml, [&] () {
			xml.attribute("label", "config");
			xml.node("parent", [&] () { xml.attribute("label", "config"); }); });
	});
}

#endif /* _GEN_E2FS_H_ */
