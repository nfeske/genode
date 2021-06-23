/*
 * \brief  Window layouter
 * \author Norman Feske
 * \date   2013-02-14
 */

/*
 * Copyright (C) 2013-2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/log.h>
#include <base/signal.h>
#include <base/component.h>
#include <base/attached_rom_dataspace.h>
#include <base/heap.h>
#include <os/reporter.h>
#include <os/session_policy.h>
#include <gui_session/connection.h>
#include <input_session/client.h>
#include <input/event.h>
#include <input/keycodes.h>
#include <timer_session/connection.h>

/* local includes */
#include <window_list.h>
#include <assign_list.h>
#include <target_list.h>
#include <user_state.h>
#include <operations.h>

namespace Window_layouter { struct Main; }


struct Window_layouter::Main : Operations,
                               Layout_rules::Change_handler,
                               Window_list::Change_handler
{
	Env &_env;

	Attached_rom_dataspace _config { _env, "config" };

	Signal_handler<Main> _config_handler {
		_env.ep(), *this, &Main::_handle_config };

	Timer::Connection _drop_timer { _env };

	enum class Drag_state { IDLE, DRAGGING, SETTLING };

	Drag_state _drag_state { Drag_state::IDLE };

	Signal_handler<Main> _drop_timer_handler {
		_env.ep(), *this, &Main::_handle_drop_timer };

	Heap _heap { _env.ram(), _env.rm() };

	Area _screen_size { };

	unsigned _to_front_cnt = 1;

	Focus_history _focus_history { };

	Layout_rules _layout_rules { _env, _heap, *this };

	Decorator_margins _decorator_margins { Xml_node("<floating/>") };

	Window_list _window_list {
		_env, _heap, *this, _focus_history, _decorator_margins };

	Assign_list _assign_list { _heap };

	Target_list _target_list { _heap };

	/**
	 * Bring window to front, return true if the stacking order changed
	 */
	bool _to_front(Window &window)
	{
		bool stacking_order_changed = false;

		if (window.to_front_cnt() != _to_front_cnt) {
			_to_front_cnt++;
			window.to_front_cnt(_to_front_cnt);
			stacking_order_changed = true;
		};

		return stacking_order_changed;
	}

	void _update_window_layout()
	{
		_window_list.dissolve_windows_from_assignments();

		_layout_rules.with_rules([&] (Xml_node rules) {
			_assign_list.update_from_xml(rules);
			_target_list.update_from_xml(rules, _screen_size);
		});

		_assign_list.assign_windows(_window_list);

		/* position windows */
		_assign_list.for_each([&] (Assign &assign) {
			_target_list.for_each([&] (Target const &target) {

				if (target.name() != assign.target_name())
					return;

				assign.for_each_member([&] (Assign::Member &member) {

					member.window.floating(assign.floating());
					member.window.target_geometry(target.geometry());

					Rect const rect = assign.window_geometry(member.window.id().value,
					                                         member.window.client_size(),
					                                         target.geometry(),
					                                         _decorator_margins);
					member.window.outer_geometry(rect);
					member.window.maximized(assign.maximized());
				});
			});
		});

		/* bring new windows that solely match a wildcard assignment to the front */
		_assign_list.for_each_wildcard_assigned_window([&] (Window &window) {
			_to_front(window); });

		_gen_window_layout();

		if (_assign_list.matching_wildcards())
			_gen_rules();

		_gen_resize_request();
	}

	/**
	 * Layout_rules::Change_handler interface
	 */
	void layout_rules_changed() override
	{
		_update_window_layout();

		_gen_resize_request();
	}

	/**
	 * Window_list::Change_handler interface
	 */
	void window_list_changed() override
	{
		/*
		 * Discharge the enforced top-most position of the most recently
		 * manually topped window (see '_gen_rules_with_frontmost_screen').
		 * If a window re-appears in front of the currently top-most window,
		 * the re-appeared window should stay on top.
		 *
		 * https://github.com/genodelabs/genode/issues/4195
		 */
		_to_front_cnt++;

		_update_window_layout();
	}

	void _handle_config()
	{
		_config.update();

		if (_config.xml().has_sub_node("report")) {
			Xml_node const report = _config.xml().sub_node("report");
			_rules_reporter.conditional(report.attribute_value("rules", false),
			                            _env, "rules", "rules");
		}

		_layout_rules.update_config(_config.xml());
	}

	User_state _user_state { *this, _focus_history };


	/**************************
	 ** Operations interface **
	 **************************/

	void close(Window_id id) override
	{
		_window_list.with_window(id, [&] (Window &window) {
			window.close(); });

		_gen_resize_request();
		_gen_focus();
	}

	void to_front(Window_id id) override
	{
		bool stacking_order_changed = false;

		_window_list.with_window(id, [&] (Window &window) {
			stacking_order_changed = _to_front(window); });

		if (stacking_order_changed)
			_gen_rules();
	}

	void focus(Window_id) override
	{
		_gen_window_layout();
		_gen_focus();
	}

	void toggle_fullscreen(Window_id id) override
	{
		/* make sure that the specified window is the front-most one */
		to_front(id);

		_window_list.with_window(id, [&] (Window &window) {
			window.maximized(!window.maximized()); });

		_gen_rules();
		_gen_resize_request();
	}

	void screen(Target::Name const & name) override
	{
		_gen_rules_with_frontmost_screen(name);
	}

	void drag(Window_id id, Window::Element element, Point clicked, Point curr) override
	{
		if (_drag_state == Drag_state::SETTLING)
			return;

		_drag_state = Drag_state::DRAGGING;

		to_front(id);

		bool window_layout_changed = false;

		_window_list.with_window(id, [&] (Window &window) {

			bool const orig_dragged  = window.dragged();
			Rect const orig_geometry = window.effective_inner_geometry();
			window.drag(element, clicked, curr);
			bool const next_dragged  = window.dragged();
			Rect const next_geometry = window.effective_inner_geometry();

			window_layout_changed = orig_geometry.p1() != next_geometry.p1()
			                     || orig_geometry.p2() != next_geometry.p2()
			                     || orig_dragged       != next_dragged;
		});

		if (window_layout_changed)
			_gen_window_layout();

		_gen_resize_request();
	}

	void _handle_drop_timer()
	{
		_drag_state = Drag_state::IDLE;

		_gen_rules();

		_window_list.for_each_window([&] (Window &window) {
			window.finalize_drag_operation(); });
	}

	void finalize_drag(Window_id, Window::Element, Point, Point) override
	{
		/*
		 * Update window layout because highlighting may have changed after the
		 * drag operation. E.g., if the window has not kept up with the
		 * dragging of a resize handle, the resize handle is no longer hovered.
		 *
		 * The call of '_handle_hover' implicitly triggers '_gen_window_layout'.
		 */
		_handle_hover();

		_drag_state = Drag_state::SETTLING;

		_drop_timer.trigger_once(250*1000);
	}

	/**
	 * Install handler for responding to hover changes
	 */
	void _handle_hover();

	Signal_handler<Main> _hover_handler {
		_env.ep(), *this, &Main::_handle_hover};

	Attached_rom_dataspace _hover { _env, "hover" };


	void _handle_focus_request();

	Signal_handler<Main> _focus_request_handler {
		_env.ep(), *this, &Main::_handle_focus_request };

	Attached_rom_dataspace _focus_request { _env, "focus_request" };

	int _handled_focus_request_id = 0;


	/**
	 * Respond to decorator-margins information reported by the decorator
	 */
	Attached_rom_dataspace _decorator_margins_rom { _env, "decorator_margins" };

	void _handle_decorator_margins()
	{
		_decorator_margins_rom.update();

		Xml_node const margins = _decorator_margins_rom.xml();
		if (margins.has_sub_node("floating"))
			_decorator_margins = Decorator_margins(margins.sub_node("floating"));

		/* respond to change by adapting the maximized window geometry */
		_handle_mode_change();
	}

	Signal_handler<Main> _decorator_margins_handler {
		_env.ep(), *this, &Main::_handle_decorator_margins};

	void _handle_input()
	{
		while (_input.pending())
			_user_state.handle_input(_input_ds.local_addr<Input::Event>(),
			                         _input.flush(), _config.xml());
	}

	Signal_handler<Main> _input_handler {
		_env.ep(), *this, &Main::_handle_input };

	Gui::Connection _gui { _env };

	void _handle_mode_change()
	{
		/* determine maximized window geometry */
		Framebuffer::Mode const mode = _gui.mode();

		_screen_size = mode.area;

		_update_window_layout();
	}
	
	Signal_handler<Main> _mode_change_handler {
		_env.ep(), *this, &Main::_handle_mode_change };


	Input::Session_client _input { _env.rm(), _gui.input_session() };

	Attached_dataspace _input_ds { _env.rm(), _input.dataspace() };

	Expanding_reporter _window_layout_reporter  { _env, "window_layout",  "window_layout"};
	Expanding_reporter _resize_request_reporter { _env, "resize_request", "resize_request"};
	Expanding_reporter _focus_reporter          { _env, "focus",          "focus" };

	Constructible<Expanding_reporter> _rules_reporter { };

	bool _focused_window_maximized() const
	{
		bool result = false;
		Window_id const id = _user_state.focused_window_id();
		_window_list.with_window(id, [&] (Window const &window) {
			result = window.maximized(); });
		return result;
	}

	void _import_window_list(Xml_node);
	void _gen_window_layout();
	void _gen_resize_request();
	void _gen_focus();
	void _gen_rules_with_frontmost_screen(Target::Name const &);

	void _gen_rules()
	{
		/* keep order of screens unmodified */
		_gen_rules_with_frontmost_screen(Target::Name());
	}

	void _gen_rules_assignments(Xml_generator &) const;

	/**
	 * Constructor
	 */
	Main(Env &env) : _env(env)
	{
		_gui.mode_sigh(_mode_change_handler);
		_handle_mode_change();

		_drop_timer.sigh(_drop_timer_handler);

		_hover.sigh(_hover_handler);
		_decorator_margins_rom.sigh(_decorator_margins_handler);
		_input.sigh(_input_handler);
		_focus_request.sigh(_focus_request_handler);

		_window_list.initial_import();
		_handle_focus_request();

		/* attach update handler for config */
		_config.sigh(_config_handler);
		_handle_config();

		/* reflect initial rules configuration */
		_gen_rules();
	}
};


