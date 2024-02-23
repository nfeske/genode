/*
 * \brief  PDU (Protocol Data Unit) parsing utilities
 * \author Norman Feske
 * \date   2024-02-22
 */

/*
 * Copyright (C) 2024 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__GEMS__PDU_ENCODING_
#define _INCLUDE__GEMS__PDU_ENCODING_

#include <util/string.h>
#include <util/utf8.h>

/*
   Example for SMS-DELIVER

   07  SMSC length

   91  SCA (originating short message service center)
   94
   71
   22
   72
   30
   33

   04  MTI (SMS-DELIVER, validity period format)

   0D  OA (originating address) length
   91  type (bit7=1)
   21  number...
   43
   65
   87
   09
   21
   F3

   00  PID (protocol identifier)
   00  DCS (data encoding scheme)

   42  last two digits of the year
   20  month
   22  day
   41  hour
   53  minute
   22  second
   40  timezone (CET winter)

   0A  message length

   D4  messaage data (7 bits per character)
   F2
   9C
   0E
   0A
   9F
   C3
   69
   37
*/

namespace Pdu {

	using namespace Genode;

	static void with_hex_digit(char c, auto const &fn)
	{
		if      (c >= '0' && c <= '9') fn(uint8_t(c - '0'));
		else if (c >= 'A' && c <= 'F') fn(uint8_t(c - 'A' + 10));
		/* 'fn' is not called in the presence of an unexpected character */
	}

	static void with_nth_octet(unsigned n, Const_byte_range_ptr const &ptr, auto const &fn)
	{
		if (ptr.num_bytes > n*2)
			with_hex_digit(ptr.start[n*2], [&] (uint8_t high) {
				with_hex_digit(ptr.start[n*2 + 1], [&] (uint8_t low) {
					fn(uint8_t((high<<4) | low)); }); });
	}

	static uint8_t nth_octed(unsigned n, Const_byte_range_ptr const &ptr, uint8_t default_value)
	{
		uint8_t result = default_value;
		with_nth_octet(n, ptr, [&] (uint8_t v) { result = v; });
		return result;
	}

	static void with_nth_nibble(unsigned n, Const_byte_range_ptr const &ptr, auto const &fn)
	{
		with_nth_octet(n/2, ptr, [&] (uint8_t v) {
			fn((n & 1) ? (v >> 4) : (v & 0xf)); });
	}

	/**
	 * Return Nth digit interpreted as decimal number (0...9)
	 */
	static uint8_t nth_decimal(unsigned n, Const_byte_range_ptr const &ptr, uint8_t default_value)
	{
		auto with_decimal_digit = [] (char c, auto const &fn)
		{
			if (c >= '0' && c <= '9') fn(uint8_t(c - '0'));
			/* 'fn' is not called in the presence of an unexpected character */
		};

		uint8_t result = default_value;
		if (n < ptr.num_bytes)
			with_decimal_digit(ptr.start[n], [&] (uint8_t v) {
				result = v; });
		return result;
	}

	static void with_skipped_octets(unsigned n, Const_byte_range_ptr const &ptr, auto const &fn)
	{
		if (ptr.num_bytes >= n*2)
			fn(Const_byte_range_ptr(ptr.start + n*2, ptr.num_bytes - n*2));
	}

	static void with_skipped_smcs(Const_byte_range_ptr const &ptr, auto const &fn)
	{
		with_nth_octet(0, ptr, [&] (uint8_t smsc_len) {
			with_skipped_octets(1 + smsc_len, ptr, [&] (Const_byte_range_ptr const &ptr) {
				fn(ptr); }); });
	}

	/**
	 * Call 'fn' with printable originating address and remaining bytes
	 */
	static void with_originating_address(Const_byte_range_ptr const &ptr, auto const &fn)
	{
		with_skipped_smcs(ptr, [&] (Const_byte_range_ptr const &ptr) {
			uint8_t const num_digits = nth_octed(1, ptr, 0);
			with_skipped_octets(3, ptr, [&] (Const_byte_range_ptr const &ptr) {

				struct
				{
					uint8_t num_digits;
					Const_byte_range_ptr const &ptr;

					void print(Output &out) const
					{
						for (unsigned i = 0; i < num_digits; i++)
							with_nth_nibble(i, ptr, [&] (uint8_t v) {
								Genode::print(out, v); });
					}

				} originating_address { num_digits, ptr };

				unsigned const num_octets = (num_digits + 1)/2;

				with_skipped_octets(num_octets, ptr, [&] (Const_byte_range_ptr const &ptr) {
					fn(originating_address, ptr); });
			});
		});
	}

