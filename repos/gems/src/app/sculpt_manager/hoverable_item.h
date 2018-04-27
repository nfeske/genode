/*
 * \brief  GUI element that shows hovering feedback
 * \author Norman Feske
 * \date   2018-05-03
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _HOVERABLE_ITEM_H_
#define _HOVERABLE_ITEM_H_

#include "types.h"
#include "hover_tracker.h"

namespace Sculpt_manager { struct Hoverable_item; }


struct Sculpt_manager::Hoverable_item
{
	typedef String<64> Id;

	Id _hovered  { };

	/**
	 * Apply hovering information
	 *
	 * \return true if hovering changed
	 */
	template <typename... ARGS>
	bool hovered(Hover_tracker const &hover_tracker, ARGS &&... args)
	{
		Id const orig = _hovered;
		_hovered = hover_tracker.query<Id>(args...);
		return _hovered != orig;
	}

	/**
	 * Return true if item is currently hovered
	 */
	bool hovered(Id const &id) const { return id == _hovered; }

	/**
	 * Generate button attributes depending on the item state
	 */
	void gen_button_attr(Xml_generator &xml, Id const &id) const
	{
		xml.attribute("name", id);

		if (hovered(id))  xml.attribute("hovered", "yes");
	}
};

#endif /* _HOVERABLE_ITEM_H_ */
