/*
 * \brief  HRD generator
 * \author Norman Feske
 * \date   2025-08-06
 */

/*
 * Copyright (C) 2025 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <util/hrd.h>

using namespace Genode;


void Hrd_generator::_node(char const *name, Node_fn::Ft const &fn)
{
	if (_node_state.indent.level == 0)
		print(_out_buffer, name);
	else
		print(_out_buffer, "\n", _node_state.indent, "+ ", name);

	if (_out_buffer.exceeded())
		return;

	{
		struct Orig { size_t used; Node_state node_state; };
		Orig const orig { _out_buffer.used(), _node_state, };

		_node_state = { .indent      = { _node_state.indent.level + 1 },
		                .attr_offset = _out_buffer.used(),
		                .has_attr    = false };

		struct Guard
		{
			Hrd_generator &g; Orig orig; bool ok;

			~Guard()
			{
				g._node_state = orig.node_state;
				if (!ok)
					g._out_buffer.rewind(orig.used);
			}
		} guard { .g = *this, .orig = orig, .ok = false };

		fn();
		guard.ok = true;
	}

	if (_node_state.indent.level == 0)
		print(_out_buffer, "\n-\n");
}


void Hrd_generator::_attribute(char const *tag, char const *value, size_t val_len)
{
	auto insert = [&] (size_t gap, auto const &fn)
	{
		_out_buffer.with_inserted_gap(_node_state.attr_offset, gap, [&] (Out_buffer &out) {
			fn(out);
			_node_state.attr_offset += gap; });
	};

	if (!_node_state.has_attr) {
		if (strcmp(tag, "name") == 0)
			insert(1 + val_len, [&] (Out_buffer &out) {
				print(out, " ", Cstring(value, val_len)); });
		else
			insert(2 + strlen(tag) + 2 + val_len, [&] (Out_buffer &out) {
				print(out, "  ", tag, ": ", Cstring(value, val_len)); });
	} else {
		insert(3 + strlen(tag) + 2 + val_len, [&] (Out_buffer &out) {
			print(out, " | ", tag, ": ", Cstring(value, val_len)); });
	}
	_node_state.has_attr = true;
}


#include <util/xml_node.h>

void Hrd_generator::node_attributes(Xml_node const &node)
{
	node.for_each_attribute([&] (auto const &attr) {
		attr.with_raw_value([&] (char const *start, size_t num_bytes) {
			attribute(attr.name().string(), start, num_bytes); }); });
}
