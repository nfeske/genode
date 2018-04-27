/*
 * \brief  Connection state of the wireless driver
 * \author Norman Feske
 * \date   2018-05-08
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _WIFI_CONNECTION_H_
#define _WIFI_CONNECTION_H_

#include "access_point.h"

namespace Sculpt_manager { struct Wifi_connection; }


struct Sculpt_manager::Wifi_connection
{
	enum State { DISCONNECTED, CONNECTED };

	State state;

	Access_point::Bssid bssid;
	Access_point::Ssid  ssid;

	/**
	 * Create 'Wifi_connection' object from 'wlan_state' report
	 */
	static Wifi_connection from_xml(Xml_node node)
	{
		bool const connected =
			node.has_sub_node("accesspoint") &&
			node.sub_node("accesspoint").attribute("state").has_value("connected");

		if (!connected)
			return { .state = DISCONNECTED, Access_point::Ssid{}, Access_point::Bssid{} };

		Xml_node const ap = node.sub_node("accesspoint");

		return { .state = CONNECTED,
		         .bssid = ap.attribute_value("bssid", Access_point::Bssid()),
		         .ssid  = ap.attribute_value("ssid",  Access_point::Ssid()) };
	}

	static Wifi_connection disconnected_wifi_connection()
	{
		return Wifi_connection { DISCONNECTED, Access_point::Bssid{}, Access_point::Ssid{} };
	}

	bool connected() const { return state == CONNECTED; }
};

#endif /* _WIFI_CONNECTION_H_ */