void Window_layouter::Main::_gen_window_layout()
{
	/* update hover and focus state of each window */
	_window_list.for_each_window([&] (Window &window) {

		window.focused(window.has_id(_user_state.focused_window_id()));

		bool const hovered = window.has_id(_user_state.hover_state().window_id);
		window.hovered(hovered ? _user_state.hover_state().element
		                       : Window::Element::UNDEFINED);
	});

	_window_layout_reporter.generate([&] (Xml_generator &xml) {
		_target_list.gen_layout(xml, _assign_list); });
}


void Window_layouter::Main::_gen_resize_request()
{
	bool resize_needed = false;
	_assign_list.for_each([&] (Assign const &assign) {
		assign.for_each_member([&] (Assign::Member const &member) {
			if (member.window.resize_request_needed())
				resize_needed = true; }); });

	if (!resize_needed)
		return;

	_resize_request_reporter.generate([&] (Xml_generator &xml) {
		_window_list.for_each_window([&] (Window const &window) {
			window.gen_resize_request(xml); }); });

	/* prevent superfluous resize requests for the same size */
	_window_list.for_each_window([&] (Window &window) {
		window.resize_request_updated(); });
}


void Window_layouter::Main::_gen_focus()
{
	_focus_reporter.generate([&] (Xml_generator &xml) {
		xml.node("window", [&] () {
			xml.attribute("id", _user_state.focused_window_id().value); }); });
}


