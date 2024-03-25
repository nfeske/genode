/*
 * \brief  Sculpt MMC-driver management
 * \author Norman Feske
 * \date   2024-03-25
 */

/*
 * Copyright (C) 2024 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _MMC_DRIVER_H_
#define _MMC_DRIVER_H_

/* Genode includes */
#include <block_session/block_session.h>

/* local includes */
#include <model/child_exit_state.h>
#include <model/board_info.h>
#include <runtime.h>

namespace Sculpt { struct Mmc_driver; }


struct Sculpt::Mmc_driver : private Noncopyable
{
	struct Action : Interface
	{
		virtual void handle_mmc_discovered() = 0;
	};

	Env    &_env;
	Action &_action;

	Constructible<Child_state> _mmc { };

	Attached_rom_dataspace _devices { _env, "report -> runtime/mmc/block_devices" };

	Signal_handler<Mmc_driver> _devices_handler {
		_env.ep(), *this, &Mmc_driver::_handle_devices };

	void _handle_devices()
	{
		_devices.update();
		_action.handle_mmc_discovered();
	}

	Mmc_driver(Env &env, Action &action) : _env(env), _action(action)
	{
		_devices.sigh(_devices_handler);
		_devices_handler.local_submit();
	}

	void gen_start_node(Xml_generator &xml) const
	{
		if (!_mmc.constructed())
			return;

		xml.node("start", [&] {
			_mmc->gen_start_node_content(xml);
			gen_named_node(xml, "binary", "mmc_drv");
			gen_provides<Block::Session>(xml);
			xml.node("config", [&] {
				xml.attribute("report", "yes");
				xml.node("default-policy", [&] {
					xml.attribute("device", "mmcblk0");
					xml.attribute("writeable", "yes"); });
			});
			xml.node("route", [&] {
				gen_parent_route<Platform::Session>(xml);
				gen_parent_rom_route(xml, "dtb", "mmc_drv.dtb");
				gen_parent_rom_route(xml, "ld.lib.so");
				gen_parent_rom_route(xml, "mmc_drv");
				gen_parent_route<Cpu_session>     (xml);
				gen_parent_route<Pd_session>      (xml);
				gen_parent_route<Log_session>     (xml);
				gen_parent_route<Timer::Session>  (xml);
				gen_parent_route<Report::Session> (xml);
			});
		});
	};

	void update(Registry<Child_state> &registry, Board_info const &board_info)
	{
		_mmc.conditional(board_info.mmc_present,
		                 registry, "mmc", Priority::DEFAULT,
		                 Ram_quota { 16*1024*1024 }, Cap_quota { 500 });
	}

	void with_devices(auto const &fn) const { fn(_devices.xml()); }
};

#endif /* _MMC_DRIVER_H_ */
