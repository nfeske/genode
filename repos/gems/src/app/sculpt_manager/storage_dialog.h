/*
 * \brief  Storage management dialog
 * \author Norman Feske
 * \date   2018-04-30
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _STORAGE_DIALOG_H_
#define _STORAGE_DIALOG_H_

#include "types.h"
#include "selectable_item.h"
#include "activatable_item.h"
#include "storage_devices.h"
#include "storage_target.h"

namespace Sculpt_manager { struct Storage_dialog; }


struct Sculpt_manager::Storage_dialog : Hover_tracker::Responder
{
	Env &_env;

	Storage_devices const &_storage_devices;

	Expanding_reporter _reporter { _env, "dialog", "storage_dialog" };

	Hover_tracker _hover_tracker { _env, "storage_view_hover", *this };

	bool hovered() const { return _hover_tracker.hovered(); }

	Selectable_item  _device_item    { };
	Selectable_item  _partition_item { };
	Hoverable_item   _use_item       { };
	Hoverable_item   _inspect_item   { };
	Selectable_item  _operation_item { };
	Activatable_item _confirm_item   { };

	void _generate(Xml_generator &) const;

	void _gen_block_device(Xml_generator &, Block_device const &) const;

	void _gen_partition(Xml_generator &, Block_device::Label const &, Partition const &) const;

	void _gen_partition_operations(Xml_generator &, Block_device::Label const &, Partition const &) const;

	void _gen_usb_storage_device(Xml_generator &, Usb_storage_device const &) const;

	void generate()
	{
		_reporter.generate([&] (Xml_generator &xml) { _generate(xml); });
	}

	/**
	 * Hover_tracker::Responder interface
	 */
	void respond_to_hover_change() override
	{
		bool const changed =
			_device_item.hovered(_hover_tracker,
			                     "dialog", "frame", "vbox", "button", "name") |
			_partition_item.hovered(_hover_tracker,
			                        "dialog", "frame", "vbox", "frame", "vbox", "hbox", "name") |
			_use_item.hovered(_hover_tracker,
			                  "dialog", "frame", "vbox", "frame", "vbox", "button", "name") |
			_inspect_item.hovered(_hover_tracker,
			                      "dialog", "frame", "vbox", "frame", "vbox", "button", "name") |
			_operation_item.hovered(_hover_tracker,
			                        "dialog", "frame", "vbox", "frame", "vbox", "button", "name") |
			_confirm_item.hovered(_hover_tracker,
			                      "dialog", "frame", "vbox", "frame", "vbox", "button", "name");

		if (changed)
			generate();
	}

	Storage_target const &_used_target;

	struct Action : Interface
	{
		virtual void format(Storage_target const &) = 0;

		virtual void cancel_format(Storage_target const &) = 0;

		virtual void check(Storage_target const &) = 0;

		virtual void toggle_file_browser(Storage_target const &) = 0;

		virtual void use(Storage_target const &) = 0;
	};

	Storage_target _selected_storage_target() const
	{
		Partition::Number partition = (_partition_item._selected == "")
		                            ? Partition::Number { }
		                            : Partition::Number(_partition_item._selected);

		return Storage_target { _device_item._selected, partition };
	}

	void reset_operation()
	{
		_operation_item.reset();
		_confirm_item.reset();

		generate();
	}

	void click(Action &action)
	{
		Selectable_item::Id const old_selected_device    = _device_item._selected;
		Selectable_item::Id const old_selected_partition = _partition_item._selected;

		_device_item.toggle_selection_on_click();
		_partition_item.toggle_selection_on_click();

		if (!_device_item.selected(old_selected_device))
			_partition_item.reset();

		if (!_device_item.selected(old_selected_device)
		 || !_partition_item.selected(old_selected_partition))
			reset_operation();

		Storage_target const target = _selected_storage_target();

		if (_operation_item.hovered("format")) {
			if (_operation_item.selected("format"))
				action.cancel_format(_selected_storage_target());
			else
				_operation_item.toggle_selection_on_click();
		}

		if (_operation_item.hovered("check"))  action.check (target);

		/* toggle file browser */
		if (_inspect_item.hovered("browse"))
			action.toggle_file_browser(target);

		if (_use_item.hovered("use")) {

			/* release currently used device */
			if (target.valid() && target == _used_target)
				action.use(Storage_target{});

			/* select new used device if none is defined */
			else if (!_used_target.valid())
				action.use(target);
		}

		if (_confirm_item.hovered("confirm")) {
			_confirm_item.propose_activation_on_click();
		}

		generate();
	}

	void clack(Action &action)
	{
		if (_confirm_item.hovered("confirm")) {

			_confirm_item.confirm_activation_on_clack();

			if (_confirm_item.activated("confirm")) {

				Storage_target const target = _selected_storage_target();

				log("format! ", _operation_item._selected);

				if (_operation_item.selected("format"))
					action.format(target);
			}
		} else {
			_confirm_item.reset();
		}

		generate();
	}

	Storage_dialog(Env &env, Storage_devices const &storage_devices,
	               Storage_target const &used)
	:
		_env(env), _storage_devices(storage_devices), _used_target(used)
	{ }
};


