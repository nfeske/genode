/*
 * \brief  Sculpt GUI management
 * \author Norman Feske
 * \date   2018-04-30
 *
 * The GUI is based on a dynamically configured init component, which hosts
 * one menu-view component for each dialog.
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _GUI_H_
#define _GUI_H_

/* Genode includes */
#include <os/reporter.h>

/* local includes */
#include <types.h>
#include <xml.h>

namespace Sculpt { struct Gui; }


struct Sculpt::Gui
{
	Env &_env;

	float _font_size_px = 14;

	typedef String<32> Label;

	struct Version { unsigned value; } version { 0 };

	unsigned menu_width = 0;

	Area _screen_size { };

	void _gen_menu_view_start_content(Xml_generator &, Label const &, Point, Area) const;

	void _generate_config(Xml_generator &) const;

	unsigned panel_height() const { return _font_size_px*3; }

	void gen_runtime_start_nodes(Xml_generator &xml) const;

	float font_size() const { return _font_size_px; }

	void font_size(float px)
	{
		_font_size_px = px;
		menu_width = max(px*21, 320.0);
	}

	void screen_size(Area screen_size) { _screen_size = screen_size; }

	Gui(Env &env) : _env(env) { }
};

#endif /* _GUI_H_ */
