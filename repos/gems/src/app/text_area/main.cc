/*
 * \brief  Simple text viewer and editor
 * \author Norman Feske
 * \date   2020-01-12
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/component.h>
#include <base/session_object.h>
#include <base/attached_rom_dataspace.h>
#include <base/buffered_output.h>
#include <os/buffered_xml.h>
#include <os/sandbox.h>
#include <os/dynamic_rom_session.h>
#include <os/vfs.h>
#include <os/reporter.h>

/* local includes */
#include <nitpicker.h>
#include <report.h>
#include <dialog.h>
#include <new_file.h>

namespace Text_view { struct Main; }

struct Text_view::Main : Sandbox::Local_service_base::Wakeup,
                         Nitpicker::Input_event_handler,
                         Dialog::Trigger_copy, Dialog::Trigger_paste,
                         Dialog::Trigger_save
{
	Env &_env;

	Heap _heap { _env.ram(), _env.rm() };

	Attached_rom_dataspace _config { _env, "config" };

	Root_directory _vfs { _env, _heap, _config.xml().sub_node("vfs") };

	unsigned _min_width  = 0;
	unsigned _min_height = 0;

	Sandbox _sandbox { _env };

	typedef Sandbox::Local_service<Nitpicker::Session_component> Nitpicker_service;

	Nitpicker_service _nitpicker_service { _sandbox, *this };

	typedef Sandbox::Local_service<Dynamic_rom_session> Rom_service;

	Rom_service _rom_service { _sandbox, *this };

	typedef Sandbox::Local_service<Report::Session_component> Report_service;

	Report_service _report_service { _sandbox, *this };

	void _handle_hover(Xml_node const &node)
	{
		if (!node.has_sub_node("dialog"))
			_dialog.handle_hover(Xml_node("<empty/>"));

		node.with_sub_node("dialog", [&] (Xml_node const &dialog) {
			_dialog.handle_hover(dialog); });
	}

	Report::Session_component::Xml_handler<Main>
		_hover_handler { *this, &Main::_handle_hover };

	Dialog _dialog { _env.ep(), _env.ram(), _env.rm(), _heap, *this, *this, *this };

	void _generate_sandbox_config(Xml_generator &xml) const
	{
		xml.node("parent-provides", [&] () {

			auto service_node = [&] (char const *name) {
				xml.node("service", [&] () {
					xml.attribute("name", name); }); };

			service_node("ROM");
			service_node("CPU");
			service_node("PD");
			service_node("LOG");
			service_node("File_system");
			service_node("Nitpicker");
			service_node("Timer");
			service_node("Report");
		});

		xml.node("start", [&] () {
			xml.attribute("name", "menu_view");
			xml.attribute("caps", 120);
			xml.node("resource", [&] () {
				xml.attribute("name", "RAM");
				xml.attribute("quantum", "6M");
			});

			xml.node("config", [&] () {
				xml.attribute("xpos", "100");
				xml.attribute("ypos", "50");

				if (_min_width)  xml.attribute("width",  _min_width);
				if (_min_height) xml.attribute("height", _min_height);

				xml.node("report", [&] () {
					xml.attribute("hover", "yes"); });

				xml.node("libc", [&] () {
					xml.attribute("stderr", "/dev/log"); });

				xml.node("vfs", [&] () {
					xml.node("tar", [&] () {
						xml.attribute("name", "menu_view_styles.tar"); });
					xml.node("dir", [&] () {
						xml.attribute("name", "dev");
						xml.node("log", [&] () { });
					});
					xml.node("dir", [&] () {
						xml.attribute("name", "fonts");
						xml.node("fs", [&] () {
							xml.attribute("label", "fonts");
						});
					});
				});
			});

			xml.node("route", [&] () {

				xml.node("service", [&] () {
					xml.attribute("name", "ROM");
					xml.attribute("label", "dialog");
					xml.node("local", [&] () { });
				});

				xml.node("service", [&] () {
					xml.attribute("name", "Report");
					xml.attribute("label", "hover");
					xml.node("local", [&] () { });
				});

				xml.node("service", [&] () {
					xml.attribute("name", "Nitpicker");
					xml.node("local", [&] () { });
				});

				xml.node("service", [&] () {
					xml.attribute("name", "File_system");
					xml.attribute("label", "fonts");
					xml.node("parent", [&] () {
						xml.attribute("label", "fonts"); });
				});

				xml.node("any-service", [&] () {
					xml.node("parent", [&] () { }); });
			});
		});
	}

	/**
	 * Sandbox::Local_service_base::Wakeup interface
	 */
	void wakeup_local_service() override
	{
		_rom_service.for_each_requested_session([&] (Rom_service::Request &request) {

			if (request.label == "menu_view -> dialog")
				request.deliver_session(_dialog.rom_session);
			else
				request.deny();
		});

		_report_service.for_each_requested_session([&] (Report_service::Request &request) {

			if (request.label == "menu_view -> hover") {
				Report::Session_component &session = *new (_heap)
					Report::Session_component(_env, _hover_handler,
					                          _env.ep(),
					                          request.resources, "", request.diag);
				request.deliver_session(session);
			}
		});

		_report_service.for_each_session_to_close([&] (Report::Session_component &session) {

			destroy(_heap, &session);
			return Report_service::Close_response::CLOSED;
		});

		_nitpicker_service.for_each_requested_session([&] (Nitpicker_service::Request &request) {

			Nitpicker::Session_component &session = *new (_heap)
				Nitpicker::Session_component(_env, *this, _env.ep(),
				                             request.resources, "", request.diag);

			request.deliver_session(session);
		});

		_nitpicker_service.for_each_upgraded_session([&] (Nitpicker::Session_component &session,
		                                                  Session::Resources const &amount) {
			session.upgrade(amount);
			return Nitpicker_service::Upgrade_response::CONFIRMED;
		});

		_nitpicker_service.for_each_session_to_close([&] (Nitpicker::Session_component &session) {

			destroy(_heap, &session);
			return Nitpicker_service::Close_response::CLOSED;
		});
	}

	/**
	 * Nitpicker::Input_event_handler interface
	 */
	void handle_input_event(Input::Event const &event) override
	{
		_dialog.handle_input_event(event);
	}

	Directory::Path _path() const
	{
		return _config.xml().attribute_value("path", Directory::Path());
	}

	void _watch(bool enabled)
	{
		_watch_handler.conditional(enabled, _vfs, _path(), *this, &Main::_handle_watch);
	}

	void _load()
	{
		struct Max_line_len_exceeded : Exception { };

		try {
			File_content content(_heap, _vfs, _path(), File_content::Limit{1024*1024});

			enum { MAX_LINE_LEN = 1000 };
			typedef String<MAX_LINE_LEN + 1> Content_line;

			_dialog.clear();
			content.for_each_line<Content_line>([&] (Content_line const &line) {

				if (line.length() == Content_line::capacity()) {
					warning("maximum line length ", (size_t)MAX_LINE_LEN, " exceeded");
					throw Max_line_len_exceeded();
				}

				_dialog.append_newline();

				for (Utf8_ptr utf8(line.string()); utf8.complete(); utf8 = utf8.next())
					_dialog.append_character(utf8.codepoint());
			});

		}
		catch (...) {
			warning("failed to load file ", _path());
			_dialog.clear();
		}

		_dialog.rom_session.trigger_update();
	}

	Constructible<Watch_handler<Main>> _watch_handler { };

	void _handle_watch() { _load(); }

	/*
	 * Copy
	 */

	Constructible<Expanding_reporter> _clipboard_reporter { };

	/**
	 * Dialog::Trigger_copy interface
	 */
	void trigger_copy() override
	{
		if (!_clipboard_reporter.constructed())
			return;

		_clipboard_reporter->generate([&] (Xml_generator &xml) {
			_dialog.gen_clipboard_content(xml); });
	}

	/*
	 * Paste
	 */

	Constructible<Attached_rom_dataspace> _clipboard_rom { };

	enum { PASTE_BUFFER_SIZE = 64*1024 };
	struct Paste_buffer { char buffer[PASTE_BUFFER_SIZE]; } _paste_buffer { };

	/**
	 * Dialog::Trigger_paste interface
	 */
	void trigger_paste() override
	{
		if (!_clipboard_rom.constructed())
			return;

		_clipboard_rom->update();

		_paste_buffer = { };

		/* leave last byte as zero-termination in tact */
		size_t const max_len = sizeof(_paste_buffer.buffer) - 1;
		size_t const len =
			_clipboard_rom->xml().decoded_content(_paste_buffer.buffer, max_len);

		if (len == max_len) {
			warning("clipboard content exceeds paste buffer");
			return;
		}

		for (Utf8_ptr utf8(_paste_buffer.buffer); utf8.complete(); utf8 = utf8.next())
			_dialog.insert_at_cursor_position(utf8.codepoint());

		_dialog.rom_session.trigger_update();
	}

	/*
	 * Save
	 */

	Constructible<Expanding_reporter> _saved_reporter { };

	struct Saved_version { unsigned value; } _saved_version { 0 };

	void _save_to_file(Directory::Path const &path)
	{
		bool write_error = false;

		try {
			New_file new_file(_vfs, path);

			auto write = [&] (char const *cstring)
			{
				switch (new_file.append(cstring, strlen(cstring))) {
				case New_file::Append_result::OK: break;
				case New_file::Append_result::WRITE_ERROR:
					write_error = true;
					break;
				}
			};

			Buffered_output<1024, decltype(write)> output(write);

			_dialog.for_each_character([&] (Codepoint c) { print(output, c); });
		}
		catch (New_file::Create_failed) {
			error("file creation failed while saving file"); }

		if (write_error) {
			error("write error while saving ", _path());
			return;
		}

		if (_saved_reporter.constructed())
			_saved_reporter->generate([&] (Xml_generator &xml) {
				xml.attribute("version", _saved_version.value); });
	}


	/**
	 * Dialog::Trigger_save interface
	 */
	void trigger_save() override
	{
		_saved_version.value++;
		_save_to_file(_path());
	}

	void _handle_config()
	{
		_config.update();

		Xml_node const config = _config.xml();

		_min_width  = config.attribute_value("min_width",  0U);
		_min_height = config.attribute_value("min_height", 0U);

		bool const copy_enabled  = config.attribute_value("copy",  false);
		bool const paste_enabled = config.attribute_value("paste", false);

		_clipboard_reporter.conditional(copy_enabled,  _env, "clipboard", "clipboard");
		_clipboard_rom     .conditional(paste_enabled, _env, "clipboard");

		_dialog.max_lines(config.attribute_value("max_lines", ~0U));

		_watch(config.attribute_value("watch", false));

		bool const orig_saved_reporter_enabled = _saved_reporter.constructed();

		config.with_sub_node("report", [&] (Xml_node const &node) {
			_saved_reporter.conditional(node.attribute_value("saved", false),
			                            _env, "saved", "saved"); });

		bool const saved_report_out_of_date =
			!orig_saved_reporter_enabled && _saved_reporter.constructed();

		Saved_version const orig_saved_version = _saved_version;

		config.with_sub_node("save", [&] (Xml_node const &node) {
			_saved_version.value =
				node.attribute_value("version", _saved_version.value); });

		bool const saved_version_changed =
			(_saved_version.value != orig_saved_version.value);

		if (saved_version_changed || saved_report_out_of_date)
			_save_to_file(_path());
	}

	Signal_handler<Main> _config_handler {
		_env.ep(), *this, &Main::_handle_config };

	Main(Env &env)
	:
		_env(env)
	{
		/*
		 * The '_load' must be performed before '_handle_config' because
		 * '_handle_config' may call '_save_to_file' if the <config> contains a
		 * <saved> node.
		 */
		_load();

		_config.sigh(_config_handler);
		_handle_config();

		Buffered_xml const config { _heap, "config", [&] (Xml_generator &xml) {
			_generate_sandbox_config(xml); } };

		config.with_xml_node([&] (Xml_node const &config) {
			_sandbox.apply_config(config); });
	}
};


void Component::construct(Genode::Env &env) { static Text_view::Main main(env); }

