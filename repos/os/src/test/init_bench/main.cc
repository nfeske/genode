/*
 * \brief  Benchmark for the init component
 * \author Norman Feske
 * \date   2019-04-17
 */

/*
 * Copyright (C) 2019 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <base/heap.h>
#include <base/component.h>
#include <base/session_label.h>
#include <base/attached_rom_dataspace.h>
#include <timer_session/connection.h>
#include <log_session/log_session.h>
#include <root/component.h>
#include <os/reporter.h>
#include <base/sleep.h>

namespace Test {

	struct Log_message_handler;
	class  Log_session_component;
	class  Log_root;
	struct Stopwatch;
	struct Startup_benchmark;
	struct Main;

	using namespace Genode;
}


struct Test::Log_message_handler : Interface
{
	typedef String<Log_session::MAX_STRING_LEN> Message;

	virtual void handle_log_message(Session_label const &label,
	                                Message const &message) = 0;
};


class Test::Log_session_component : public Rpc_object<Log_session>
{
	private:

		Session_label const _label;

		Log_message_handler &_handler;

	public:

		Log_session_component(Session_label const &label, Log_message_handler &handler)
		:
			_label(label), _handler(handler)
		{ }

		size_t write(String const &string) override
		{
			/* strip known line delimiter from incoming message */
			unsigned n = 0;
			Genode::String<16> const pattern("\033[0m\n");
			for (char const *s = string.string(); s[n] && pattern != s + n; n++);

			Log_message_handler::Message const
				message(Cstring(string.string(), n));

			_handler.handle_log_message(_label, message);

			return strlen(string.string());
		}
};


class Test::Log_root : public Root_component<Log_session_component>
{
	private:

		Log_message_handler &_handler;

	public:

		Log_root(Entrypoint &ep, Allocator &md_alloc, Log_message_handler &handler)
		:
			Root_component(ep, md_alloc), _handler(handler)
		{ }

		Log_session_component *_create_session(const char *args, Affinity const &) override
		{
			Session_label const label = label_from_args(args);

			return new (md_alloc()) Log_session_component(label, _handler);
		}
};


struct Test::Stopwatch
{
	struct Timeout_handler : Interface { virtual void handle_timeout() = 0; };

	Timer::Connection _timer;

	Timeout_handler &_timeout_handler;

	Microseconds _sum        { 0 }; /* accumulated measurements */
	Microseconds _curr_start { 0 }; /* start time of current measurement */

	bool _active = false;

	Timer::One_shot_timeout<Stopwatch> _one_shot_handler;

	void _handle_one_shot(Duration) { _timeout_handler.handle_timeout(); }

	void start()
	{
		if (_active)
			warning("start called twice");

		_active = true;
		_curr_start = _timer.curr_time().trunc_to_plain_us();
	}

	void stop()
	{
		if (!_active)
			warning("stop called without start");

		Microseconds const curr_stop = _timer.curr_time().trunc_to_plain_us();
		_sum.value += curr_stop.value - _curr_start.value;
		_active = false;
	}

	Microseconds elapsed() { return _sum; }

	Stopwatch(Env &env, Duration duration, Timeout_handler &timeout_handler)
	:
		_timer(env),
		_timeout_handler(timeout_handler),
		_one_shot_handler(_timer, *this, &Stopwatch::_handle_one_shot)
	{
		_one_shot_handler.schedule(duration.trunc_to_plain_us());
	}
};


struct Test::Startup_benchmark
{
	unsigned const _num_children;
	unsigned const _inflated_routes = 0;

	Stopwatch &_stopwatch;

	enum class State { START, KILL };

	State _state { State::START };

	unsigned _received_log_messages = 0;

	unsigned finished_iterations = 0;

	Startup_benchmark(Xml_node step, Stopwatch &stopwatch)
	:
		_num_children(step.attribute_value("children", 1U)),
		_inflated_routes(step.attribute_value("inflated_routes", 0U)),
		_stopwatch(stopwatch)
	{
		log("--- startup benchmark: children=", _num_children, " ---");
	}

	static void gen_common_init_config(Xml_generator &xml)
	{
		xml.node("report", [&] () {
			xml.attribute("buffer", "64K"); });

		xml.node("parent-provides", [&] () {

			auto service = [&] (char const *name) {
				xml.node("service", [&] () {
					xml.attribute("name", name); }); };

			service("PD");
			service("CPU");
			service("LOG");
			service("ROM");
		});

		xml.node("default-route", [&] () {
			xml.node("any-service", [&] () {
				xml.node("parent", [&] () {}); }); });
	}

	void gen_init_config(Xml_generator &xml)
	{
		gen_common_init_config(xml);

		if (_state == State::START) {
			for (unsigned i = 0; i < _num_children; i++) {
				xml.node("start", [&] () {
					xml.attribute("name", i);
					xml.attribute("caps", 100);
					xml.node("resource", [&] () {
						xml.attribute("name", "RAM");
						xml.attribute("quantum", "1M");
					});
					xml.node("binary", [&] () {
						xml.attribute("name", "dummy");
					});
					xml.node("config", [&] () {
						xml.node("log", [&] () {
							xml.attribute("string", "started"); }); });

					if (_inflated_routes) {
						xml.node("route", [&] () {

							auto parent_route = [&] (char const *service) {
								xml.node("service", [&] () {
									xml.attribute("name", service);
									xml.node("parent", [&] () { });
								});
							};

							parent_route("PD");
							parent_route("CPU");
							parent_route("RM");
							parent_route("LOG");

							auto parent_rom_route = [&] (char const *label) {
								xml.node("service", [&] () {
									xml.attribute("name", "ROM");
									xml.attribute("label_last", label);
									xml.node("parent", [&] () { });
								});
							};

							for (unsigned i = 0; i < _inflated_routes; i++)
								parent_rom_route("inflated");

							parent_rom_route("ld.lib.so");
							parent_rom_route("dummy");
						});
					}
				});
			}
		}
	}

