/*
 * \brief  Framebuffer-to-GUI adapter
 * \author Norman Feske
 * \date   2010-09-09
 */

/*
 * Copyright (C) 2010-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <gui_session/connection.h>
#include <input/component.h>
#include <os/surface.h>
#include <input/event.h>
#include <os/static_root.h>
#include <base/attached_rom_dataspace.h>
#include <base/component.h>

namespace Gui_fb {

	using namespace Genode;

	struct View_updater;
	struct Main;

	using Point = Gui::Point;
	using Area  = Gui::Area;
	using Rect  = Gui::Rect;

	static ::Input::Event translate_event(::Input::Event, Point, Area);
}


/***************
 ** Utilities **
 ***************/

/**
 * Translate input event
 */
static Input::Event Gui_fb::translate_event(Input::Event ev,
                                            Point const input_origin,
                                            Area  const boundary)
{
	/* function to clamp point to bounday */
	auto clamp = [boundary] (Point p) {
		return Point(min((int)boundary.w - 1, max(0, p.x)),
		             min((int)boundary.h - 1, max(0, p.y))); };

	/* function to translate point to 'input_origin' */
	auto translate = [input_origin] (Point p) { return p - input_origin; };

	ev.handle_absolute_motion([&] (int x, int y) {
		Point p = clamp(translate(Point(x, y)));
		ev = Input::Absolute_motion{p.x, p.y};
	});

	ev.handle_touch([&] (Input::Touch_id id, float x, float y) {
		Point p = clamp(translate(Point((int)x, (int)y)));
		ev = Input::Touch{id, (float)p.x, (float)p.y};
	});

	return ev;
}


struct Gui_fb::View_updater : Genode::Interface
{
	virtual void update_view() = 0;
};


/*****************************
 ** Virtualized framebuffer **
 *****************************/

namespace Framebuffer {

	using namespace Gui_fb;

	struct Session_component;
}


struct Framebuffer::Session_component : Genode::Rpc_object<Framebuffer::Session>
{
	Pd_session const &_pd;

	Gui::Connection &_gui;

	Signal_context_capability _mode_sigh { };

	Signal_context_capability _sync_sigh { };

	View_updater &_view_updater;

	/*
	 * Mode as requested by the configuration or by a mode change of our
	 * GUI session.
	 */
	Framebuffer::Mode _next_mode;

	/*
	 * Number of bytes used for backing the current virtual framebuffer at
	 * the GUI server.
	 */
	size_t _buffer_num_bytes = 0;

	/*
	 * Mode that was returned to the client at the last call of
	 * 'Framebuffer:mode'. The virtual framebuffer must correspond to this
	 * mode. The variable is mutable because it is changed as a side effect of
	 * calling the const 'mode' function.
	 */
	Framebuffer::Mode mutable _active_mode = _next_mode;

	bool _dataspace_is_new = true;

	bool _ram_suffices_for_mode(Framebuffer::Mode mode) const
	{
		/* calculation in bytes */
		size_t const used      = _buffer_num_bytes,
		             needed    = Gui::Session::ram_quota(mode, false),
		             usable    = _pd.avail_ram().value,
		             preserved = 64*1024;

		return used + usable > needed + preserved;
	}


	/**
	 * Constructor
	 */
	Session_component(Pd_session const &pd,
	                  Gui::Connection &gui,
	                  View_updater &view_updater,
	                  Framebuffer::Mode initial_mode)
	:
		_pd(pd), _gui(gui), _view_updater(view_updater),
		_next_mode(initial_mode)
	{ }

	void size(Gui::Area size)
	{
		/* ignore calls that don't change the size */
		if (Gui::Area(_next_mode.area.w, _next_mode.area.h) == size)
			return;

		Framebuffer::Mode const mode { .area = size };

		if (!_ram_suffices_for_mode(mode)) {
			warning("insufficient RAM for mode ", mode);
			return;
		}

		_next_mode = mode;

		if (_mode_sigh.valid())
			Signal_transmitter(_mode_sigh).submit();
	}

