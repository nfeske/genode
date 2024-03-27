/*
 * \brief  Board discovery information
 * \author Norman Feske
 * \date   2018-06-01
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _MODEL__BOARD_INFO_H_
#define _MODEL__BOARD_INFO_H_

#include <model/boot_fb.h>

namespace Sculpt { struct Board_info; }

struct Sculpt::Board_info
{
	bool wifi_present,
	     lan_present,
	     modem_present,
	     intel_gfx_present,
	     boot_fb_present,
	     vesa_fb_present,
	     soc_fb_present,
	     nvme_present,
	     ahci_present,
	     usb_present,
	     ps2_present,
	     soc_touch_present;

	static Board_info from_xml(Xml_node const &devices, Xml_node const &platform)
	{
		Board_info result { };

		Boot_fb::with_mode(platform, [&] (Boot_fb::Mode mode) {
			result.boot_fb_present = mode.valid(); });

		bool vga = false;

		devices.for_each_sub_node("device", [&] (Xml_node const &device) {

			if (device.attribute_value("name", String<16>()) == "ps2")
				result.ps2_present = true;

			device.with_optional_sub_node("pci-config", [&] (Xml_node const &pci) {

				enum class Pci_class : unsigned {
					WIFI = 0x28000,
					LAN  = 0x20000,
					VGA  = 0x30000,
					AHCI = 0x10601,
					NVME = 0x10802,
					UHCI = 0xc0300, OHCI = 0xc0310, EHCI = 0xc0320, XHCI = 0xc0330,
				};

				enum class Pci_vendor : unsigned { INTEL = 0x8086U, };

				auto matches_class = [&] (Pci_class value)
				{
					return pci.attribute_value("class", 0U) == unsigned(value);
				};

				auto matches_vendor = [&] (Pci_vendor value)
				{
					return pci.attribute_value("vendor_id", 0U) == unsigned(value);
				};

				if (matches_class(Pci_class::WIFI)) result.wifi_present = true;
				if (matches_class(Pci_class::LAN))  result.lan_present  = true;
				if (matches_class(Pci_class::NVME)) result.nvme_present = true;

				if (matches_class(Pci_class::UHCI) || matches_class(Pci_class::OHCI)
				 || matches_class(Pci_class::EHCI) || matches_class(Pci_class::XHCI))
					result.usb_present = true;

				if (matches_class(Pci_class::AHCI) && matches_vendor(Pci_vendor::INTEL))
					result.ahci_present  = true;

				if (matches_class(Pci_class::VGA)) {
					vga = true;
					if (matches_vendor(Pci_vendor::INTEL))
						result.intel_gfx_present = true;
				}
			});
		});

		if (result.intel_gfx_present)
			result.boot_fb_present = false;

		if (vga && !result.intel_gfx_present && !result.boot_fb_present)
			result.vesa_fb_present = true;

		return result;
	}

	void print(Output &out) const
	{
		Genode::print(out, "wifi=",      wifi_present,
		                  " lan=",       lan_present,
		                  " modem=",     modem_present,
		                  " intel_gfx=", intel_gfx_present,
		                  " boot_fb=",   boot_fb_present,
		                  " vesa_fb=",   vesa_fb_present,
		                  " nvme=",      nvme_present,
		                  " ahci=",      ahci_present,
		                  " usb=",       usb_present);
	}
};

#endif /* _MODEL__BOARD_INFO_H_ */
