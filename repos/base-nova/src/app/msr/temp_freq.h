/*
 * \author Alexander Boettcher
 * \date   2021-10-24
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#pragma once

#include "nova.h"

namespace Msr {
	struct Msr;
	struct Monitoring;
	using Genode::uint8_t;
	using Genode::uint64_t;
};

struct Msr::Monitoring
{
	virtual ~Monitoring() { }

	uint64_t mperf { };
	uint64_t aperf { };
	uint64_t freq_khz { };
	bool     freq_khz_valid { };

	uint8_t temp_tcc           { };
	bool    temp_tcc_valid     { };
	uint8_t temp_package       { };
	bool    temp_package_valid { };
	uint8_t temp_cpu           { };
	bool    temp_cpu_valid     { };

	void update_package_temperature(Nova::Utcb &utcb)
	{
		enum Registers { IA32_PKG_THERM_STATUS = 0x1b1 };

		unsigned const pos = 0;

		utcb.set_msg_word(1);
		utcb.msg()[pos] = IA32_PKG_THERM_STATUS;

		Nova_msr::msr();

		auto const status  = utcb.msg()[pos];
		auto const success = utcb.msg_words();

		struct Status : Genode::Register<64> {
			struct Temperature : Bitfield<16, 7> { }; };

		temp_package       = uint8_t(Status::Temperature::get(status));
		temp_package_valid = success & (1 << pos);
	}

	void update_cpu_temperature(Nova::Utcb &utcb)
	{
		enum Registers { IA32_THERM_STATUS = 0x19c };

		unsigned const pos = 0;

		utcb.set_msg_word(1);
		utcb.msg()[pos] = IA32_THERM_STATUS;

		Nova_msr::msr();

		auto const status  = utcb.msg()[pos];
		auto const success = utcb.msg_words();

		struct Status : Genode::Register<64> {
			struct Temperature : Bitfield<16, 7> { };
			struct Valid       : Bitfield<31, 1> { };
		};

		temp_cpu       = uint8_t(Status::Temperature::get(status));
		temp_cpu_valid = (success & (1 << pos)) &&
		                 Status::Valid::get(status);
	}

	void target_temperature(Nova::Utcb &utcb)
	{
		enum Registers { MSR_TEMPERATURE_TARGET = 0x1a2 };

		unsigned const pos = 0;

		utcb.set_msg_word(1);
		utcb.msg()[pos] = MSR_TEMPERATURE_TARGET;

		Nova_msr::msr();

		auto const status  = utcb.msg()[pos];
		auto const success = utcb.msg_words();

		struct Target : Genode::Register<64> {
			struct Temperature : Bitfield<16, 8> { }; };

		temp_tcc       = uint8_t(Target::Temperature::get(status));
		temp_tcc_valid = success & (1 << pos);
	}

	static bool mperf_aperf(Nova::Utcb &utcb, uint64_t &mperf, uint64_t &aperf)
	{
		enum Registers {
			IA32_MPERF = 0xe7,
			IA32_APERF = 0xe8,
		};

		unsigned const pos_mperf = 0;
		unsigned const pos_aperf = 1;

		utcb.set_msg_word(2);
		utcb.msg()[pos_mperf] = IA32_MPERF;
		utcb.msg()[pos_aperf] = IA32_APERF;

		if (Nova_msr::msr() != Nova::Status::NOVA_OK)
			return false;

		mperf   = utcb.msg()[pos_mperf];
		aperf   = utcb.msg()[pos_aperf];

		auto const success = utcb.msg_words();
		return (success & 0x3) == 0x3;
	}

	void cpu_frequency(Nova::Utcb &utcb, uint64_t tsc_freq_khz)
	{
		uint64_t mcurr = 0ULL, acurr = 0ULL;
		freq_khz_valid = mperf_aperf(utcb, mcurr, acurr);
		if (!freq_khz_valid)
			return;

		uint64_t mdiff = mcurr > mperf ? mcurr - mperf : 0ull;
		uint64_t adiff = acurr > aperf ? acurr - aperf : 0ull;

		if ((~0ULL / tsc_freq_khz) > adiff)
			freq_khz = mdiff ? (adiff * tsc_freq_khz) / mdiff : 0ull;
		else
			freq_khz = mdiff ? (adiff / mdiff) * tsc_freq_khz : 0ull;

		mperf = mcurr;
		aperf = acurr;
	}

	static bool supported(Nova::Utcb &utcb, bool const amd, bool const intel)
	{
		if (!amd && !intel)
			return false;

		uint64_t mperf = 0ULL, aperf = 0ULL;
		return mperf_aperf(utcb, mperf, aperf);
	}

	void report(Genode::Xml_generator &xml, unsigned const tcc) const
	{
		if (freq_khz_valid)
			xml.attribute("freq_khz", freq_khz);
		if (tcc && temp_cpu_valid)
			xml.attribute("temp_c", tcc - temp_cpu);
	}
};
