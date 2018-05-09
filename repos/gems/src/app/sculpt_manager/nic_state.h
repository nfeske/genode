/*
 * \brief  State of the NIC session provided by the NIC router
 * \author Norman Feske
 * \date   2018-05-08
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _NIC_STATE_H_
#define _NIC_STATE_H_

#include "types.h"

namespace Sculpt_manager { struct Nic_state; }


struct Sculpt_manager::Nic_state
{
	typedef String<32> Ipv4;

	Ipv4 ipv4;

	static Nic_state from_xml(Xml_node node)
	{
		Ipv4 result { };
		node.for_each_sub_node("domain", [&] (Xml_node domain) {
			if (domain.attribute_value("name", String<16>()) == "uplink")
				result = domain.attribute_value("ipv4", Ipv4()); });

		return Nic_state { result };
	}

	bool ready() const { return ipv4.valid(); }
};

#endif /* _NIC_STATE_H_ */
