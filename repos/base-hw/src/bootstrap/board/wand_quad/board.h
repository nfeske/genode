/*
 * \brief   Pbxa9 specific board definitions
 * \author  Stefan Kalkowski
 * \date    2017-02-20
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _SRC__BOOTSTRAP__SPEC__WAND_QUAD__BOARD_H_
#define _SRC__BOOTSTRAP__SPEC__WAND_QUAD__BOARD_H_

#include <hw/spec/arm/wand_quad_board.h>
#include <spec/arm/cortex_a9_actlr.h>
#include <spec/arm/cortex_a9_page_table.h>
#include <spec/arm/cpu.h>
#include <hw/spec/arm/gicv2.h>

namespace Board {

	using namespace Hw::Wand_quad_board;

	using Pic = Hw::Gicv2;

	struct L2_cache;

	static constexpr bool NON_SECURE = false;

	static volatile unsigned long initial_values[][2] {
		// (IOMUX Controller)
		{ 0x20e0000, 0x1 },
		{ 0x20e0004, 0x48643005 },
		{ 0x20e0008, 0x221 },
		{ 0x20e000c, 0x1e00040 },
		{ 0x20e0034, 0x593e4a4 },
		{ 0x20e004c, 0x0 },
		{ 0x20e0050, 0x0 },
		{ 0x20e0054, 0x0 },
		{ 0x20e0090, 0x1 },
		{ 0x20e0094, 0x1 },
		{ 0x20e0098, 0x1 },
		{ 0x20e00a4, 0x16 },
		{ 0x20e00a8, 0x4 },
		{ 0x20e00ac, 0x2 },
		{ 0x20e00b0, 0x2 },
		{ 0x20e00b4, 0x2 },
		{ 0x20e00b8, 0x2 },
		{ 0x20e00c4, 0x11 },
		{ 0x20e015c, 0x0 },
		{ 0x20e0160, 0x0 },
		{ 0x20e0164, 0x0 },
		{ 0x20e0168, 0x0 },
		{ 0x20e016c, 0x0 },
		{ 0x20e0170, 0x0 },
		{ 0x20e0174, 0x0 },
		{ 0x20e0178, 0x0 },
		{ 0x20e017c, 0x0 },
		{ 0x20e0180, 0x0 },
		{ 0x20e0184, 0x0 },
		{ 0x20e0188, 0x0 },
		{ 0x20e018c, 0x0 },
		{ 0x20e0190, 0x0 },
		{ 0x20e0194, 0x0 },
		{ 0x20e0198, 0x0 },
		{ 0x20e019c, 0x0 },
		{ 0x20e01a0, 0x0 },
		{ 0x20e01a4, 0x0 },
		{ 0x20e01a8, 0x0 },
		{ 0x20e01ac, 0x0 },
		{ 0x20e01b0, 0x0 },
		{ 0x20e01b4, 0x0 },
		{ 0x20e01b8, 0x0 },
		{ 0x20e01bc, 0x0 },
		{ 0x20e01c0, 0x0 },
		{ 0x20e01c4, 0x0 },
		{ 0x20e01c8, 0x0 },
		{ 0x20e01cc, 0x0 },
		{ 0x20e01d4, 0x1 },
		{ 0x20e01e4, 0x3 },
		{ 0x20e0220, 0x0 },
		{ 0x20e0224, 0x3 },
		{ 0x20e022c, 0x4 },
		{ 0x20e023c, 0x16 },
		{ 0x20e0248, 0x12 },
		{ 0x20e0250, 0x5 },
		{ 0x20e0264, 0x5 },
		{ 0x20e0268, 0x4 },
		{ 0x20e026c, 0x4 },
		{ 0x20e0270, 0x4 },
		{ 0x20e0274, 0x4 },
		{ 0x20e02b8, 0x0 },
		{ 0x20e0320, 0x2 },
		{ 0x20e0348, 0x0 },
		{ 0x20e0354, 0x0 },
		{ 0x20e0358, 0x0 },
		{ 0x20e035c, 0x0 },
		{ 0x20e0360, 0x17059 },
		{ 0x20e0364, 0x17059 },
		{ 0x20e0368, 0x17059 },
		{ 0x20e03a0, 0xf0b0 },
		{ 0x20e03a4, 0x100b1 },
		{ 0x20e03a8, 0x100b1 },
		{ 0x20e03ac, 0x100b1 },
		{ 0x20e03b8, 0x1b8b1 },
		{ 0x20e03c0, 0x1b0b1 },
		{ 0x20e03c4, 0x1b0b1 },
		{ 0x20e03c8, 0x1b0b1 },
		{ 0x20e03cc, 0x1b0b1 },
		{ 0x20e03d8, 0x1b8b1 },
		{ 0x20e0470, 0x10 },
		{ 0x20e0474, 0x10 },
		{ 0x20e0478, 0x10 },
		{ 0x20e047c, 0x10 },
		{ 0x20e0484, 0x10 },
		{ 0x20e0488, 0x10 },
		{ 0x20e048c, 0x10 },
		{ 0x20e0490, 0x10 },
		{ 0x20e0494, 0x10 },
		{ 0x20e0498, 0x10 },
		{ 0x20e049c, 0x10 },
		{ 0x20e04a0, 0x10 },
		{ 0x20e04a4, 0x10 },
		{ 0x20e04a8, 0x10 },
		{ 0x20e04ac, 0x10 },
		{ 0x20e04b0, 0x10 },
		{ 0x20e04b4, 0x10 },
		{ 0x20e04b8, 0x10 },
		{ 0x20e04bc, 0x10 },
		{ 0x20e04c0, 0x10 },
		{ 0x20e04c4, 0x10 },
		{ 0x20e04c8, 0x10 },
		{ 0x20e04cc, 0x10 },
		{ 0x20e04d0, 0x10 },
		{ 0x20e04d4, 0x10 },
		{ 0x20e04d8, 0x10 },
		{ 0x20e04dc, 0x10 },
		{ 0x20e04e0, 0x10 },
		{ 0x20e05e8, 0xb0 },
		{ 0x20e05f0, 0xb0 },
		{ 0x20e05f4, 0x17059 },
		{ 0x20e05fc, 0xb0 },
		{ 0x20e0600, 0xb0b0 },
		{ 0x20e060c, 0x1b8b1 },
		{ 0x20e0618, 0x1b0a8 },
		{ 0x20e0638, 0x130b0 },
		{ 0x20e063c, 0x110b0 },
		{ 0x20e0640, 0x130b0 },
		{ 0x20e0644, 0x130b0 },
		{ 0x20e064c, 0x1b0b0 },
		{ 0x20e06a4, 0x10059 },
		{ 0x20e0704, 0x0 },
		{ 0x20e0708, 0x1b0b1 },
		{ 0x20e0738, 0x10059 },
		{ 0x20e073c, 0x10059 },
		{ 0x20e0740, 0x17059 },
		{ 0x20e0744, 0x17059 },
		{ 0x20e083c, 0x1 },
		{ 0x20e0870, 0x0 },
		{ 0x20e0874, 0x0 },
		{ 0x20e08a8, 0x2 },
		{ 0x20e08ac, 0x2 },
		{ 0x20e092c, 0x1 },
		{ 0x20e0930, 0x1 },

		// (Global Power Controller)
		{ 0x20dc008, 0x6a23e613 },
		{ 0x20dc00c, 0xff69b64f },
		{ 0x20dc010, 0xfffe0003 },
		{ 0x20dc014, 0xff30f7ff },

		// (Power Management Unit)
		{ 0x20c8120, 0x11775 },
		{ 0x20c8140, 0x580016 },
		{ 0x20c8160, 0x8000000b },
		{ 0x20c8170, 0xc0672f67 },

		// (Clock Controller Module)
		{ 0x20c4018, 0x10204 },
		{ 0x20c402c, 0x7312c1 },
		{ 0x20c4030, 0x32271f92 },
		{ 0x20c4034, 0x12680 },
		{ 0x20c4038, 0x12090 },
		{ 0x20c4054, 0x78 },
		{ 0x20c4058, 0x41a0000 },
		{ 0x20c4060, 0x10e0101 },
		{ 0x20c4064, 0x2fe62 },
		{ 0x20c4068, 0xc03f0f },
		{ 0x20c406c, 0x30fc00 },
		{ 0x20c4070, 0x3ff0033 },
		{ 0x20c4074, 0x3ff3303f },
		{ 0x20c4078, 0x30c300 },
		{ 0x20c407c, 0xf0000f3 },
		{ 0x20c4080, 0xc00 },
		{ 0x20c8000, 0x80002053 },
		{ 0x20c8020, 0x3040 },
		{ 0x20c8070, 0x1006 },
		{ 0x20c80a0, 0x80002031 },
		{ 0x20c80b0, 0x7a120 },
		{ 0x20c80c0, 0xf4240 },
		{ 0x20c80e0, 0x80002003 },
		{ 0x20c80f0, 0x9391508c },
		{ 0x20c8100, 0x5058d01b }
	};
}


struct Board::L2_cache : Hw::Pl310
{
	L2_cache(Genode::addr_t mmio) : Hw::Pl310(mmio)
	{
		Aux::access_t aux = 0;
		Aux::Full_line_of_zero::set(aux, true);
		Aux::Associativity::set(aux, Aux::Associativity::WAY_16);
		Aux::Way_size::set(aux, Aux::Way_size::KB_64);
		Aux::Share_override::set(aux, true);
		Aux::Replacement_policy::set(aux, Aux::Replacement_policy::PRAND);
		Aux::Ns_lockdown::set(aux, true);
		Aux::Data_prefetch::set(aux, true);
		Aux::Inst_prefetch::set(aux, true);
		Aux::Early_bresp::set(aux, true);
		write<Aux>(aux);

		Tag_ram::access_t tag_ram = 0;
		Tag_ram::Setup_latency::set(tag_ram, 2);
		Tag_ram::Read_latency::set(tag_ram, 3);
		Tag_ram::Write_latency::set(tag_ram, 1);
		write<Tag_ram>(tag_ram);

		Data_ram::access_t data_ram = 0;
		Data_ram::Setup_latency::set(data_ram, 2);
		Data_ram::Read_latency::set(data_ram, 3);
		Data_ram::Write_latency::set(data_ram, 1);
		write<Data_ram>(data_ram);

		Prefetch_ctrl::access_t prefetch = 0;
		Prefetch_ctrl::Data_prefetch::set(prefetch, 1);
		Prefetch_ctrl::Inst_prefetch::set(prefetch, 1);
		write<Prefetch_ctrl>(prefetch | 0xF);
	}

	using Hw::Pl310::invalidate;

	void enable()
	{
		Pl310::mask_interrupts();
		write<Control::Enable>(1);
	}

	void disable() {
		write<Control::Enable>(0);
	}
};

#endif /* _SRC__BOOTSTRAP__SPEC__WAND_QUAD__BOARD_H_ */