	Gui::Area size() const
	{
		return _active_mode.area;
	}


	/************************************
	 ** Framebuffer::Session interface **
	 ************************************/

	Dataspace_capability dataspace() override
	{
		_gui.buffer(_active_mode, false);

		_buffer_num_bytes =
			max(_buffer_num_bytes, Gui::Session::ram_quota(_active_mode, false));

		/*
		 * We defer the update of the view until the client calls refresh the
		 * next time. This avoid showing the empty buffer as an intermediate
		 * artifact.
		 */
		_dataspace_is_new = true;

		return _gui.framebuffer.dataspace();
	}

	Mode mode() const override
	{
		_active_mode = _next_mode;
		return _active_mode;
	}

	void mode_sigh(Signal_context_capability sigh) override
	{
		_mode_sigh = sigh;
	}

	void refresh(int x, int y, int w, int h) override
	{
		if (_dataspace_is_new) {
			_view_updater.update_view();
			_dataspace_is_new = false;
		}

		_gui.framebuffer.refresh(x, y, w, h);
	}

	void sync_sigh(Signal_context_capability sigh) override
	{
		/*
		 * Keep a component-local copy of the signal capability. Otherwise,
		 * NOVA would revoke the capability from further recipients (in this
		 * case the GUI-server instance we are using) once we revoke the
		 * capability locally.
		 */
		_sync_sigh = sigh;

		_gui.framebuffer.sync_sigh(sigh);
	}
};


/******************
 ** Main program **
 ******************/

struct Gui_fb::Main : View_updater
{
	Env &_env;

	Attached_rom_dataspace _config_rom { _env, "config" };

	Gui::Connection _gui { _env };

	Point _position { 0, 0 };

	unsigned _refresh_rate = 0;

	Gui::Top_level_view const _view { _gui };

	Attached_dataspace _input_ds { _env.rm(), _gui.input.dataspace() };

	struct Initial_size
	{
		long const _width  { 0 };
		long const _height { 0 };

		bool set { false };

		Initial_size(Xml_node config)
		:
			_width (config.attribute_value("initial_width",  0L)),
			_height(config.attribute_value("initial_height", 0L))
		{ }

		unsigned width(Framebuffer::Mode const &mode) const
		{
			if (_width > 0) return (unsigned)_width;
			if (_width < 0) return (unsigned)(mode.area.w + _width);
			return mode.area.w;
		}

		unsigned height(Framebuffer::Mode const &mode) const
		{
			if (_height > 0) return (unsigned)_height;
			if (_height < 0) return (unsigned)(mode.area.h + _height);
			return mode.area.h;
		}

		bool valid() const { return _width != 0 && _height != 0; }

	} _initial_size { _config_rom.xml() };

	Framebuffer::Mode _initial_mode()
	{
		return Framebuffer::Mode { .area = { _initial_size.width (_gui.mode()),
		                                     _initial_size.height(_gui.mode()) } };
	}

	/*
	 * Input and framebuffer sessions provided to our client
	 */
	Input::Session_component       _input_session { _env, _env.ram() };
	Framebuffer::Session_component _fb_session { _env.pd(), _gui, *this, _initial_mode() };

	Static_root<Input::Session> _input_root { _env.ep().manage(_input_session) };

	/*
	 * Attach root interfaces to the entry point
	 */

	struct Fb_root : Static_root<Framebuffer::Session>
	{
		Main &_main;

		Fb_root(Main &main)
		:
			Static_root<Framebuffer::Session>(main._env.ep().manage(main._fb_session)),
			_main(main)
		{ }

		void close(Capability<Session>) override
		{
			_main._fb_session.sync_sigh(Signal_context_capability());
			_main._fb_session.mode_sigh(Signal_context_capability());
		}
	};

