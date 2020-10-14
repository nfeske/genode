/*
 * \brief  Suplib driver implementation
 * \author Norman Feske
 * \author Christian Helmuth
 * \author Alexander Boettcher
 * \date   2020-10-12
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

#ifndef _SUP_DRV_H_
#define _SUP_DRV_H_

/* local includes */
#include <sup.h>
#include <sup_gip.h>

/* Genode includes */
#include <base/env.h>
#include <base/attached_rom_dataspace.h>
#include <base/sleep.h>

namespace Sup { struct Drv; }

class Sup::Drv
{
	public:

		enum class Cpu_virt { NONE, VMX, SVM };

	private:

		Env &_env;

		Attached_rom_dataspace const _platform_info_rom { _env, "platform_info" };

		Cpu_count _cpu_count_from_env()
		{
			return Cpu_count { _env.cpu().affinity_space().total() };
		}

		Cpu_freq_khz _cpu_freq_khz_from_rom()
		{
			unsigned khz = 0;

			_platform_info_rom.xml().with_sub_node("hardware", [&] (Xml_node const &node) {
				node.with_sub_node("tsc", [&] (Xml_node const &node) {
					khz = node.attribute_value("freq_khz", khz); });
			});

			if (khz == 0) {
				error("could not read CPU frequency");
				sleep_forever();
			}

			return Cpu_freq_khz { khz };
		}

		Cpu_virt _cpu_virt_from_rom()
		{
			Cpu_virt virt = Cpu_virt::NONE;

			_platform_info_rom.xml().with_sub_node("hardware", [&] (Xml_node const &node) {
				node.with_sub_node("features", [&] (Xml_node const &node) {
					if (node.attribute_value("vmx", false))
						virt = Cpu_virt::VMX;
					else if (node.attribute_value("svm", false))
						virt = Cpu_virt::SVM;
				});
			});

			return virt;
		}

		Cpu_virt const _cpu_virt { _cpu_virt_from_rom() };

		Gip _gip { _env, _cpu_count_from_env(), _cpu_freq_khz_from_rom() };

	public:

		Drv(Env &env) : _env(env) { }

		SUPGLOBALINFOPAGE *gip() { return _gip.gip(); }

		Cpu_virt cpu_virt() { return _cpu_virt; }
};

#endif /* _SUP_DRV_H_ */
