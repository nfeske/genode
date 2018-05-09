/*
 * \brief  Utility for querying the child-exit state from init's state report
 * \author Norman Feske
 * \date   2018-04-30
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CHILD_EXIT_STATE_H_
#define _CHILD_EXIT_STATE_H_

/* Genode includes */
#include <util/xml_node.h>

/* local includes */
#include "types.h"

namespace Sculpt_manager { struct Child_exit_state; }

struct Sculpt_manager::Child_exit_state
{
	bool exists = false;
	bool exited = false;
	int  code   = 0;

	typedef String<64> Name;

	Child_exit_state(Xml_node init_state, Name const &name)
	{
		init_state.for_each_sub_node("child", [&] (Xml_node child) {
			if (child.attribute_value("name", Name()) == name) {
				exists = true;
				if (child.has_attribute("exited")) {
					exited = true;
					code = child.attribute_value("exited", 0L);
				}
			}
		});
	}
};

#endif /* _CHILD_EXIT_STATE_H_ */