	Fb_root _fb_root { *this };

	/**
	 * View_updater interface
	 */
	void update_view() override
	{
		using Command = Gui::Session::Command;
		_gui.enqueue<Command::Geometry>(_view.id(), Rect(_position, _fb_session.size()));
		_gui.enqueue<Command::Front>(_view.id());
		_gui.execute();
	}

	/**
	 * Return screen-coordinate origin, depening on the config and screen mode
	 */
	static Point _coordinate_origin(Framebuffer::Mode mode, Xml_node config)
	{
		char const * const attr = "origin";

		if (!config.has_attribute(attr))
			return Point(0, 0);

		using Value = String<32>;
		Value const value = config.attribute_value(attr, Value());

		if (value == "top_left")     return Point(0, 0);
		if (value == "top_right")    return Point(mode.area.w, 0);
		if (value == "bottom_left")  return Point(0, mode.area.h);
		if (value == "bottom_right") return Point(mode.area.w, mode.area.h);

		warning("unsupported ", attr, " attribute value '", value, "'");
		return Point(0, 0);
	}

	void _update_size()
	{
		Xml_node const config = _config_rom.xml();

		Framebuffer::Mode const gui_mode = _gui.mode();

		_position = _coordinate_origin(gui_mode, config) + Point::from_xml(config);

		bool const attr = config.has_attribute("width") ||
		                  config.has_attribute("height");
		if (_initial_size.valid() && attr) {
			warning("setting both inital and normal attributes not "
			        " supported, ignore initial size");
			/* force initial to disable check below */
			_initial_size.set = true;
		}

		unsigned const gui_width  = gui_mode.area.w;
		unsigned const gui_height = gui_mode.area.h;

		long width  = config.attribute_value("width",  (long)gui_mode.area.w),
		     height = config.attribute_value("height", (long)gui_mode.area.h);

		if (!_initial_size.set && _initial_size.valid()) {
			width  = _initial_size.width (gui_mode);
			height = _initial_size.height(gui_mode);

			_initial_size.set = true;
		} else {

			/*
			 * If configured width / height values are negative, the effective
			 * width / height is deduced from the screen size.
			 */
			if (width  < 0) width  = gui_width  + width;
			if (height < 0) height = gui_height + height;
		}

		_fb_session.size(Area((unsigned)width, (unsigned)height));
	}

	void _handle_config_update()
	{
		_config_rom.update();

		_update_size();

		update_view();
	}

	Signal_handler<Main> _config_update_handler =
		{ _env.ep(), *this, &Main::_handle_config_update };

	void _handle_mode_update() { _update_size(); }

	Signal_handler<Main> _mode_update_handler =
		{ _env.ep(), *this, &Main::_handle_mode_update };

	void _handle_input()
	{
		Input::Event const * const events = _input_ds.local_addr<Input::Event>();

		unsigned const num = _gui.input.flush();
		bool update = false;

		for (unsigned i = 0; i < num; i++) {
			update |= events[i].focus_enter();
			_input_session.submit(translate_event(events[i], _position, _fb_session.size()));
		}

		/* get to front if we got input focus */
		if (update)
			update_view();
	}

	Signal_handler<Main> _input_handler { _env.ep(), *this, &Main::_handle_input };

	/**
	 * Constructor
	 */
	Main(Env &env) : _env(env)
	{
		_input_session.event_queue().enabled(true);

		/*
		 * Announce services
		 */
		_env.parent().announce(_env.ep().manage(_fb_root));
		_env.parent().announce(_env.ep().manage(_input_root));

		/*
		 * Apply initial configuration
		 */
		_handle_config_update();

		/*
		 * Register signal handlers
		 */
		_config_rom.sigh(_config_update_handler);
		_gui.mode_sigh(_mode_update_handler);
		_gui.input.sigh(_input_handler);
	}
};


void Component::construct(Genode::Env &env) { static Gui_fb::Main inst(env); }