void Window_layouter::Main::_gen_rules_assignments(Xml_generator &xml) const
{
	auto gen_window_geometry = [] (Xml_generator &xml,
	                               Assign const &assign, Window const &window) {
		if (!assign.floating())
			return;

		assign.gen_geometry_attr(xml, { .geometry  = window.effective_inner_geometry(),
		                                .maximized = window.maximized() });
	};

	auto gen_window_assign_node = [&] (Xml_generator &xml, Assign const &assign)
	{
		if (assign.wildcard())
			return;

		xml.node("assign", [&] () {

			assign.gen_assign_attr(xml);

			/*
			 * Determine current geometry of window. If multiple windows
			 * are present with the same label, use the geometry of any of
			 * them as they cannot be distinguished based on their label.
			 */
			bool geometry_generated = false;
			assign.for_each_member([&] (Assign::Member const &member) {

				if (geometry_generated)
					return;

				gen_window_geometry(xml, assign, member.window);
				geometry_generated = true;
			});

			/* assign node with no window */
			if (!geometry_generated && assign.floating())
				assign.gen_geometry_attr(xml);
		});
	};

	auto gen_wildcard_assign_node = [&] (Xml_generator &xml, Assign const &assign)
	{
		if (!assign.wildcard())
			return;

		xml.node("assign", [&] () {
			assign.gen_assign_attr(xml);
			assign.gen_geometry_attr(xml);
		});
	};

	auto used_by_present_window = [] (Assign const &assign)
	{
		bool used = false;
		assign.for_each_member([&] (Assign::Member const &) {
			used = true; });
		return used;
	};

	auto used_by_topped_window = [&] (Assign const &assign)
	{
		bool topped = false;
		assign.for_each_member([&] (Assign::Member const &member) {
			if (member.window.to_front_cnt() == _to_front_cnt)
				topped = true; });
		return topped;
	};

	/*
	 * Generate all unused assign nodes in front of the first used assign node.
	 * Those assignments belong to disappeared windows such as confirmed alert
	 * boxes that should be placed in front once they re-appear.
	 */
	{
		bool before_first_used_assign = true;
		_assign_list.for_each([&] (Assign const &assign) {

			if (used_by_present_window(assign))
				before_first_used_assign = false;

			if (before_first_used_assign)
				gen_window_assign_node(xml, assign);
		});
	}

	/*
	 * Turn wildcard assignments into exact assignments. This covers new
	 * appearing windows, which are expected to be placed in front.
	 */
	auto fn = [&] (Assign const &assign, Assign::Member const &member) {
		xml.node("assign", [&] () {
			xml.attribute("label",  member.window.label());
			xml.attribute("target", assign.target_name());
			gen_window_geometry(xml, assign, member.window);
		});
	};
	_assign_list.for_each_wildcard_member(fn);

	/*
	 * Generate assign node for the most recently interactively topped window.
	 */
	_assign_list.for_each([&] (Assign const &assign) {
		if (used_by_topped_window(assign))
			gen_window_assign_node(xml, assign); });

	/*
	 * Generate all remaining assign nodes, in particular for all present
	 * windows other than the topped window.
	 */
	{
		bool before_first_used_assign = true;
		_assign_list.for_each([&] (Assign const &assign) {

			if (used_by_present_window(assign))
				before_first_used_assign = false;

			if (before_first_used_assign)
				return;

			if (used_by_topped_window(assign))
				return;

			gen_window_assign_node(xml, assign);
		});
	}

	/*
	 * Preserve wildcard nodes. The wildcards come last because they should
	 * be considered only when no other concrete assign node matches.
	 */
	_assign_list.for_each([&] (Assign const &assign) {
		if (assign.wildcard())
			gen_wildcard_assign_node(xml, assign); });
}


