/*
 * \brief  Runtime for hosting GUI dialogs in distant menu-view instances
 * \author Norman Feske
 * \date   2023-07-19
 */

/*
 * Copyright (C) 2023 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _DIALOG__DISTANT_RUNTIME_H_
#define _DIALOG__DISTANT_RUNTIME_H_

#include <os/reporter.h>
#include <base/attached_rom_dataspace.h>
#include <util/dictionary.h>
#include <util/color.h>
#include <dialog/types.h>

namespace Dialog { struct Distant_runtime; }


class Dialog::Distant_runtime : Noncopyable
{
	public:

		class View;

		struct Event_handler_base : Interface, Noncopyable
		{
			virtual void handle_event(Event const &event) = 0;
		};

		template <typename T> class Event_handler;

	private:

		Env &_env;

		using Views = Dictionary<View, Top_level_dialog::Name>;

		Event::Seq_number _global_seq_number { 1 };

		Views _views { };

		/* sequence numbers to correlate hover info with click/clack events */
		Constructible<Event::Seq_number> _click_seq_number { };
		Constructible<Event::Seq_number> _clack_seq_number { };

		bool _click_delivered = false; /* used to deliver each click only once */

		bool _dragged() const
		{
			return _click_seq_number.constructed()
			   && *_click_seq_number == _global_seq_number
			   &&  _click_delivered;
		}

		/* true when using a pointer device, false when using touch */
		bool _hover_observable_without_click = false;

		void _try_handle_click_and_clack();

	public:

		Distant_runtime(Env &env) : _env(env) { }

		/**
		 * Route input event to the 'Top_level_dialog' click/clack interfaces
		 */
		void route_input_event(Event::Seq_number, Input::Event const &);

		/**
		 * Respond to runtime-init state changes
		 *
		 * \return true  if the runtime-init configuration needs to be updated
		 */
		bool apply_runtime_state(Xml_node const &);

		void gen_start_nodes(Xml_generator &) const;
};


class Dialog::Distant_runtime::View : private Views::Element
{
	private:

		/* needed for privately inheriting 'Views::Element' */
		friend class Dictionary<View, Top_level_dialog::Name>;
		friend class Avl_node<View>;
		friend class Avl_tree<View>;

		friend class Distant_runtime;

		using Start_name = Session_label::String;

		Env              &_env;
		Distant_runtime  &_runtime;
		Top_level_dialog &_dialog;

		Start_name const _start_name   { _dialog.name, "_view" };
		Ram_quota  const _initial_ram  { 4*1024*1024 };
		Cap_quota  const _initial_caps { 200 };

		bool  const _opaque;
		Color const _background;

		Ram_quota _ram  = _initial_ram;
		Cap_quota _caps = _initial_caps;

		unsigned _version = 0;

		Expanding_reporter _dialog_reporter {
			_env, "dialog", { _dialog.name, "_dialog" } };

		Attached_rom_dataspace _hover_rom {
			_env, Session_label::String(_dialog.name, "_view_hover").string() };

		Signal_handler<View> _hover_handler { _env.ep(), *this, &View::_handle_hover };

		bool _dialog_hovered = false; /* used to cut hover feedback loop */

		Event::Seq_number _hover_seq_number { };

		void _with_dialog_hover(auto const &fn) const
		{
			bool done = false;

			_hover_rom.xml().with_optional_sub_node("dialog", [&] (Xml_node const &dialog) {
				fn(dialog);
				done = true; });

			if (!done)
				fn(Xml_node("<empty/>"));
		}

		void _handle_hover()
		{
			_hover_rom.update();

			Xml_node const hover = _hover_rom.xml();

			bool const orig_dialog_hovered = _dialog_hovered;

			_hover_seq_number = { hover.attribute_value("seq_number", 0U) };
			_dialog_hovered = (hover.num_sub_nodes() > 0);

			if (_runtime._dragged()) {
				_with_dialog_hover([&] (Xml_node const &hover) {
					Dragged_at at(*_runtime._click_seq_number, hover);
					_dialog.drag(at);
				});
			}

			_runtime._try_handle_click_and_clack();

			if (orig_dialog_hovered != _dialog_hovered || _dialog_hovered)
				_generate_dialog();
		}

		Signal_handler<View> _refresh_handler { _env.ep(), *this, &View::_generate_dialog };

		void _generate_dialog()
		{
			_dialog_reporter.generate([&] (Xml_generator &xml) {
				_with_dialog_hover([&] (Xml_node const &hover) {

					Event::Dragged const dragged { _runtime._dragged() };

					bool const supply_hover = _runtime._hover_observable_without_click
					                       || dragged.value;

					static Xml_node omitted_hover("<hover/>");

					At const at { _runtime._global_seq_number,
					              supply_hover ? hover : omitted_hover };

					Scope<> top_level_scope(xml, at, dragged, { _dialog.name });
					_dialog.view(top_level_scope);
				});
			});
		}

		/**
		 * Adapt runtime state information to the child
		 *
		 * This method responds to RAM and cap-resource requests by increasing
		 * the resource quotas as needed.
		 *
		 * \param  child  child node of the sandbox state report
		 * \return true if runtime must be reconfigured so that the changes
		 *         can take effect
		 */
		bool _apply_child_state_report(Xml_node const &child)
		{
			bool result = false;

			if (child.attribute_value("name", Start_name()) != _start_name)
				return false;

			if (child.has_sub_node("ram") && child.sub_node("ram").has_attribute("requested")) {
				_ram.value = min(2*_ram.value, 32*1024*1024u);
				result = true;
			}

			if (child.has_sub_node("caps") && child.sub_node("caps").has_attribute("requested")) {
				_caps.value = min(_caps.value + 100, 2000u);
				result = true;
			}

			if (child.attribute_value("skipped_heartbeats", 0U) > 2) {
				_version++;
				_ram  = _initial_ram;
				_caps = _initial_caps;
				result = true;
			}

			return result;
		}

		void _gen_start_node(Xml_generator &) const;

	public:

		unsigned min_width = 0, min_height = 0;

		struct Attr
		{
			bool      opaque;
			Color     background;
			Ram_quota initial_ram;
		};

		View(Distant_runtime &runtime, Top_level_dialog &dialog, Attr attr)
		:
			Views::Element(runtime._views, dialog.name),
			_env(runtime._env), _runtime(runtime), _dialog(dialog),
			_initial_ram(attr.initial_ram), _opaque(attr.opaque),
			_background(attr.background)
		{
			_hover_rom.sigh(_hover_handler);
			_refresh_handler.local_submit();
		}

		View(Distant_runtime &runtime, Top_level_dialog &dialog)
		:
			View(runtime, dialog, Attr { .opaque      = false,
			                             .background  = { },
			                             .initial_ram = { 4*1024*1024 } })
		{ }

		void refresh() { _refresh_handler.local_submit(); }

		bool if_hovered(auto const &fn) const
		{
			bool result = false;
			if (_dialog_hovered)
				_with_dialog_hover([&] (Xml_node const &location) {
					result = fn(Hovered_at { Event::Seq_number { }, location }); });
			return result;
		}
};

#endif /* _DIALOG__DISTANT_RUNTIME_H_ */
