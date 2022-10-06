/*
 * \author Alexander Boettcher
 * \date   2022-10-15
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#pragma once

#include "nova.h"
#include "cpuid.h"

namespace Msr {
	using Genode::uint64_t;
	using Genode::uint8_t;
	struct Power_amd;
}

struct Msr::Power_amd
{
	Cpuid cpuid { };

	uint64_t const write_msr = 1u << 29;

	uint64_t pstate_limit  { };
	uint64_t pstate_ctrl   { };
	uint64_t pstate_status { };

	bool valid_pstate_limit  { };
	bool valid_pstate_ctrl   { };
	bool valid_pstate_status { };

	struct Pstate_limit : Genode::Register<64> {
		struct Cur_limit : Bitfield< 0, 4> { };
		struct Max_value : Bitfield< 4, 4> { };
	};

	struct Pstate_ctrl : Genode::Register<64> {
		struct Command : Bitfield< 0, 4> { };
	};

	struct Pstate_status : Genode::Register<64> {
		struct Status : Bitfield< 0, 4> { };
	};

	enum {
		AMD_PSTATE_LIMIT  = 0xc0010061,
		AMD_PSTATE_CTRL   = 0xc0010062,
		AMD_PSTATE_STATUS = 0xc0010063,
	};

	void read_pstate(Nova::Utcb &utcb)
	{
		utcb.set_msg_word(3);
		utcb.msg()[0] = AMD_PSTATE_LIMIT;
		utcb.msg()[1] = AMD_PSTATE_CTRL;
		utcb.msg()[2] = AMD_PSTATE_STATUS;

		auto const res     = Nova_msr::msr();
		auto const success = utcb.msg_words();

		pstate_limit  = utcb.msg()[0];
		pstate_ctrl   = utcb.msg()[1];
		pstate_status = utcb.msg()[2];

		valid_pstate_limit  = (res == Nova::NOVA_OK) && (success & (1 << 0));
		valid_pstate_ctrl   = (res == Nova::NOVA_OK) && (success & (1 << 1));
		valid_pstate_status = (res == Nova::NOVA_OK) && (success & (1 << 2));
	}

	bool write_pstate(Nova::Utcb &utcb, uint64_t const &value)
	{
		utcb.set_msg_word(2);
		utcb.msg()[0] = AMD_PSTATE_CTRL | write_msr;
		utcb.msg()[1] = value;

		uint8_t const res     = Nova_msr::msr();
		auto    const success = utcb.msg_words();
		return (res == Nova::NOVA_OK) && ((success & 3) == 3);
	}

	void update(Nova::Utcb &);
	void update(Nova::Utcb &, Genode::Xml_node const &);
	void report(Genode::Xml_generator &) const;
};

void Msr::Power_amd::update(Nova::Utcb &utcb)
{
	if (cpuid.pstate_support())
		read_pstate(utcb);
}

void Msr::Power_amd::report(Genode::Xml_generator &xml) const
{
	if (cpuid.pstate_support()) {
		xml.node("pstate", [&] () {
			if (valid_pstate_limit) {
				xml.attribute("ro_limit_cur", Pstate_limit::Cur_limit::get(pstate_limit));
				xml.attribute("ro_max_value", Pstate_limit::Max_value::get(pstate_limit));
			}
			if (valid_pstate_ctrl) {
				xml.attribute("rw_command", Pstate_ctrl::Command::get(pstate_ctrl));
			}
			if (valid_pstate_status) {
				xml.attribute("ro_status", Pstate_status::Status::get(pstate_status));
			}
		});
	}
}

void Msr::Power_amd::update(Nova::Utcb &utcb, Genode::Xml_node const &config)
{
	using Genode::warning;

	bool const verbose = config.attribute_value("verbose", false);

	config.with_optional_sub_node("pstate", [&] (Genode::Xml_node const &node) {
		if (!cpuid.pstate_support())
			return;

		if (!node.has_attribute("rw_command"))
			return;

		unsigned value = node.attribute_value("rw_command", 0u /* max */);

		if (valid_pstate_limit && value > Pstate_limit::Max_value::get(pstate_limit)) {
			if (verbose)
				warning("pstate - out of range - ", value, " [0-",
				        Pstate_limit::Max_value::get(pstate_limit), "]");
			return;
		}

		if (!write_pstate(utcb, value)) {
			if (verbose)
				warning("pstate - setting ", value, " failed");
			Genode::error("write failed");
		}
	});
}
