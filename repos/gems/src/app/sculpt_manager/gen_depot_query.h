/*
 * \brief  XML configuration for the depot-query tool
 * \author Norman Feske
 * \date   2018-05-09
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _GEN_DEPOT_QUERY_H_
#define _GEN_DEPOT_QUERY_H_

#include "xml.h"

namespace Sculpt_manager { void gen_depot_query_start_content(Xml_generator &); }


void Sculpt_manager::gen_depot_query_start_content(Xml_generator &xml)
{
	gen_common_start_content(xml, "depot_query",
	                         Cap_quota{200}, Ram_quota{2*1024*1024});

	gen_named_node(xml, "binary", "depot_query");

	xml.node("config", [&] () {
		xml.attribute("query", "rom");
		xml.node("vfs", [&] () {
			gen_named_node(xml, "dir", "depot", [&] () {
				xml.node("fs", [&] () {}); }); }); });

	xml.node("route", [&] () {
		gen_parent_rom_route(xml, "depot_query");
		gen_parent_rom_route(xml, "ld.lib.so");
		gen_parent_rom_route(xml, "vfs.lib.so");
		gen_parent_rom_route(xml, "query", "config -> deploy/query");

		gen_parent_route<Cpu_session>     (xml);
		gen_parent_route<Pd_session>      (xml);
		gen_parent_route<Log_session>     (xml);
		gen_parent_route<Report::Session> (xml);

		gen_service_node<::File_system::Session>(xml, [&] () {
			gen_named_node(xml, "child", "depot"); });
	});
}

#endif /* _GEN_DEPOT_QUERY_H_ */
