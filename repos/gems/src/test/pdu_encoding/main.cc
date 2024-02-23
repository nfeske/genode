/*
 * \brief  PDU (Protocol Data Unit) parsing test
 * \author Norman Feske
 * \date   2024-02-22
 */

/*
 * Copyright (C) 2024 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/component.h>
#include <base/log.h>
#include <gems/pdu_encoding.h>

void Component::construct(Genode::Env &)
{
	using namespace Genode;

	char const * const test_msgs[]
	{
		"0791947106004009040C919489674523010010422022414243400CCD77DA0D72BEE5EDB0DB05",
		"0791947106004009040C91948967452301000842203241218540100045006D006F006A00690020D83DDE3A",
		"0791947122723033000D91942143658709F100004220222151154004D4F29C0E",
		"0791947122723033040D91942143658709F100004220224162954005D2329C9D07",
	};

	auto test = [&] (char const * const msg)
	{
		log("message:");
		Const_byte_range_ptr bytes { msg, strlen(msg) };

		Pdu::with_originating_address(bytes, [&] (auto const &oa, Const_byte_range_ptr const &) {
			log("  from: ", oa); });

		Pdu::with_date(bytes, [&] (Pdu::Date const &date) {
			log("  date: \"", date, "\""); });

		Pdu::with_content(bytes, [&] (auto const &content) {
			log("  content: \"", content, "\""); });
	};

	for (auto const &msg : test_msgs)
		test(msg);
}

