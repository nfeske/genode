/*
 * \brief  XML configuration for the chroot component
 * \author Norman Feske
 * \date   2017-12-08
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _GEN_CHROOT_H_
#define _GEN_CHROOT_H_

#include "xml.h"

namespace Sculpt_manager {

	void gen_chroot_start_content(Xml_generator &, Start_name const &,
	                              Path const &, Writeable);
}


void Sculpt_manager::gen_chroot_start_content(Xml_generator &xml, Start_name const &name,
                                              Path const &path, Writeable writable)
{
	gen_common_start_content(xml, name, Cap_quota{100}, Ram_quota{2*1024*1024});

	gen_named_node(xml, "binary", "chroot");

	xml.node("config", [&] () {
		xml.node("default-policy", [&] () {
			xml.attribute("path", path);
			if (writable == WRITEABLE)
				xml.attribute("writeable", "yes");
		});
	});

	gen_provides<::File_system::Session>(xml);

	xml.node("route", [&] () {

	 	gen_service_node<::File_system::Session>(xml, [&] () {
			gen_named_node(xml, "child", "default_fs_rw"); });

		gen_parent_rom_route(xml, "chroot");
		gen_parent_rom_route(xml, "ld.lib.so");
		gen_parent_route<Cpu_session>(xml);
		gen_parent_route<Pd_session> (xml);
		gen_parent_route<Log_session>(xml);
	});
}

#endif /* _GEN_CHROOT_H_ */