void Sculpt_manager::Storage_dialog::_gen_partition_operations(Xml_generator             &xml,
                                                               Block_device::Label const &device,
                                                               Partition           const &partition) const
{
	Storage_target const target { device, partition.number };

	String<16> const version(device, ".", partition.number);

	bool const whole_device = !partition.number.valid();

	bool const target_in_use = (_used_target == target)
	                        || (whole_device && (_used_target.device == device))
	                        || partition.file_system_inspected;

	if (partition.file_system.accessible()) {

		if ((!_used_target.valid() || _used_target == target)
		 && !partition.check_in_progress && !partition.format_in_progress) {

			xml.node("button", [&] () {
				xml.attribute("version", version);
				_use_item.gen_button_attr(xml, "use");
				if (_used_target == target)
					xml.attribute("selected", "yes");
				xml.node("label", [&] () { xml.attribute("text", "Use"); });
			});
		}

		if (!partition.check_in_progress && !partition.format_in_progress
		 && partition.file_system.accessible()) {

			xml.node("button", [&] () {
				_inspect_item.gen_button_attr(xml, "browse");
				xml.attribute("version", version);

				if (partition.file_system_inspected)
					xml.attribute("selected", "yes");

				xml.node("label", [&] () { xml.attribute("text", "Inspect"); });
			});
		}

		if (!target_in_use && !partition.format_in_progress && partition.checkable()) {
			xml.node("button", [&] () {
				_operation_item.gen_button_attr(xml, "check");
				xml.attribute("version", version);

				if (partition.check_in_progress)
					xml.attribute("selected", "yes");

				xml.node("label", [&] () { xml.attribute("text", "Check"); });
			});
			if (partition.check_in_progress)
				xml.node("label", [&] () { xml.attribute("text", "In progress..."); });
		}
	}

	if (!target_in_use && !partition.check_in_progress) {
		xml.node("button", [&] () {
			_operation_item.gen_button_attr(xml, "format");
			xml.attribute("version", version);

			if (partition.format_in_progress)
				xml.attribute("selected", "yes");

			if (partition.number.valid()) {
				xml.node("label", [&] () { xml.attribute("text", "Format partition"); });
			} else {
				xml.node("label", [&] () { xml.attribute("text", "Format device"); });
			}
		});

		if (_operation_item.selected("format")) {
			if (partition.format_in_progress) {
				xml.node("label", [&] () { xml.attribute("text", "In progress..."); });
			} else {
				xml.node("button", [&] () {
					_confirm_item.gen_button_attr(xml, "confirm");
					xml.attribute("version", version);
					xml.node("label", [&] () { xml.attribute("text", "Confirm"); });
				});
			}
		}
	}
}


