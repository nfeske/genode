/*
 * \brief   Core-internal utilities
 * \author  Martin Stein
 * \author  Stefan Kalkowski
 * \date    2012-01-02
 */

/*
 * Copyright (C) 2012-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__UTIL_H_
#define _CORE__UTIL_H_

#include <hw/util.h>

namespace Genode {

	using Hw::get_page_mask;
	using Hw::trunc_page;
	using Hw::round_page;
	using Hw::trunc;
	using Hw::aligned;
	using Hw::round;

	/**
	 * Select source used for map operations
	 */
	constexpr addr_t map_src_addr(addr_t, addr_t phys) { return phys; }

	/**
	 * Return highest supported flexpage size for the given mapping size
	 */
	constexpr size_t constrain_map_size_log2(size_t size_log2) { return size_log2; }
}

#endif /* _CORE__UTIL_H_ */