void Window_layouter::Main::_gen_rules_with_frontmost_screen(Target::Name const &screen)
{
	if (!_rules_reporter.constructed())
		return;

	_rules_reporter->generate([&] (Xml_generator &xml) {
		_target_list.gen_screens(xml, screen);
		_gen_rules_assignments(xml);
	});
}


/**
 * Determine window element that corresponds to hover model
 */
static Window_layouter::Window::Element
_element_from_hover_model(Genode::Xml_node hover_window_xml)
{
	typedef Window_layouter::Window::Element::Type Type;

	bool const left_sizer   = hover_window_xml.has_sub_node("left_sizer"),
	           right_sizer  = hover_window_xml.has_sub_node("right_sizer"),
	           top_sizer    = hover_window_xml.has_sub_node("top_sizer"),
	           bottom_sizer = hover_window_xml.has_sub_node("bottom_sizer");

	if (left_sizer && top_sizer)    return Type::TOP_LEFT;
	if (left_sizer && bottom_sizer) return Type::BOTTOM_LEFT;
	if (left_sizer)                 return Type::LEFT;

	if (right_sizer && top_sizer)    return Type::TOP_RIGHT;
	if (right_sizer && bottom_sizer) return Type::BOTTOM_RIGHT;
	if (right_sizer)                 return Type::RIGHT;

	if (top_sizer)    return Type::TOP;
	if (bottom_sizer) return Type::BOTTOM;

	if (hover_window_xml.has_sub_node("title"))     return Type::TITLE;
	if (hover_window_xml.has_sub_node("closer"))    return Type::CLOSER;
	if (hover_window_xml.has_sub_node("maximizer")) return Type::MAXIMIZER;
	if (hover_window_xml.has_sub_node("minimizer")) return Type::MINIMIZER;

	return Type::UNDEFINED;
}


