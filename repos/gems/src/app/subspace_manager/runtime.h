/*
 * \brief  Representation of a runtime description
 * \author Norman Feske
 * \date   2017-08-23
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _RUNTIME_H_
#define _RUNTIME_H_

/* Genode includes */
#include <util/string.h>

/* local includes */
#include <resource.h>

namespace Subspace_manager { struct Runtime; }


struct Subspace_manager::Runtime
{
	typedef String<122> Name;

	struct Arg
	{
		Resource::Type  const _type;
		Resource::Label const _label;

		Arg(Xml_node node)
		:
			_type(Resource::Type::from_xml(node)),
			_label(node.attribute_value("label", Resource::Label()))
		{ }

		Resource::Type type() const { return _type; }

		bool operator == (Arg const &other) const
		{
			return _type.value == other._type.value && _label == other._label;
		}

		Resource::Label label() const { return _label; }
	};

	Managed_list<Arg> _expected_resources;

	Resource_provider _provided_resources;

	static Xml_node _provides_xml(Xml_node runtime)
	{
		return runtime.has_sub_node("provides") ? runtime.sub_node("provides")
		                                        : Xml_node("<provides/>");
	}

	Runtime(Allocator &alloc, Xml_node runtime)
	:
		_expected_resources(alloc),
		_provided_resources(alloc, _provides_xml(runtime))
	{
		runtime.for_each_sub_node([&] (Xml_node node) {
			Resource::Type type = Resource::Type::from_xml(node);
			if (type.valid())
				_expected_resources.append(node);
		});

		Name const name = runtime.attribute_value("name", Name());
		log("resources expected by runtime '", name, "'");
		_expected_resources.for_each([&] (Arg const &arg) {
			log("  type=", arg.type(), " label=", arg.label()); });
	}
};

#endif /* _RUNTIME_H_ */
