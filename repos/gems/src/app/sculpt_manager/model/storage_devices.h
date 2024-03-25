/*
 * \brief  Registry of known storage devices
 * \author Norman Feske
 * \date   2018-05-03
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _MODEL__STORAGE_DEVICES_
#define _MODEL__STORAGE_DEVICES_

#include "types.h"
#include "block_device.h"
#include "usb_storage_device.h"

namespace Sculpt { struct Storage_devices; }


struct Sculpt::Storage_devices
{
	Block_devices       block_devices       { };
	Usb_storage_devices usb_storage_devices { };

	bool _block_devices_report_valid = false;
	bool _usb_active_config_valid    = false;

	bool usb_present = false;

	/**
	 * Update 'block_devices' from 'block_devices' report
	 */
	void update_block_devices_from_xml(Env &env, Allocator &alloc, Xml_node node,
	                                   Signal_context_capability sigh)
	{
		block_devices.update_from_xml(node,

			/* create */
			[&] (Xml_node const &node) -> Block_device & {
				return *new (alloc)
					Block_device(env, alloc, sigh,
					             node.attribute_value("label", Block_device::Label()),
					             node.attribute_value("model", Block_device::Model()),
					             Capacity { node.attribute_value("block_size",  0ULL)
					                      * node.attribute_value("block_count", 0ULL) });
			},

			/* destroy */
			[&] (Block_device &b) { destroy(alloc, &b); },

			/* update */
			[&] (Block_device &, Xml_node const &) { }
		);

		if (node.has_type("block_devices"))
			_block_devices_report_valid = true;
	}

	/**
	 * Update 'usb_storage_devices' from 'usb_active_config' report
	 *
	 * \return true if USB storage device was added or vanished
	 */
	bool update_usb_storage_devices_from_xml(Env &env, Allocator &alloc, Xml_node node,
	                                         Signal_context_capability sigh)
	{
		using Label = Usb_storage_device::Label;

		bool device_added_or_vanished = false;

		usb_storage_devices.update_from_xml(node,

			/* create */
			[&] (Xml_node const &node) -> Usb_storage_device &
			{
				device_added_or_vanished = true;

				return *new (alloc)
					Usb_storage_device(env, alloc, sigh,
					                   node.attribute_value("name", Label()));
			},

			/* destroy */
			[&] (Usb_storage_device &elem)
			{
				device_added_or_vanished = true;
				destroy(alloc, &elem);
			},

			/* update */
			[&] (Usb_storage_device &, Xml_node const &) { }
		);

		_usb_active_config_valid = true;

		usb_present = false;
		usb_storage_devices.for_each([&] (Storage_device const &) {
			usb_present = true; });

		return device_added_or_vanished;
	}

	void gen_usb_storage_policies(Xml_generator &xml) const
	{
		usb_storage_devices.for_each([&] (Usb_storage_device const &device) {
			device.gen_usb_policy(xml); });
	}

	/**
	 * Return true as soon as the storage-device information from the drivers
	 * subsystem is complete.
	 */
	bool all_devices_enumerated() const
	{
		return _block_devices_report_valid && _usb_active_config_valid;
	}

	template <typename FN>
	void for_each(FN const &fn) const
	{
		block_devices.for_each([&] (Block_device const &dev) {
			fn(dev); });

		usb_storage_devices.for_each([&] (Usb_storage_device const &dev) {
			fn(dev); });
	}

	template <typename FN>
	void for_each(FN const &fn)
	{
		block_devices.for_each([&] (Block_device &dev) {
			fn(dev); });

		usb_storage_devices.for_each([&] (Usb_storage_device &dev) {
			fn(dev); });
	}
};

#endif /* _MODEL__STORAGE_DEVICES_ */
