/*
 * \brief  Error-code helpers
 * \author Norman Feske
 * \date   2025-03-05
 */

/*
 * Copyright (C) 2025 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/log.h>


void Genode::print(Output &out, Alloc_error e)
{
	auto name = [] (Alloc_error e) {
		switch (e) {
		case Alloc_error::OUT_OF_RAM:  return "OUT_OF_RAM";
		case Alloc_error::OUT_OF_CAPS: return "OUT_OF_CAPS";
		case Alloc_error::DENIED:      return "DENIED"; }
		return "<unknown>";
	};
	Genode::print(out, name(e));
}
