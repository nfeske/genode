/*
 * \brief  Utility for tracking and interpreting the hovering state
 * \author Norman Feske
 * \date   2018-04-30
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _HOVER_TRACKER_H_
#define _HOVER_TRACKER_H_

#include "types.h"

namespace Sculpt_manager { struct Hover_tracker; }


struct Sculpt_manager::Hover_tracker
{
	struct Responder : Interface
	{
		virtual void respond_to_hover_change() = 0;
	};

	Responder &_responder;

	Attached_rom_dataspace _hover_rom;

	Signal_handler<Hover_tracker> _hover_handler;

	void _handle_hover()
	{
		_hover_rom.update();
		_responder.respond_to_hover_change();
	}

	Hover_tracker(Env &env, Session_label const &label, Responder &responder)
	:
		_responder(responder),
		_hover_rom(env, label.string()),
		_hover_handler(env.ep(), *this, &Hover_tracker::_handle_hover)
	{
		_hover_rom.sigh(_hover_handler);
	}

	template <typename T>
	T _attribute_value(Xml_node node, char const *attr_name) const
	{
		return node.attribute_value(attr_name, T{});
	}

	template <typename T, typename... ARGS>
	T _attribute_value(Xml_node node, char const *sub_node_type, ARGS &&... args) const
	{
		if (!node.has_sub_node(sub_node_type))
			return T{};

		return _attribute_value<T>(node.sub_node(sub_node_type), args...);
	}

	/**
	 * Query attribute value from hover state
	 *
	 * The list of arguments except for the last one refer to XML path into the
	 * hover state. The last argument denotes the queried attribute name.
	 */
	template <typename T, typename... ARGS>
	T query(ARGS &&... args) const
	{
		return _attribute_value<T>(_hover_rom.xml(), args...);
	}

	/**
	 * Return true if dialog is hovered
	 */
	bool hovered() const { return _hover_rom.xml().has_sub_node("dialog"); }
};

#endif /* _HOVER_TRACKER_H_ */
