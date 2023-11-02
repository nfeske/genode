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

#ifndef _VIEW__NETWORK_DIALOG_H_
#define _VIEW__NETWORK_DIALOG_H_

/* local includes */
#include <model/nic_target.h>
#include <model/nic_state.h>
#include <model/pci_info.h>
#include <view/ap_selector.h>

namespace Sculpt { struct Network_dialog; }


struct Sculpt::Network_dialog : Widget<Frame>
{
	using Wlan_config_policy = Ap_selector::Wlan_config_policy;

	Nic_target const &_nic_target;
	Nic_state  const &_nic_state;
	Pci_info   const &_pci_info;

	struct Action : Ap_selector::Action
	{
		virtual void nic_target(Nic_target::Type) = 0;
	};

	struct Target_selector : Widget<Hbox>
	{
		using Type = Nic_target::Type;

		Hosted<Hbox, Select_button<Type>>
			_off   { Id { "Off"   }, Type::OFF          },
			_local { Id { "Local" }, Type::DISCONNECTED },
			_wired { Id { "Wired" }, Type::WIRED        },
			_wifi  { Id { "Wifi"  }, Type::WIFI         },
			_modem { Id { "Modem" }, Type::MODEM        };

		void view(Scope<Hbox> &s, Nic_target const &target, Pci_info const &pci_info) const
		{
			Type const selected = target.type();

			s.widget(_off, selected);

			/*
			 * Allow interactive selection only if NIC-router configuration
			 * is not manually maintained.
			 */
			if (target.managed() || target.manual_type == Nic_target::DISCONNECTED)
				s.widget(_local, selected);

			if (target.managed() || target.manual_type == Nic_target::WIRED)
				if (pci_info.lan_present)
					s.widget(_wired, selected);

			if (target.managed() || target.manual_type == Nic_target::WIFI)
				if (pci_info.wifi_present)
					s.widget(_wifi, selected);

			if (target.managed() || target.manual_type == Nic_target::MODEM)
				if (pci_info.modem_present)
					s.widget(_modem, selected);
		}

		void click(Clicked_at const &at, Action &action)
		{
			_off  .propagate(at, [&] (Type t) { action.nic_target(t); });
			_local.propagate(at, [&] (Type t) { action.nic_target(t); });
			_wired.propagate(at, [&] (Type t) { action.nic_target(t); });
			_wifi .propagate(at, [&] (Type t) { action.nic_target(t); });
			_modem.propagate(at, [&] (Type t) { action.nic_target(t); });
		}
	};

	Hosted<Frame, Vbox, Target_selector> _target_selector { Id { "target" } };

	Hosted<Frame, Vbox, Frame, Vbox, Ap_selector> _ap_selector;

	void _gen_connected_ap(Xml_generator &, bool) const;

	Network_dialog(Nic_target           const &nic_target,
	               Access_points        const &access_points,
	               Wifi_connection      const &wifi_connection,
	               Nic_state            const &nic_state,
	               Blind_wpa_passphrase const &wpa_passphrase,
	               Wlan_config_policy   const &wlan_config_policy,
	               Pci_info             const &pci_info)
	:
		_nic_target(nic_target), _nic_state(nic_state), _pci_info(pci_info),
		_ap_selector(Id { "aps" },
		             access_points, wifi_connection,
		             wlan_config_policy, wpa_passphrase)
	{ }

	void view(Scope<Frame> &s, Ap_selector::List_hovered &ap_list_hovered) const
	{
		s.sub_scope<Vbox>([&] (Scope<Frame, Vbox> &s) {

			s.widget(_target_selector, _nic_target, _pci_info);

			if (_nic_target.wifi() || _nic_target.wired() || _nic_target.modem()) {

				s.sub_scope<Frame>([&] (Scope<Frame, Vbox, Frame> &s) {
					s.sub_scope<Vbox>([&] (Scope<Frame, Vbox, Frame, Vbox> &s) {

						if (_nic_target.wifi())
							s.widget(_ap_selector, ap_list_hovered);

						if (_nic_state.ready())
							s.sub_scope<Dialog::Label>(_nic_state.ipv4);
					});
				});
			}
		});
	}

	void click(Clicked_at const &at, Action &action)
	{
		_target_selector.propagate(at, action);
		_ap_selector.propagate(at, action);
	}

	bool need_keyboard_focus_for_passphrase() const
	{
		return _nic_target.wifi()
		    && _ap_selector.need_keyboard_focus_for_passphrase();
	}
};

#endif /* _VIEW__NETWORK_DIALOG_H_ */
