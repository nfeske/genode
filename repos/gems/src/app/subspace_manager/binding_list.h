/*
 * \brief  List of runtime-argument bindings
 * \author Norman Feske
 * \date   2017-08-23
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _BINDING_LIST_H_
#define _BINDING_LIST_H_

/* local includes */
#include <binding.h>

namespace Subspace_manager { struct Binding_list; }


struct Subspace_manager::Binding_list
{
	Managed_list<Binding> _bindings;

	Binding_list(Allocator &alloc, Xml_node node) : _bindings(alloc)
	{
		node.for_each_sub_node([&] (Xml_node sub_node) {
			_bindings.append(sub_node); });
	}

	template <typename FN>
	bool resolve(Runtime::Arg const &arg, FN const &fn) const
	{
		bool done = false;
		_bindings.for_each([&] (Binding const &binding) {
			if (!done && binding.matches(arg)) {
				fn(binding);
				done = true;
			}
		});
		return done;
	}
};

#endif /* _BINDING_LIST_H_ */