	enum class Result { OK, RECONFIGURE_INIT, ITERATION_COMPLETE };

	Result handle_log_message(Session_label const &,
	                          Log_message_handler::Message const &)
	{
		_received_log_messages++;
		if (_received_log_messages == _num_children) {
			_state = State::KILL;
			_stopwatch.stop(); /* don't measure kill time */
			return Result::RECONFIGURE_INIT;
		}
		return Result::OK;
	}

	Result handle_init_state(Xml_node state)
	{
		bool const children_exist = state.has_sub_node("child");

		if (_state == State::KILL && !children_exist) {
			_state = State::START;
			_received_log_messages = 0;
			finished_iterations++;
			_stopwatch.start();
			return Result::ITERATION_COMPLETE;
		}

		return Result::OK;
	}

	void print_stats() const
	{
		Microseconds avr { _stopwatch.elapsed().value
		                 / (finished_iterations*_num_children) };

		log("iterations: ", finished_iterations);
		log("consumed:   ", _stopwatch.elapsed());
		log("average:    ", avr);
	}
};


struct Test::Main : Log_message_handler, Stopwatch::Timeout_handler
{
	Env &_env;

	Constructible<Stopwatch> _stopwatch { };

	Expanding_reporter _init_config { _env, "config",  "init_config" };

	Attached_rom_dataspace _config { _env, "config" };

	unsigned const _num_steps = _config.xml().num_sub_nodes();
	unsigned       _curr_step = 0;

	bool _time_is_up = false;

	template <typename FN>
	void _with_curr_step(FN const &fn)
	{
		fn(_config.xml().sub_node(_curr_step));
	}

	Attached_rom_dataspace _init_state { _env, "init_state" };

	Signal_handler<Main> _init_state_handler {
		_env.ep(), *this, &Main::_handle_init_state };

	void _handle_benchmark_result(Startup_benchmark::Result result)
	{
		bool const new_iteration =
			(result == Startup_benchmark::Result::RECONFIGURE_INIT) ||
			(result == Startup_benchmark::Result::ITERATION_COMPLETE && !_time_is_up);

		if (new_iteration) {
			_init_config.generate([&] (Xml_generator &xml) {
				_startup_benchmark->gen_init_config(xml); });
		}

		if (result == Startup_benchmark::Result::ITERATION_COMPLETE && _time_is_up) {
			_startup_benchmark->print_stats();
			_startup_benchmark.destruct();
			_advance_step();
		}
	}

	void _handle_init_state()
	{
		_init_state.update();

		if (_startup_benchmark.constructed()) {

			Startup_benchmark::Result const result =
				_startup_benchmark->handle_init_state(_init_state.xml());

			_handle_benchmark_result(result);
		}
	}

	void _advance_step()
	{
		_curr_step++;

		/* exit when reaching the end of the sequence */
		if (_curr_step == _num_steps) {
			_env.parent().exit(0);
			sleep_forever();
		}

		_enter_curr_step();
	};

	Constructible<Startup_benchmark> _startup_benchmark { };

	void _enter_startup_benchmark(Xml_node step)
	{
		Milliseconds const ms { step.attribute_value("seconds", 1U)*1000 };

		_stopwatch.construct(_env, Duration(ms), *this);

		_startup_benchmark.construct(step, *_stopwatch);

		_init_config.generate([&] (Xml_generator &xml) {
			_startup_benchmark->gen_init_config(xml); });

		_stopwatch->start();
	}

	void _enter_curr_step()
	{
		_time_is_up = false;
		_with_curr_step([&] (Xml_node step) {

			if (step.has_type("startup"))
				_enter_startup_benchmark(step);
		});
	}

	/**
	 * Log_message_handler interface
	 */
	void handle_log_message(Session_label const &label,
	                        Log_message_handler::Message const &message) override
	{
		if (_startup_benchmark.constructed()) {
			Startup_benchmark::Result const result =
				_startup_benchmark->handle_log_message(label, message);

			_handle_benchmark_result(result);
		}
	}

	/**
	 * Stopwatch::Timeout_handler interface
	 */
	void handle_timeout() override
	{
		/* cancel benchmark when completing the current iteration */
		if (_startup_benchmark.constructed() && _stopwatch.constructed())
			_time_is_up = true;
	}

	Sliced_heap _sliced_heap { _env.ram(), _env.rm() };

	Log_root _log_root { _env.ep(), _sliced_heap, *this };

	Main(Env &env) : _env(env)
	{
		_init_state.sigh(_init_state_handler);
		_enter_curr_step();

		_env.parent().announce(_env.ep().manage(_log_root));
	}
};


void Component::construct(Genode::Env &env) { static Test::Main main(env); }

