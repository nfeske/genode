/*
 * \brief  System dialog
 * \author Norman Feske
 * \date   2023-04-24
 */

/*
 * Copyright (C) 2023 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _VIEW__SYSTEM_DIALOG_H_
#define _VIEW__SYSTEM_DIALOG_H_

#include <view/layout_helper.h>
#include <view/dialog.h>
#include <view/software_presets_dialog.h>
#include <view/software_update_dialog.h>
#include <view/software_version_dialog.h>
#include <model/settings.h>

namespace Sculpt { struct System_dialog; }


struct Sculpt::System_dialog : Top_level_dialog
{
	using Depot_users = Depot_users_dialog::Depot_users;
	using Image_index = Attached_rom_dataspace;

	Presets const &_presets;

	Software_presets_dialog::Action &_presets_action;

	Build_info const &_build_info;

	enum Tab { PRESET, UPDATE } _selected_tab = Tab::PRESET;

	Hosted<Frame, Vbox, Software_presets_dialog> _presets_dialog { Id { "presets" } };
	Hosted<Frame, Vbox, Software_update_dialog>  _update_dialog;
	Hosted<Frame, Vbox, Software_version_dialog> _version_dialog { Id { "version" } };

	void _hover(Xml_node hover)
	{
		hover.with_optional_sub_node("frame", [&] (Xml_node const &frame) {
			frame.with_optional_sub_node("vbox", [&] (Xml_node const &vbox) {
				switch (_selected_tab) {
				case Tab::PRESET: break;
				case Tab::UPDATE: _update_dialog.hover(vbox);  break;
				}
			});
		});
	}

	Hosted<Frame, Vbox, Hbox, Select_button<Tab>>
		_preset_tab { Id { " Presets " }, Tab::PRESET },
		_update_tab { Id { " Update "  }, Tab::UPDATE };

	void view(Scope<> &s) const override
	{
		s.sub_scope<Frame>([&] (Scope<Frame> &s) {
			s.sub_scope<Vbox>([&] (Scope<Frame, Vbox> &s) {

				/* tabs */
				s.sub_scope<Hbox>([&] (Scope<Frame, Vbox, Hbox> &s) {
					s.widget(_preset_tab, _selected_tab, [&] (auto &s) {
						if (!_presets.available())
							s.attribute("style", "unimportant");
						s.template sub_scope<Dialog::Label>(_preset_tab.id.value);
					});
					s.widget(_update_tab, _selected_tab);
				});

				switch (_selected_tab) {
				case Tab::PRESET:
					s.widget(_presets_dialog, _presets);
					break;
				case Tab::UPDATE:
					s.widget(_update_dialog);
					s.widget(_version_dialog, _build_info);
					break;
				};
			});
		});
	}

	void click(Clicked_at const &at) override
	{
		_preset_tab.propagate(at, [&] (Tab t) { _selected_tab = t; });
		_update_tab.propagate(at, [&] (Tab t) { _selected_tab = t; });

		_presets_dialog.propagate(at, _presets);

		_hover(at._location);
		if (_selected_tab == Tab::UPDATE)
			_update_dialog.propagate(at);
	}

	void clack(Clacked_at const &at) override
	{
		_presets_dialog.propagate(at, _presets, _presets_action);
	}

	void drag(Dragged_at const &) override { }

	System_dialog(Presets                   const &presets,
	              Build_info                const &build_info,
	              Nic_state                 const &nic_state,
	              Download_queue            const &download_queue,
	              Index_update_queue        const &index_update_queue,
	              File_operation_queue      const &file_operation_queue,
	              Depot_users               const &depot_users,
	              Image_index               const &image_index,
	              Software_presets_dialog::Action &presets_action,
	              Depot_users_dialog::Action      &depot_users_action,
	              Software_update_dialog::Action  &update_action)
	:
		Top_level_dialog("system"),
		_presets(presets), _presets_action(presets_action),
		_build_info(build_info),
		_update_dialog(Id { "update" },
		               build_info, nic_state, download_queue,
		               index_update_queue, file_operation_queue, depot_users,
		               image_index, depot_users_action, update_action)
	{ }

	bool update_tab_selected() const { return _selected_tab == Tab::UPDATE; }

	bool keyboard_needed() const { return _update_dialog.keyboard_needed(); }

	void handle_key(Codepoint c) { _update_dialog.handle_key(c); }

	void sanitize_user_selection() { _update_dialog.sanitize_user_selection(); }

	void reset_update_dialog() { _update_dialog.reset(); }
};

#endif /* _VIEW__SYSTEM_DIALOG_H_ */
