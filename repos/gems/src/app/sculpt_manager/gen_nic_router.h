/*
 * \brief  XML configuration for the NIC router
 * \author Norman Feske
 * \date   2018-05-08
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _GEN_NIC_ROUTER_H_
#define _GEN_NIC_ROUTER_H_

#include "xml.h"

namespace Sculpt_manager {

	void gen_nic_router_start_content(Xml_generator &, Nic_target const &);
}


void Sculpt_manager::gen_nic_router_start_content(Xml_generator &xml,
                                                  Nic_target const &nic_target)
{
	gen_common_start_content(xml, "nic_router",
	                         Cap_quota{300}, Ram_quota{10*1024*1024});

	gen_provides<Nic::Session>(xml);

	xml.node("config", [&] () {
		xml.attribute("verbose_domain_state", "yes");

		xml.node("report", [&] () {
			xml.attribute("interval_sec", "5");
			xml.attribute("bytes",        "yes");
			xml.attribute("config",       "yes");
		});

		xml.node("default-policy", [&] () {
			xml.attribute("domain", "default"); });

		gen_named_node(xml, "domain", "uplink", [&] () {
			xml.node("nat", [&] () {
				xml.attribute("domain",    "default");
				xml.attribute("tcp-ports", "1000");
				xml.attribute("udp-ports", "1000");
				xml.attribute("icmp-ids",  "1000");
			});
		});

		gen_named_node(xml, "domain", "default", [&] () {
			xml.attribute("interface", "10.0.1.1/24");

			xml.node("dhcp-server", [&] () {
				xml.attribute("ip_first",        "10.0.1.2");
				xml.attribute("ip_last",         "10.0.1.2");
				xml.attribute("dns_server_from", "uplink"); });

			xml.node("tcp", [&] () {
				xml.attribute("dst", "0.0.0.0/0");
				xml.node("permit-any", [&] () {
					xml.attribute("domain", "uplink"); }); });

			xml.node("udp", [&] () {
				xml.attribute("dst", "0.0.0.0/0");
				xml.node("permit-any", [&] () {
					xml.attribute("domain", "uplink"); }); });

			xml.node("icmp", [&] () {
				xml.attribute("dst", "0.0.0.0/0");
				xml.attribute("domain", "uplink"); });
		});
	});

	xml.node("route", [&] () {
		gen_parent_rom_route(xml, "nic_router");
		gen_parent_rom_route(xml, "ld.lib.so");
		gen_parent_route<Cpu_session>     (xml);
		gen_parent_route<Pd_session>      (xml);
		gen_parent_route<Rm_session>      (xml);
		gen_parent_route<Log_session>     (xml);
		gen_parent_route<Timer::Session>  (xml);
		gen_parent_route<Report::Session> (xml);

		gen_service_node<Nic::Session>(xml, [&] () {

			if (nic_target.type == Nic_target::WIRED)
				gen_named_node(xml, "child", "nic_drv");

			if (nic_target.type == Nic_target::WIFI)
				gen_named_node(xml, "child", "wifi_drv");
		});
	});
}

#endif /* _GEN_NIC_ROUTER_H_ */