void Window_layouter::Main::_handle_hover()
{
	_hover.update();

	User_state::Hover_state const orig_hover_state = _user_state.hover_state();

	try {
		Xml_node const hover_window_xml = _hover.xml().sub_node("window");

		_user_state.hover(hover_window_xml.attribute_value("id", 0UL),
		                  _element_from_hover_model(hover_window_xml));
	}

	/*
	 * An exception may occur during the 'Xml_node' construction if the hover
	 * model lacks a window. Under this condition, we invalidate the hover
	 * state.
	 */
	catch (...) {

		_user_state.reset_hover();

		/*
		 * Don't generate a focus-model update here. In a situation where the
		 * pointer has moved over a native GUI view (outside the realm of
		 * the window manager), the hover model as generated by the decorator
		 * naturally becomes empty. If we posted a focus update, this would
		 * steal the focus away from the native GUI view.
		 */
	}

	/*
	 * Propagate changed hovering to the decorator
	 *
	 * Should a decorator issue a hover update with the unchanged information,
	 * avoid triggering a superfluous window-layout update. This can happen in
	 * corner cases such as the completion of a window-drag operation.
	 */
	if (_user_state.hover_state() != orig_hover_state)
		_gen_window_layout();
}


void Window_layouter::Main::_handle_focus_request()
{
	_focus_request.update();

	int const id = _focus_request.xml().attribute_value("id", 0L);

	/* don't apply the same focus request twice */
	if (id == _handled_focus_request_id)
		return;

	_handled_focus_request_id = id;

	Window::Label const prefix =
		_focus_request.xml().attribute_value("label", Window::Label(""));

	unsigned const next_to_front_cnt = _to_front_cnt + 1;

	bool stacking_order_changed = false;

	auto label_matches = [] (Window::Label const &prefix, Window::Label const &label) {
		return !strcmp(label.string(), prefix.string(), prefix.length() - 1); };

	_window_list.for_each_window([&] (Window &window) {

		if (label_matches(prefix, window.label())) {
			window.to_front_cnt(next_to_front_cnt);
			_user_state.focused_window_id(window.id());
			stacking_order_changed = true;
		}
	});

	if (stacking_order_changed) {
		_to_front_cnt++;
		_gen_focus();
		_gen_rules();
	}
}


void Component::construct(Genode::Env &env)
{
	static Window_layouter::Main application(env);
}
