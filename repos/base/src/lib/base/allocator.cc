/*
 * \brief  Support for allocator interface
 * \author Norman Feske
 * \date   2016-10-07
 */

/*
 * Copyright (C) 2016 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#include <base/allocator.h>
#include <base/log.h>

using namespace Genode;


void Allocation_size::_print_error_message()
{
	error("attempt to allocate zero-sized block");
}
