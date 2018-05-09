/*
 * \brief  Representation of a wireless access point
 * \author Norman Feske
 * \date   2018-05-07
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _ACCESS_POINT_H_
#define _ACCESS_POINT_H_

#include "types.h"

namespace Sculpt_manager {

	struct Access_point;
	struct Access_point_update_policy;

	typedef List_model<Access_point> Access_points;
};


struct Sculpt_manager::Access_point : List_model<Access_point>::Element
{
	typedef String<32> Bssid;
	typedef String<32> Ssid;

	enum Protection { UNKNOWN, UNPROTECTED, WPA_PSK };

	Bssid      const bssid;
	Ssid       const ssid;
	Protection const protection;

	unsigned quality = 0;

	Access_point(Bssid const &bssid, Ssid const &ssid, Protection protection)
	: bssid(bssid), ssid(ssid), protection(protection) { }
};


/**
 * Policy for transforming a 'wlan_accesspoints' report into a list model
 */
struct Sculpt_manager::Access_point_update_policy : List_model<Access_point>::Update_policy
{
	Allocator &_alloc;

	Access_point_update_policy(Allocator &alloc) : _alloc(alloc) { }

	void destroy_element(Access_point &elem) { destroy(_alloc, &elem); }

	Access_point &create_element(Xml_node node)
	{
		auto const protection = node.attribute_value("protection", String<16>());

		return *new (_alloc)
			Access_point(node.attribute_value("bssid", Access_point::Bssid()),
			             node.attribute_value("ssid",  Access_point::Ssid()),
			             protection == "WPA-PSK" ? Access_point::Protection::WPA_PSK :
			                                       Access_point::Protection::UNPROTECTED);
	}

	void update_element(Access_point &ap, Xml_node node)
	{
		ap.quality = node.attribute_value("quality", 0UL);
	}

	static bool element_matches_xml_node(Access_point const &elem, Xml_node node)
	{
		return node.attribute_value("ssid", Access_point::Ssid()) == elem.ssid;
	}
};

#endif /* _ACCESS_POINT_H_ */
