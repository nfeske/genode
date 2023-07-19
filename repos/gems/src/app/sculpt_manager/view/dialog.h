/*
 * \brief  Menu-view dialog handling
 * \author Norman Feske
 * \date   2023-07-12
 */

/*
 * Copyright (C) 2023 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _VIEW__DIALOG_H_
#define _VIEW__DIALOG_H_

#include <dialog/widgets.h>

namespace Sculpt { using namespace Dialog; }


namespace Dialog {

	struct Annotation;
	struct Left_annotation;
	struct Left_right_annotation;
	struct Titled_frame;
}


struct Dialog::Annotation : Sub_scope
{
	template <typename SCOPE, typename TEXT>
	static void sub_node(SCOPE &s, TEXT const &text)
	{
		s.sub_node("label", [&] {
			s.attribute("text", text);
			s.attribute("font", "annotation/regular"); });
	}
};


struct Dialog::Left_annotation : Sub_scope
{
	template <typename SCOPE, typename TEXT>
	static void view_sub_scope(SCOPE &s, TEXT const &text)
	{
		s.node("hbox", [&] {
			s.sub_node("float", [&] () {
				s.attribute("west", "yes");
				Annotation::sub_node(s, text); }); });
	}

	template <typename AT, typename FN>
	static void with_narrowed_at(AT const &, FN const &) { }
};


struct Dialog::Left_right_annotation : Sub_scope
{
	template <typename SCOPE, typename LEFT_TEXT, typename RIGHT_TEXT>
	static void view_sub_scope(SCOPE &s, LEFT_TEXT const &left, RIGHT_TEXT const &right)
	{
		s.node("hbox", [&] {
			s.named_sub_node("float", "left", [&] () {
				s.attribute("west", "yes");
				Annotation::sub_node(s, left); });

			s.named_sub_node("float", "right", [&] () {
				s.attribute("east", "yes");
				Annotation::sub_node(s, right); });
		});
	}

	template <typename AT, typename FN>
	static void with_narrowed_at(AT const &, FN const &) { }
};


struct Dialog::Titled_frame : Widget<Frame>
{
	struct Attr { unsigned min_ex; };

	template <typename FN>
	static void view(Scope<Frame> &s, Attr const attr, FN const &fn)
	{
		s.sub_node("vbox", [&] {
			if (attr.min_ex)
				s.named_sub_node("label", "min_ex", [&] {
					s.attribute("min_ex", attr.min_ex); });
			s.sub_node("label", [&] { s.attribute("text", s.id.value); });
			s.sub_node("float", [&] {
				s.sub_node("vbox", [&] () {
					fn(); }); }); });
	}

	template <typename FN>
	static void view(Scope<Frame> &s, FN const &fn)
	{
		view(s, Attr { }, fn);
	}
};


/* local includes */
#include <view/deprecated_dialog.h>

namespace Dialog {

	template <typename FN>
	static inline void with_dummy_scope(Xml_generator &xml, FN const &fn)
	{
		static Xml_node const hover("<hover/>");
		At const no_hover(Dialog::Event::Seq_number { }, hover);
		Scope<> scope(xml, no_hover, Dialog::Event::Dragged { }, Id { });
		fn(scope);
	}
}

#endif /* _VIEW__DIALOG_H_ */
