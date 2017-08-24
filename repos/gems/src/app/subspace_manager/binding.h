/*
 * \brief  Binding for the a runtime argument
 * \author Norman Feske
 * \date   2017-08-23
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _BINDING_H_
#define _BINDING_H_

/* local includes */
#include <runtime.h>

namespace Subspace_manager { struct Binding; }


struct Subspace_manager::Binding
{
	typedef String<64> Child_name;
	typedef String<48> Label;

	struct Target
	{
		Resource::Type type;
		bool           parent;
		Child_name     child_name;
		Label          server_label;

		void print(Output &out) const
		{
			using Genode::print;
			print(out, type);
			print(out, ":");
			if (parent) print(out, "parent"); else print(out, child_name);
			if (server_label.valid()) print(out, "[", server_label, "]");
		}
	};

	Target const _target;
	Label  const _client_label;

	struct Invalid : Exception { };

	/*
	 * \throw Invalid
	 */
	static Xml_node _target_xml(Xml_node node)
	{
		if (node.num_sub_nodes())
			return node.sub_node();

		warning("route lacks target node: ", node);
		throw Invalid();
	}

	/**
	 * Constructor
	 *
	 * \throw Invalid
	 */
	Binding(Xml_node node)
	:
		_target( { Resource::Type::from_xml(node),
		           _target_xml(node).has_type("parent"),
		           _target_xml(node).attribute_value("name", Child_name()),
		           _target_xml(node).attribute_value("label", Label()) } ),
		_client_label(node.attribute_value("label", Label()))
	{
		if (!_target.type.valid()) {
			warning("binding to unknown service type '", node.type(), "'");
			throw Invalid();
		}
	}

	Resource::Type type() const { return _target.type; }

	bool matches(Runtime::Arg const &arg) const
	{
		return arg.type() == _target.type && arg.label() == _client_label;
	}

	void print(Output &out) const
	{
		using Genode::print;

		print(out, _target.type);

		if (_client_label.valid())
			print(out, "[", _client_label, "]");

		print(out, ":", _target.parent ? "parent" : _target.child_name.string());

		if (_target.server_label.valid())
			print(out, "[", _target.server_label, "]");
	}
};

#endif /* _BINDING_H_ */
