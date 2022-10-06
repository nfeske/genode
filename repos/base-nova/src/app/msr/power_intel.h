/*
 * \author Alexander Boettcher
 * \date   2021-10-25
 */

/*
 * Copyright (C) 2021-2022 Genode Labs GmbH
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
	struct Power_intel;
}

struct Msr::Power_intel
{
	Cpuid cpuid { };

	uint64_t const write_msr = 1u << 29;

	uint64_t hwp_cap       { };
	uint64_t hwp_req_pkg   { };
	uint64_t hwp_req       { };
	uint64_t epb           { };

	bool valid_hwp_cap     { };
	bool valid_hwp_req_pkg { };
	bool valid_hwp_req     { };
	bool valid_epb         { };

	bool enabled_hwp       { };
	bool init_done         { };

	struct Hwp_cap : Genode::Register<64> {
		struct Perf_highest   : Bitfield< 0, 8> { };
		struct Perf_guaranted : Bitfield< 8, 8> { };
		struct Perf_most_eff  : Bitfield<16, 8> { };
		struct Perf_lowest    : Bitfield<24, 8> { };
	};

	struct Hwp_request : Genode::Register<64> {
		struct Perf_min     : Bitfield< 0, 8> { };
		struct Perf_max     : Bitfield< 8, 8> { };
		struct Perf_desired : Bitfield<16, 8> { };
		struct Perf_epp     : Bitfield<24, 8> {
			enum { PERFORMANCE = 0, BALANCED = 128, ENERGY = 255 };
		};
		struct Activity_wnd  : Bitfield<32,10> { };
		struct Pkg_ctrl      : Bitfield<42, 1> { };
		struct Act_wnd_valid : Bitfield<59, 1> { };
		struct Epp_valid     : Bitfield<60, 1> { };
		struct Desired_valid : Bitfield<61, 1> { };
		struct Max_valid     : Bitfield<62, 1> { };
		struct Min_valid     : Bitfield<63, 1> { };
	};

	struct Epb : Genode::Register<64> {
		struct Hint : Bitfield<0, 4> {
			enum { PERFORMANCE = 0, BALANCED = 7, POWER_SAVING = 15 };
		};
	};

	enum {
		IA32_ENERGY_PERF_BIAS = 0x1b0,

		IA32_PM_ENABLE        = 0x770, 
		IA32_HWP_CAPABILITIES = 0x771, 
		IA32_HWP_REQUEST_PKG  = 0x772, 
		IA32_HWP_REQUEST      = 0x774

		/*
		 * Intel Speed Step - chapter 14.1
		 *
		 * - IA32_PERF_CTL = 0x199
		 *
		 * gets disabled, as soon as Intel HWP is enabled
		 * - see 14.4.2 Enabling HWP
		 */

		/*
		 * Intel spec
		 * - IA32_POWER_CTL = 0x1fc -> http://biosbits.org
		 *
		 * C1E Enable (R/W)
		 * When set to ‘1’, will enable the CPU to switch to the
		 * Minimum Enhanced Intel SpeedStep Technology
		 * operating point when all execution cores enter MWAIT.
		 */

