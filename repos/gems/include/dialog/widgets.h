/*
 * \brief  Widget types
 * \author Norman Feske
 * \date   2023-03-24
 */

/*
 * Copyright (C) 2023 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__DIALOG__WIDGETS_H_
#define _INCLUDE__DIALOG__WIDGETS_H_

#include <util/color.h>
#include <dialog/sub_scopes.h>

namespace Dialog {

	template <typename> struct Select_button;
	struct Toggle_button;
	struct Action_button;
	struct Deferred_action_button;
}


struct Dialog::Toggle_button : Widget<Button>
{
	template <typename FN>
	void view(Scope<Button> &s, bool, FN const &fn) const
	{
		s.attribute("style", "invisible");
		fn(s);
	}

	void view(Scope<Button> &s, bool selected) const
	{
		bool const hovered = s.hovered() && !s.dragged();

		view(s, selected, [&] (Scope<Button> &s) {
			s.sub_scope<Dialog::Label>(s.id.value, [&] (auto &s) {
				auto color = [&]
				{
					if (hovered)  return Color { 255, 255, 200 };
					if (selected) return Color { 200, 200, 255 };
					else          return Color { 150, 150, 150 };
				};
				s.attribute("font", "button/metal");
				s.attribute("color", String<30>(color()));
			});
		});
	}

	template <typename FN>
	void click(Clicked_at const &, FN const &toggle_fn) const { toggle_fn(); }
};


template <typename ENUM>
struct Dialog::Select_button : Widget<Button>
{
	ENUM const _value;

	Select_button(ENUM value) : _value(value) { }

	void view(Scope<Button> &s, ENUM, auto const &fn) const
	{
		s.attribute("style", "invisible");
		fn(s);
	}

	void view(Scope<Button> &s, ENUM selected_value) const
	{
		bool const selected = (selected_value == _value),
		           hovered  = (s.hovered() && !s.dragged() && !selected);

		view(s, selected_value, [&] (auto &s) {
			s.template sub_scope<Dialog::Label>(s.id.value, [&] (auto &s) {
				auto color = [&]
				{
					if (selected) return Color { 255, 255, 255 };
					if (hovered)  return Color { 255, 255, 200 };
					else          return Color { 150, 150, 150 };
				};
				s.attribute("font", "button/metal");
				s.attribute("color", String<30>(color()));
			});
		});
	}

	template <typename FN>
	void click(Clicked_at const &, FN const &select_fn) const { select_fn(_value); }
};


struct Dialog::Action_button : Widget<Button>
{
	Event::Seq_number _seq_number { };

	template <typename FN>
	void view(Scope<Button> &s, FN const &fn) const
	{
		fn(s);
	}

	void view(Scope<Button> &s) const
	{
		s.attribute("style", "invisible");

		bool const selected = _seq_number == s.hover.seq_number,
		           hovered  = (s.hovered() && (!s.dragged() || selected));

		view(s, [&] (Scope<Button> &s) {
			s.sub_scope<Label>(s.id.value, [&] (auto &s) {

				auto color = [&]
				{
					if (selected) return Color { 255, 255, 255 };
					if (hovered)  return Color { 255, 255, 200 };
					else          return Color { 150, 150, 150 };
				};
				s.attribute("font", "button/metal");
				s.attribute("color", String<30>(color()));
			});
		});
	}

	template <typename FN>
	void click(Clicked_at const &at, FN const &activate_fn)
	{
		_seq_number = at.seq_number;
		activate_fn();
	}
};


struct Dialog::Deferred_action_button : Widget<Button>
{
	Event::Seq_number _seq_number { };  /* remembered at proposal time */

	template <typename FN>
	void view(Scope<Button> &s, FN const &fn) const
	{
		s.attribute("style", "invisible");
		fn(s);
	}

	void view(Scope<Button> &s) const
	{
		bool const selected = s.hovered() && s.dragged() && s.hover.matches(_seq_number),
		           hovered  = s.hovered() && (!s.dragged() || selected);

		view(s, [&] (Scope<Button> &s) { s.sub_scope<Label>(s.id.value, [&] (auto &s) {
			auto color = [&]
			{
				if (selected) return Color { 255, 255, 255 };
				if (hovered)  return Color { 255, 255, 200 };
				else          return Color { 150, 150, 150 };
			};
			s.attribute("font", "button/metal");
			s.attribute("color", String<30>(color()));
		}); });
	}

	void click(Clicked_at const &at)
	{
		_seq_number = at.seq_number;
	}

	template <typename FN>
	void clack(Clacked_at const &at, FN const &activate_fn)
	{
		if (at.matches(_seq_number))
			activate_fn();
	}
};

#endif /* _INCLUDE__DIALOG__WIDGETS_H_ */
