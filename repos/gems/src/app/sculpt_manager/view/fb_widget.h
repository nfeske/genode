/*
 * \brief  Framebuffer settings
 * \author Norman Feske
 * \date   2024-10-23
 */

/*
 * Copyright (C) 2024 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _VIEW__FB_WIDGET_H_
#define _VIEW__FB_WIDGET_H_

#include <view/dialog.h>
#include <model/fb_connectors.h>

namespace Sculpt { struct Fb_widget; }

struct Sculpt::Fb_widget : Widget<Vbox>
{
	using Connector     = Fb_connectors::Connector;
	using Mode          = Connector::Mode;
	using Hosted_choice = Hosted<Vbox, Choice<Mode::Id>>;
	using Mode_radio    = Hosted<Radio_select_button<Mode::Id>>;

	struct Action : Interface
	{
		virtual void select_fb_mode(Fb_connectors::Name const &, Mode::Id const &) = 0;
	};

	Fb_connectors::Name _selected_connector { };

	void view(Scope<Vbox> &s, Fb_connectors const &connectors) const
	{
		auto view_connector = [&] (Connector const &conn)
		{
			Hosted_choice choice { Id { conn.name }, conn.name };

			Mode::Id selected_mode { "off" };
			conn._modes.for_each([&] (Mode const &mode) {
				if (mode.used)
					selected_mode = mode.id; });

			s.widget(choice,
				Hosted_choice::Attr {
					.left_ex = 12, .right_ex = 20,
					.unfolded      = _selected_connector,
					.selected_item = selected_mode
				},
				[&] (Hosted_choice::Sub_scope &s) {
					conn._modes.for_each([&] (Mode const &mode) {
						String<32> text { mode.name };
						if (mode.hz)
							text = { text, " (", mode.hz, " Hz)" };

						s.widget(Mode_radio { Id { mode.id }, mode.id },
						         selected_mode, text);
					});
					s.widget(Mode_radio { Id { "off" }, "off" }, selected_mode, "off");
				});
		};

		unsigned const num_merged = connectors.num_merged();

		auto view_controls = [&] (Scope<Vbox> &s, unsigned const count, Id const &id)
		{
			if (count <= 1)
				return;

			s.sub_scope<Float>([&] (Scope<Vbox, Float> &s) {
				s.sub_scope<Hbox>(id, [&] (Scope<Vbox, Float, Hbox> &s) {
					s.sub_scope<Button>(Id { "equal" }, [&] (Scope<Vbox, Float, Hbox, Button> &s) {
						if (count <= num_merged)
							s.attribute("selected", "yes");
						if (count == num_merged || count == num_merged + 1) {
							if (s.hovered() && !s.dragged())
								s.attribute("hovered", "yes");
						} else  {
							s.attribute("style", "unimportant");
						}
						s.sub_scope<Label>("=");
					});
					s.sub_scope<Button>(Id { "swap" }, [&] (Scope<Vbox, Float, Hbox, Button> &s) {
						s.sub_scope<Label>("Swap");
					});
				});
			});
		};

		unsigned count = 0;

		connectors._merged.for_each([&] (Connector const &conn) {
			count++;
			view_controls(s, count, Id { conn.name });
			view_connector(conn);
		});

		connectors._discrete.for_each([&] (Connector const &conn) {
			count++;
			view_controls(s, count, Id { conn.name });
			view_connector(conn);
		});
	}

	void click(Clicked_at const &at, Fb_connectors const &connectors, Action &action)
	{
		auto click_connector = [&] (Connector const &conn)
		{
			Hosted_choice choice { Id { conn.name }, conn.name };

			choice.propagate(at, _selected_connector,
				[&] { _selected_connector = { }; },
				[&] (Clicked_at const &at) {
					Id const id = at.matching_id<Mode_radio>();
					if (id.valid())
						action.select_fb_mode(conn.name, id);
				});
		};

		connectors._merged.for_each([&] (Connector const &conn) {
			click_connector(conn); });

		connectors._discrete.for_each([&] (Connector const &conn) {
			click_connector(conn); });
	}
};

#endif /* _VIEW__FB_WIDGET_H_ */
