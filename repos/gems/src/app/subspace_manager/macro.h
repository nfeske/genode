/*
 * \brief  Macro
 * \author Norman Feske
 * \date   2017-08-23
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _MACRO_H_
#define _MACRO_H_

/* Genode includes */
#include <util/reconstructible.h>

/* local includes */
#include <runtime.h>
#include <binding_list.h>

namespace Subspace_manager { struct Macro; }


struct Subspace_manager::Macro : Binding_list
{
	typedef String<112> Name;

	Allocator &_alloc;

	Runtime::Name const _runtime_name;

	Constructible<Runtime> _runtime;

	Macro(Allocator &alloc, Xml_node node)
	:
		Binding_list(alloc, node),
		_alloc(alloc),
		_runtime_name(node.attribute_value("runtime", Runtime::Name()))
	{ }

	bool runtime_defined() const { return _runtime.constructed(); }

	void define_runtime(Xml_node runtime)
	{
		if (!_runtime.constructed())
			_runtime.construct(_alloc, runtime);
	}

	Runtime::Name runtime_name() const { return _runtime_name; }
};

#endif /* _MACRO_H_ */