/* check spec XXX
        IA32_ENERGY_PERF_BIAS   = 0x1b0,
        MSR_HWP_INTERRUPT       = 0x773,
*/
	};

	bool hwp_enabled(Nova::Utcb &utcb)
	{
		utcb.set_msg_word(1);
		utcb.msg()[0] = IA32_PM_ENABLE;

		uint8_t res = Nova_msr::msr();
		if (res != Nova::NOVA_OK)
			return false;

		auto const success = utcb.msg_words();
		return (success & 1) && (utcb.msg()[0] & 1);
	}

	void read_epb(Nova::Utcb &utcb)
	{
		utcb.set_msg_word(1);
		utcb.msg()[0] = IA32_ENERGY_PERF_BIAS;

		uint8_t const res = Nova_msr::msr();
		auto const success = utcb.msg_words();

		epb       = utcb.msg()[0];
		valid_epb = (res == Nova::NOVA_OK) && ((success & 1) == 1);
	}

	bool write_epb(Nova::Utcb &utcb, uint64_t const &value)
	{
		utcb.set_msg_word(2);
		utcb.msg()[0] = IA32_ENERGY_PERF_BIAS | write_msr;
		utcb.msg()[1] = value;

		uint8_t const res = Nova_msr::msr();

		auto    const success = utcb.msg_words();
		return (res == Nova::NOVA_OK) && ((success & 3) == 3);
	}

	bool enable_hwp(Nova::Utcb &utcb)
	{
		utcb.set_msg_word(2);
		utcb.msg()[0] = IA32_PM_ENABLE | write_msr;
		utcb.msg()[1] = 1ull;

		uint8_t const res     = Nova_msr::msr();
		auto    const success = utcb.msg_words();
		return (res == Nova::NOVA_OK) && ((success & 3) == 3);
	}

	bool write_hwp_request(Nova::Utcb &utcb, uint64_t const &value)
	{
		utcb.set_msg_word(2);
		utcb.msg()[0] = IA32_HWP_REQUEST | write_msr;
		utcb.msg()[1] = value;

		uint8_t const res = Nova_msr::msr();

		auto    const success = utcb.msg_words();
		return (res == Nova::NOVA_OK) && ((success & 3) == 3);
	}

	void read_hwp(Nova::Utcb &utcb)
	{
		utcb.set_msg_word(3);
		utcb.msg()[0] = IA32_HWP_CAPABILITIES;
		utcb.msg()[1] = IA32_HWP_REQUEST_PKG;
		utcb.msg()[2] = IA32_HWP_REQUEST;

		uint8_t res = Nova_msr::msr();

		if (res != Nova::NOVA_OK) {
			valid_hwp_cap = valid_hwp_req_pkg = valid_hwp_req = false;
			return;
		}

		auto const success = utcb.msg_words();

		hwp_cap     = utcb.msg()[0];
		hwp_req_pkg = utcb.msg()[1];
		hwp_req     = utcb.msg()[2];

		valid_hwp_cap     = (res == Nova::NOVA_OK) && (success & (1 << 0));
		valid_hwp_req_pkg = (res == Nova::NOVA_OK) && (success & (1 << 1));
		valid_hwp_req     = (res == Nova::NOVA_OK) && (success & (1 << 2));
	}

	void update(Nova::Utcb &utcb)
	{
		if (cpuid.hwp()) {
			if (!init_done) {
				enabled_hwp = hwp_enabled(utcb);
				init_done   = true;
			}

			if (enabled_hwp)
				read_hwp(utcb);
		}

		if (cpuid.hwp_energy_perf_bias())
			read_epb(utcb);
	}

	void update(Nova::Utcb &utcb, Genode::Xml_node const &config, Genode::Affinity::Location const &cpu)
	{
		bool const verbose = config.attribute_value("verbose", false);

		config.with_optional_sub_node("intel_speed_step", [&] (Genode::Xml_node const &node) {
			if (!cpuid.hwp_energy_perf_bias())
				return;

			unsigned epb_set = node.attribute_value("epb", ~0U);

			if (Epb::Hint::PERFORMANCE <= epb_set &&
			    epb_set <= Epb::Hint::POWER_SAVING) {

				uint64_t raw_epb = epb;
				Epb::Hint::set(raw_epb, epb_set);
				if (write_epb(utcb, raw_epb))
					read_epb(utcb);
				else
					Genode::warning("epb not updated");
			} else
				if (verbose && epb_set != ~0U)
					Genode::warning("epb out of range [",
					                int(Epb::Hint::PERFORMANCE), "-",
					                int(Epb::Hint::POWER_SAVING), "]");
		});

		config.with_optional_sub_node("hwp", [&] (auto const &node) {
			if (!cpuid.hwp())
				return;

			if (!node.has_attribute("enable"))
				return;

			bool on = node.attribute_value("enable", false);

			if (on && !enabled_hwp) {
				bool ok = enable_hwp(utcb);
				Genode::log(cpu, " enabling HWP ", ok ? " succeeded" : " failed");
			} else
			if (!on && enabled_hwp)
				Genode::log(cpu, " disabling HWP not supported - see Intel spec");

			enabled_hwp = hwp_enabled(utcb);
		});

		config.with_optional_sub_node("hwp_request", [&] (auto const &node) {
			if (!enabled_hwp)
				return;

			if (!valid_hwp_req)
				return;

			if (!cpuid.hwp_energy_perf_pref())
				return;

			using Genode::warning;
			using Genode::Hex;

			uint8_t const low  = uint8_t(Hwp_cap::Perf_lowest::get(hwp_cap));
			uint8_t const high = uint8_t(Hwp_cap::Perf_highest::get(hwp_cap));

			uint64_t raw_hwp = hwp_req;

			if (node.has_attribute("min")) {
				unsigned value = node.attribute_value("min", low);
				if ((low <= value) && (value <= high))
					Hwp_request::Perf_min::set(raw_hwp, value);
				else
					if (verbose)
						warning("min - out of range - ", value, " [",
						        low, "-", high, "]");
			}
			if (node.has_attribute("max")) {
				unsigned value = node.attribute_value("max", high);
				if ((low <= value) && (value <= high))
					Hwp_request::Perf_max::set(raw_hwp, value);
				else
					if (verbose)
						warning("max - out of range - ", value, " [",
						        low, "-", high, "]");
			}
			if (node.has_attribute("desired")) {
				unsigned value = node.attribute_value("desired", 0u /* disable */);
				if (!value || ((low <= value) && (value <= high)))
					Hwp_request::Perf_desired::set(raw_hwp, value);
				else
					if (verbose)
						warning("desired - out of range - ", value, " [",
						        low, "-", high, "]");
			}
			if (node.has_attribute("epp")) {
				unsigned value = node.attribute_value("epp", unsigned(Hwp_request::Perf_epp::BALANCED));
				if (value <= Hwp_request::Perf_epp::ENERGY)
					Hwp_request::Perf_epp::set(raw_hwp, value);
				else
					if (verbose)
						warning("epp - out of range - ", value, " [",
						        low, "-", high, "]");
			}

			if (raw_hwp != hwp_req) {
				if (write_hwp_request(utcb, raw_hwp))
					read_hwp(utcb);
				else
					warning("hwp_request failed, ",
					        Hex(hwp_req), " -> ", Hex(raw_hwp));
			}
		});
	}

	void report(Genode::Xml_generator &xml) const
	{
		using Genode::String;

		if (cpuid.hwp()) {
			xml.node("hwp", [&] () {
				xml.attribute("enable", enabled_hwp);
			});
		}

		if (valid_hwp_cap) {
			xml.node("hwp_cap", [&] () {
				xml.attribute("high", Hwp_cap::Perf_highest::get(hwp_cap));
				xml.attribute("guar", Hwp_cap::Perf_guaranted::get(hwp_cap));
				xml.attribute("effi", Hwp_cap::Perf_most_eff::get(hwp_cap));
				xml.attribute("low",  Hwp_cap::Perf_lowest::get(hwp_cap));
				xml.attribute("raw",  String<19>(Genode::Hex(hwp_cap)));
			});
		}

		if (valid_hwp_req_pkg) {
			xml.node("hwp_request_package", [&] () {
				xml.attribute("raw", String<19>(Genode::Hex(hwp_req_pkg)));
			});
		}

		if (valid_hwp_req) {
			xml.node("hwp_request", [&] () {
				xml.attribute("min", Hwp_request::Perf_min::get(hwp_req));
				xml.attribute("max", Hwp_request::Perf_max::get(hwp_req));
				xml.attribute("desired", Hwp_request::Perf_desired::get(hwp_req));
				xml.attribute("epp", Hwp_request::Perf_epp::get(hwp_req));
				xml.attribute("raw", String<19>(Genode::Hex(hwp_req)));
			});
		}

		if (valid_epb) {
			xml.node("intel_speed_step", [&] () {
				xml.attribute("epb", epb);
			});
		}

		/* msr mperf and aperf availability */
		if (cpuid.hardware_coordination_feedback_cap())
			xml.node("hwp_coord_feed_cap", [&] () { });
	}
};

