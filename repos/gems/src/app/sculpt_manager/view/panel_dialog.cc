/*
 * \brief  Panel dialog
 * \author Norman Feske
 * \date   2020-01-26
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <view/panel_dialog.h>

using namespace Sculpt;


void Panel_dialog::_generate(Xml_generator &xml) const
{
	xml.node("frame", [&] () {
		xml.attribute("style", "unimportant");

		gen_named_node(xml, "float", "left", [&] () {
			xml.attribute("north", true);
			xml.attribute("south", true);
			xml.attribute("west",  true);
			xml.node("hbox", [&] () {
				xml.node("button", [&] () {
					_item.gen_button_attr(xml, "settings");
					xml.node("label", [&] () {
						xml.attribute("text", "Settings");
					});
				});
			});
		});

		gen_named_node(xml, "float", "center", [&] () {
			xml.attribute("north", true);
			xml.attribute("south", true);
			xml.node("hbox", [&] () {

				auto gen_tab = [&] (auto name, auto text, Tab tab) {

					gen_named_node(xml, "button", name, [&] () {

						if (_state.selected_tab() == tab)
							xml.attribute("selected", true);
						else
							_item.gen_hovered_attr(xml, name);

						xml.node("label", [&] () {
							xml.attribute("text", text);
						});
					});
				};

				gen_tab("files",      "Files",      Tab::FILES);
				gen_tab("components", "Components", Tab::COMPONENTS);

				if (_state.inspect_tab_visible())
					gen_tab("inspect", "Inspect", Tab::INSPECT);
			});
		});

		gen_named_node(xml, "float", "right", [&] () {
			xml.attribute("north", true);
			xml.attribute("south", true);
			xml.attribute("east",  true);
			xml.node("hbox", [&] () {
				xml.node("button", [&] () {
					_item.gen_button_attr(xml, "network");
					if (_state.network_visible())
						xml.attribute("selected", true);
					xml.node("label", [&] () {
						xml.attribute("text", "Network");
					});
				});
				xml.node("button", [&] () {
					_item.gen_button_attr(xml, "log");
					if (_state.log_visible())
						xml.attribute("selected", true);
					xml.node("label", [&] () {
						xml.attribute("text", "Log");
					});
				});
			});
		});
	});
}


void Panel_dialog::generate()
{
	_dialog_reporter.generate([&] (Xml_generator &xml) { _generate(xml); });
}
