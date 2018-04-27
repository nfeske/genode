/*
 * \brief  Nitpicker wrapper for monitoring the user input of GUI components
 * \author Norman Feske
 * \date   2018-04-30
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _NITPICKER_H_
#define _NITPICKER_H_

/* Genode includes */
#include <root/component.h>
#include <input/component.h>
#include <nitpicker_session/connection.h>
#include <base/component.h>
#include <base/heap.h>

/* local includes */
#include "input_event_handler.h"

namespace Nitpicker {

	using namespace Genode;
	using Sculpt_manager::Input_event_handler;

	struct Session_component;
	struct Root;
}


struct Nitpicker::Session_component : Rpc_object<Nitpicker::Session>
{
	Env &_env;

	Input_event_handler &_event_handler;

	Nitpicker::Connection _connection;

	Input::Session_component _input_component { _env, _env.ram() };

	Signal_handler<Session_component> _input_handler {
		_env.ep(), *this, &Session_component::_handle_input };

	void _handle_input()
	{
		_connection.input()->for_each_event([&] (Input::Event ev) {

		 	/* handle event locally within the sculpt manager */
			_event_handler.handle_input_event(ev);

			/* pass pointer location to dynamic GUI components */
			if (ev.absolute_motion() || ev.hover_leave())
				_input_component.submit(ev);
		});
	}

	Session_component(Env &env, char const *args, Input_event_handler &event_handler)
	:
		_env(env), _event_handler(event_handler),
		_connection(env, session_label_from_args(args).string())
	{
		_connection.input()->sigh(_input_handler);
		_env.ep().manage(_input_component);
		_input_component.event_queue().enabled(true);
	}

	~Session_component() { _env.ep().dissolve(_input_component); }

	void upgrade(Session::Resources const &resources)
	{
		_connection.upgrade(resources);
	}

	Framebuffer::Session_capability framebuffer_session() override {
		return _connection.framebuffer_session(); }

	Input::Session_capability input_session() override {
		return _input_component.cap(); }

	View_handle create_view(View_handle parent) override {
		return _connection.create_view(parent); }

	void destroy_view(View_handle view) override {
		_connection.destroy_view(view); }

	View_handle view_handle(View_capability view_cap, View_handle handle) override {
		return _connection.view_handle(view_cap, handle); }

	View_capability view_capability(View_handle view) override {
		return _connection.view_capability(view); }

	void release_view_handle(View_handle view) override {
		_connection.release_view_handle(view); }

	Dataspace_capability command_dataspace() override {
		return _connection.command_dataspace(); }

	void execute() override {
		_connection.execute(); }

	Framebuffer::Mode mode() override {
		return _connection.mode(); }

	void mode_sigh(Signal_context_capability sigh) override {
		_connection.mode_sigh(sigh); }

	void buffer(Framebuffer::Mode mode, bool use_alpha) override {
		_connection.buffer(mode, use_alpha); }

	void focus(Capability<Nitpicker::Session> session) override {
		_connection.focus(session); }
};


struct Nitpicker::Root : Root_component<Session_component>
{
	Env &_env;

	Input_event_handler &_event_handler;

	Session_component *_create_session(const char *args) override
	{
		return new (md_alloc()) Session_component(_env, args, _event_handler);
	}

	void _upgrade_session(Session_component *session, const char *args) override
	{
		session->upgrade(session_resources_from_args(args));
	}

	void _destroy_session(Session_component *session) override
	{
		Genode::destroy(md_alloc(), session);
	}

	Root(Env &env, Allocator &md_alloc, Input_event_handler &event_handler)
	:
		Root_component<Session_component>(env.ep(), md_alloc),
		_env(env), _event_handler(event_handler)
	{
		env.parent().announce(env.ep().manage(*this));
	}

	~Root() { _env.ep().dissolve(*this); }
};

#endif /* _NITPICKER_H_ */