void Sculpt_manager::Storage_dialog::_gen_partition(Xml_generator             &xml,
                                                    Block_device::Label const &device,
                                                    Partition           const &partition) const
{
	bool const selected = _partition_item.selected(partition.number);

	xml.node("hbox", [&] () {
		xml.attribute("name", partition.number);
		xml.node("button", [&] () {

			if (_partition_item.hovered(partition.number))
				xml.attribute("hovered", "yes");

			if (selected)
				xml.attribute("selected", "yes");

			xml.node("label", [&] () { xml.attribute("text", partition.number); });
		});

		if (partition.label.length() > 1)
			xml.node("label", [&] () {
				xml.attribute("text", String<80>(" (", partition.label, ")")); });

		xml.node("label", [&] () { xml.attribute("text", String<64>(" ", partition.capacity)); });

		Storage_target const target { device, partition.number };
		if (_used_target == target)
			xml.node("label", [&] () { xml.attribute("text", " *"); });
	});

	if (selected)
		_gen_partition_operations(xml, device, partition);
}


void Sculpt_manager::Storage_dialog::_gen_block_device(Xml_generator      &xml,
                                                       Block_device const &dev) const
{
	bool const selected = _device_item.selected(dev.label);

	xml.node("button", [&] () {
		xml.attribute("name", dev.label);

		if (_device_item.hovered(dev.label))
			xml.attribute("hovered", "yes");

		if (selected)
			xml.attribute("selected", "yes");

		xml.node("hbox", [&] () {
			xml.node("label", [&] () { xml.attribute("text", dev.label); });
			xml.node("label", [&] () { xml.attribute("text", String<80>(" (", dev.model, ") ")); });
			xml.node("label", [&] () { xml.attribute("text", String<64>(dev.capacity)); });

			if (_used_target.device == dev.label)
				xml.node("label", [&] () { xml.attribute("text", " *"); });
		});
	});

	if (selected) {
		xml.node("frame", [&] () {
			xml.attribute("name", dev.label);
			xml.node("vbox", [&] () {
				dev.partitions.for_each([&] (Partition const &partition) {
					_gen_partition(xml, dev.label, partition); });

				if (!_partition_item.any_selected())
					_gen_partition_operations(xml, dev.label, *dev.whole_device_partition);
			});
		});
	}
}


void Sculpt_manager::Storage_dialog::_gen_usb_storage_device(Xml_generator            &xml,
                                                             Usb_storage_device const &dev) const
{
	bool const selected = _device_item.selected(dev.label);

	xml.node("button", [&] () {
		xml.attribute("name", dev.label);

		if (_device_item.hovered(dev.label))
			xml.attribute("hovered", "yes");

		if (selected)
			xml.attribute("selected", "yes");

		xml.node("hbox", [&] () {
			xml.node("label", [&] () { xml.attribute("text", dev.label); });

			if (dev.driver_info.constructed()) {
				xml.node("label", [&] () {
					String<16> const vendor  { dev.driver_info->vendor };
					xml.attribute("text", String<64>(" (", vendor, ") ")); });
				xml.node("label", [&] () {
					xml.attribute("text", String<32>(dev.capacity)); });
			}

			if (_used_target.device == dev.label)
				xml.node("label", [&] () { xml.attribute("text", " *"); });
		});
	});

	if (selected)
		xml.node("frame", [&] () {
			xml.attribute("name", dev.label);
			xml.node("vbox", [&] () {
				dev.partitions.for_each([&] (Partition const &partition) {
					_gen_partition(xml, dev.label, partition); });

				if (!_partition_item.any_selected())
					_gen_partition_operations(xml, dev.label, *dev.whole_device_partition);
			});
		});
}


void Sculpt_manager::Storage_dialog::_generate(Xml_generator &xml) const
{
	xml.node("frame", [&] () {
		xml.node("vbox", [&] () {
			xml.node("label", [&] () {
				xml.attribute("text", "Storage");
				xml.attribute("font", "title/regular");
			});
			_storage_devices.block_devices.for_each([&] (Block_device const &dev) {
				_gen_block_device(xml, dev); });
			_storage_devices.usb_storage_devices.for_each([&] (Usb_storage_device const &dev) {
				_gen_usb_storage_device(xml, dev); });

			xml.node("button", [&] () {
				xml.attribute("name", "ram_fs");
				xml.node("label", [&] () { xml.attribute("text", "ram (in-memory file system)"); });
			});
		});
	});
}

#endif /* _STORAGE_DIALOG_H_ */
