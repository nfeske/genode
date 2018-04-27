/*
 * \brief  Network management dialog
 * \author Norman Feske
 * \date   2018-05-07
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _NETWORK_DIALOG_H_
#define _NETWORK_DIALOG_H_

#include "types.h"
#include "selectable_item.h"
#include "nic_target.h"
#include "nic_state.h"
#include "wifi_connection.h"

namespace Sculpt_manager { struct Network_dialog; }


struct Sculpt_manager::Network_dialog : Hover_tracker::Responder
{
	Env &_env;

	Nic_target      const &_used_nic;
	Access_points   const &_access_points;
	Wifi_connection const &_wifi_connection;
	Nic_state       const &_nic_state;

	Expanding_reporter _reporter { _env, "dialog", "network_dialog" };

	Hover_tracker _hover_tracker { _env, "network_view_hover", *this };

	Hoverable_item  _nic_item { };
	Selectable_item _ap_item  { };
	Hoverable_item  _nic_info { };

	bool hovered() const { return _hover_tracker.hovered(); }

	bool ap_list_hovered() const { return _used_nic.type == Nic_target::WIFI
		                               && _nic_info.hovered("nic_info"); }

	void _gen_access_point(Xml_generator &, Access_point const &) const;
	void _gen_connected_ap(Xml_generator &) const;
	void _gen_access_point_list(Xml_generator &) const;

	void _generate(Xml_generator &) const;

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
			_nic_item.hovered(_hover_tracker, "dialog", "frame", "vbox", "hbox", "button", "name") |
			_ap_item.hovered (_hover_tracker, "dialog", "frame", "vbox", "frame", "vbox", "hbox", "name");

		_nic_info.hovered(_hover_tracker, "dialog", "frame", "vbox", "frame", "name");

		if (changed)
			generate();
	}

	struct Action : Interface
	{
		virtual void nic_target(Nic_target const &) = 0;

		virtual void wifi_disconnect() = 0;
	};

	void click(Action &action)
	{
		if (_nic_item.hovered("off"))   action.nic_target(Nic_target { Nic_target::OFF   });
		if (_nic_item.hovered("local")) action.nic_target(Nic_target { Nic_target::LOCAL });
		if (_nic_item.hovered("wired")) action.nic_target(Nic_target { Nic_target::WIRED });
		if (_nic_item.hovered("wifi"))  action.nic_target(Nic_target { Nic_target::WIFI  });

		if (_wifi_connection.connected() && _ap_item.hovered(_wifi_connection.bssid)) {
			action.wifi_disconnect();
			_ap_item.reset();
		} else {
			_ap_item.toggle_selection_on_click();
		}

		generate();
	}

	Network_dialog(Env                   &env,
	               Nic_target      const &used_nic,
	               Access_points   const &access_points,
	               Wifi_connection const &wifi_connection,
	               Nic_state       const &nic_state)
	:
		_env(env), _used_nic(used_nic), _access_points(access_points),
		_wifi_connection(wifi_connection), _nic_state(nic_state)
	{ }
};


void Sculpt_manager::Network_dialog::_gen_access_point(Xml_generator &xml,
                                                       Access_point const &ap) const
{
	xml.node("hbox", [&] () {
		xml.attribute("name", ap.bssid);
		xml.node("button", [&] () {

			if (_wifi_connection.connected())
				xml.attribute("selected", "yes");
			else
				_ap_item.gen_button_attr(xml, ap.bssid);

			xml.node("label", [&] () {
				xml.attribute("text", " "); }); });

		xml.node("label", [&] () {
			xml.attribute("text", String<20>(" ", ap.ssid)); });

		xml.node("label", [&] () {
			xml.attribute("font", "annotation/regular");
			if (ap.protection == Access_point::WPA_PSK)
				xml.attribute("text", " (WPA) ");
			else
				xml.attribute("text", " ");
		});
		xml.node("float", [&] () {
			xml.attribute("name", "%");
			xml.attribute("east", "yes");
			xml.node("label", [&] () {
				xml.attribute("text", String<8>(ap.quality, "%")); });
		});
	});
}


void Sculpt_manager::Network_dialog::_gen_access_point_list(Xml_generator &xml) const
{
	unsigned cnt = 0;
	_access_points.for_each([&] (Access_point const &ap) {

		/* limit view to highest-quality access points */
		if (cnt++ > 16)
			return;

		/* whenever the user has selected an access point, hide all others */
		if (_ap_item.any_selected() && !_ap_item.selected(ap.bssid))
			return;

		_gen_access_point(xml, ap);
	});

	/*
	 * Present motivational message until we get the first 'wlan_accesspoints'
	 * report.
	 */
	if (cnt == 0)
		xml.node("label", [&] () {
			xml.attribute("text", "Scanning..."); });
}


void Sculpt_manager::Network_dialog::_gen_connected_ap(Xml_generator &xml) const
{
	bool done = false;

	/*
	 * Try to present complete info including the quality from access-point
	 * list.
	 */
	_access_points.for_each([&] (Access_point const &ap) {
		if (!done && _wifi_connection.bssid == ap.bssid) {
			_gen_access_point(xml, ap);
			done = true;
		}
	});

	/*
	 * If access point is not present in the list, fall back to the information
	 * given in the 'wlan_state' report.
	 */
	if (!done)
		_gen_access_point(xml, Access_point { _wifi_connection.bssid,
		                                      _wifi_connection.ssid,
		                                      Access_point::UNKNOWN });

	xml.node("label", [&] () { xml.attribute("text", "associated"); });
}


void Sculpt_manager::Network_dialog::_generate(Xml_generator &xml) const
{
	xml.node("frame", [&] () {
		xml.node("vbox", [&] () {
			xml.node("label", [&] () {
				xml.attribute("text", "Network");
				xml.attribute("font", "title/regular");
			});

			xml.node("hbox", [&] () {

				auto gen_nic_button = [&] (Hoverable_item::Id const &id,
				                           Nic_target::Type   const  type,
				                           String<10>         const &label) {
					xml.node("button", [&] () {

						xml.attribute("name", id);
						_nic_item.gen_button_attr(xml, id);

						if (_used_nic.type == type)
							xml.attribute("selected", "yes");

						xml.node("label", [&] () { xml.attribute("text", label); });
					});
				};

				gen_nic_button("off",   Nic_target::OFF,   "Off");
				gen_nic_button("local", Nic_target::LOCAL, "Local");
				gen_nic_button("wired", Nic_target::WIRED, "Wired");
				gen_nic_button("wifi",  Nic_target::WIFI,  "Wifi");
			});

			if (_used_nic.type == Nic_target::WIFI || _used_nic.type == Nic_target::WIRED) {
				xml.node("frame", [&] () {
					xml.attribute("name", "nic_info");
					xml.node("vbox", [&] () {

						/*
						 * If connected via Wifi, show the information of the
						 * connected access point. If not connected, present
						 * the complete list of access points with the option
						 * to select one.
						 */
						if (_used_nic.type == Nic_target::WIFI) {
							if (_wifi_connection.connected())
								_gen_connected_ap(xml);
							else
								_gen_access_point_list(xml);
						}

						/* append display of uplink IP address */
						if (_nic_state.ready())
							xml.node("label", [&] () {
								xml.attribute("text", _nic_state.ipv4); });
					});
				});
			}
		});
	});
}

#endif /* _NETWORK_DIALOG_H_ */