	static Codepoint nth_7bit_char(unsigned n, Const_byte_range_ptr const &ptr)
	{
		unsigned const lo_offset = (n*7)/8,
		               hi_offset = lo_offset + 1;

		unsigned const lo  = nth_octed(lo_offset, ptr, 0),
		               hi  = nth_octed(hi_offset, ptr, 0);

		unsigned const hi_lo  = (hi << 8) | lo;
		unsigned const rshift = (n*7) & 7;

		return { (hi_lo >> rshift) & 0x7f };
	}

	static Codepoint nth_ucs_char(unsigned n, Const_byte_range_ptr const &ptr)
	{
		unsigned const lo_offset = n*2 + 1,
		               hi_offset = n*2;

		uint8_t const lo = nth_octed(lo_offset, ptr, 0),
		              hi = nth_octed(hi_offset, ptr, 0);

		return { uint16_t((hi << 8) | lo) };
	}

	struct Content
	{
		uint8_t encoding;  /* TP-DCS */
		uint8_t len;       /* TP-UDL (number of characters) */
		Const_byte_range_ptr const &ptr;

		void print(Output &out) const
		{
			using Genode::print;

			static constexpr uint8_t GSM = 0, UCS2 = 0x8;

			if (encoding == GSM) {
				for (unsigned i = 0; i < len; i++)
					print(out, nth_7bit_char(i, ptr));

			} else if (encoding == UCS2) {
				for (unsigned i = 0; i < len/2; i++)
					print(out, nth_ucs_char(i, ptr));

			} else {
				print(out, "[unsupported encoding ", Hex(encoding), "]");
			}
		}
	};

	struct Date
	{
		uint8_t year, month, day, hour, minute, second, timezone;

		static Date from_pdu(Const_byte_range_ptr const &ptr)
		{
			auto nth_pair_of_decimals = [&] (unsigned n)
			{
				uint8_t const lo = nth_decimal(n*2,     ptr, 0),
				              hi = nth_decimal(n*2 + 1, ptr, 0);

				return uint8_t(10*hi + lo);
			};

			return Date {
				.year     = nth_pair_of_decimals(0),
				.month    = nth_pair_of_decimals(1),
				.day      = nth_pair_of_decimals(2),
				.hour     = nth_pair_of_decimals(3),
				.minute   = nth_pair_of_decimals(4),
				.second   = nth_pair_of_decimals(5),
				.timezone = nth_pair_of_decimals(6),
			};
		}

		void print(Output &out) const
		{
			struct Two_decimals
			{
				uint8_t v;
				void print(Output &out) const
				{
					Genode::print(out, (v < 10) ? "0" : "", v);
				}
			};

			Genode::print(out, "20",
			                   Two_decimals { year   }, "-",
			                   Two_decimals { month  }, "-",
			                   Two_decimals { day    }, " ",
			                   Two_decimals { hour   }, ":",
			                   Two_decimals { minute }, ":",
			                   Two_decimals { second });
		}
	};

	static void with_date(Const_byte_range_ptr const &ptr, auto const &fn)
	{
		with_originating_address(ptr, [&] (auto const &, Const_byte_range_ptr const &ptr) {
			with_skipped_octets(2, ptr, [&] (Const_byte_range_ptr const &ptr) {
				fn(Date::from_pdu(ptr)); }); });
	}

	static void with_content(Const_byte_range_ptr const &ptr, auto const &fn)
	{
		with_originating_address(ptr, [&] (auto const &, Const_byte_range_ptr const &ptr) {
			with_nth_octet(1, ptr, [&] (uint8_t const encoding) {
				with_skipped_octets(2 + 7, ptr, [&] (Const_byte_range_ptr const &ptr) {
					with_nth_octet(0, ptr, [&] (uint8_t len) {
						with_skipped_octets(1, ptr, [&] (Const_byte_range_ptr const &ptr) {
							Content const content { .encoding = encoding,
							                        .len      = len,
							                        .ptr      = ptr };
							fn(content); }); }); }); }); });
	}
}

#endif /* _INCLUDE__GEMS__PDU_ENCODING_ */
