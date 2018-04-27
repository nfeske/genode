/*
 * \brief  Argument type for denoting a network interface
 * \author Norman Feske
 * \date   2018-04-30
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _NIC_TARGET_H_
#define _NIC_TARGET_H_

#include "types.h"

namespace Sculpt_manager { struct Nic_target; }


struct Sculpt_manager::Nic_target
{
	enum Type { OFF, LOCAL, WIRED, WIFI };

	Type type;
};

#endif /* _NIC_TARGET_H_ */
