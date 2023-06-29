/*
 * \brief  BDF-font implementation of 'Text_painter::Font' interface
 * \author Norman Feske
 * \date   2023-06-20
 */

/*
 * Copyright (C) 2023 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/log.h>

/* gems includes */
#include <gems/bdf_font.h>

using namespace Genode;


static void for_each_line(Const_byte_range_ptr const &bytes, auto const &fn)
{
	char const *src           = bytes.start;
	char const *curr_line     = src;
	size_t      curr_line_len = 0;

	for (size_t n = 0; ; n++) {

		char const c = (n == bytes.num_bytes) ? 0 : *src++;
		bool const end_of_data = (c == 0);
		bool const end_of_line = (c == '\n');

		if (!end_of_data && !end_of_line) {
			curr_line_len++;
			continue;
		}

		if (!end_of_data || curr_line_len > 0)
			fn(Const_byte_range_ptr(curr_line, curr_line_len));

		if (end_of_data)
			break;

		curr_line     = src;
		curr_line_len = 0;
	}
}


static void with_skipped_bytes(Const_byte_range_ptr const &bytes,
                               size_t const n, auto const &fn)
{
	if (bytes.num_bytes < n)
		return;

	Const_byte_range_ptr const remainder { bytes.start     + n,
	                                       bytes.num_bytes - n };
	fn(remainder);
}


static void with_skipped_space(Const_byte_range_ptr const &bytes, auto const &fn)
{
	size_t skip = 0;
	while ((bytes.start[skip] == ' ') && (skip < bytes.num_bytes))
		skip++;

	Const_byte_range_ptr const remainder { bytes.start     + skip,
	                                       bytes.num_bytes - skip };
	fn(remainder);
}


template <typename T>
static void with_parsed_value(Const_byte_range_ptr const &bytes, auto const &fn)
{
	T value { };
	size_t const n = ascii_to(bytes.start, value);
	with_skipped_bytes(bytes, n, [&] (Const_byte_range_ptr const &remainder) {
		with_skipped_space(remainder, [&] (Const_byte_range_ptr const &remainder) {
			fn(value, remainder); }); });
}


static void with_skipped_prefix(Const_byte_range_ptr const &bytes,
                                Const_byte_range_ptr const &prefix,
                                auto const &fn)
{
	if (bytes.num_bytes < prefix.num_bytes)
		return;

	if (strcmp(prefix.start, bytes.start, prefix.num_bytes) != 0)
		return;

	with_skipped_bytes(bytes, prefix.num_bytes, fn);
}


static void with_skipped_prefix(Const_byte_range_ptr const &bytes,
                                char const *prefix, auto const &fn)
{
	with_skipped_prefix(bytes, Const_byte_range_ptr { prefix, strlen(prefix) }, fn);
}


static void with_bounding_box(Const_byte_range_ptr const &bytes, auto const &fn)
{
	with_parsed_value<unsigned>(bytes, [&] (unsigned w, Const_byte_range_ptr const &bytes) {
		with_parsed_value<unsigned>(bytes, [&] (unsigned h, Const_byte_range_ptr const &bytes) {
			with_parsed_value<int>(bytes, [&] (int x_shift, Const_byte_range_ptr const &bytes) {
				with_parsed_value<int>(bytes, [&] (int y_shift, Const_byte_range_ptr const &) {
					fn(w, h, x_shift, y_shift); }); }); }); });
}


Bdf_font::Bdf_font(Allocator &alloc, Const_byte_range_ptr const &bdf, Attr attr)
:
	_alloc(alloc), _shade(attr.shade),
	_slanted(attr.slanted), _raised(attr.raised), _highlighted(attr.highlighted),
	_interlaced(attr.interlaced)
{
	Glyph *curr_glyph = nullptr;
	unsigned curr_line = 0;
	bool bitmap_data = false;

	for_each_line(bdf, [&] (Const_byte_range_ptr const &line) {

		with_skipped_prefix(line, "STARTCHAR ", [&] (Const_byte_range_ptr const &) {
			curr_glyph = nullptr;
			curr_line = 0;
			bitmap_data = false;
		});

		with_skipped_prefix(line, "ENCODING ", [&] (Const_byte_range_ptr const &enc) {
			Codepoint codepoint { };
			ascii_to(enc.start, codepoint.value);
			curr_glyph = nullptr;

			try {
				curr_glyph = new (_alloc) Glyph(_glyphs, codepoint);
			} catch (Glyphs::Conflicting_id) {
				warning("BDF contains two glyphs for the codepoint ", codepoint.value);
			}
		});

		with_skipped_prefix(line, "BBX ", [&] (Const_byte_range_ptr const &bbx) {
			with_bounding_box(bbx, [&] (unsigned w, unsigned h, int x_shift, int y_shift) {
				if (curr_glyph) {
					curr_glyph->bounding_box = { w, h };
					curr_glyph->x_shift = x_shift;
					curr_glyph->y_shift = y_shift; } }); });

		with_skipped_prefix(line, "FONTBOUNDINGBOX ", [&] (Const_byte_range_ptr const &bbx) {
			with_bounding_box(bbx, [&] (unsigned w, unsigned h, int, int y_shift) {
				if (w < Glyph::MAX_WIDTH && h < Glyph::MAX_HEIGHT && unsigned(h + y_shift) < Glyph::MAX_HEIGHT) {
					_baseline = h + y_shift;
					_bounding_box = { w, h }; } }); });

		with_skipped_prefix(line, "FIGURE_WIDTH ", [&] (Const_byte_range_ptr const &bytes) {
			with_parsed_value<unsigned>(bytes, [&] (unsigned w, Const_byte_range_ptr const &) {
				_figure_width = w; }); });

		with_skipped_prefix(line, "DWIDTH ", [&] (Const_byte_range_ptr const &bytes) {
			with_parsed_value<unsigned>(bytes, [&] (unsigned w, Const_byte_range_ptr const &) {
				if (curr_glyph)
					curr_glyph->dwidth = w; }); });

		with_skipped_prefix(line, "ENDCHAR", [&] (Const_byte_range_ptr const &) {
			bitmap_data = false; });

		if (bitmap_data && curr_line < Glyph::MAX_HEIGHT) {
			uint32_t bits { };
			size_t const n = ascii_to_unsigned(line.start, bits, 16);
			if (n > 0) {
				if (curr_glyph)
					curr_glyph->lines[curr_line] = bits;
				curr_line++;
			}
		}

		with_skipped_prefix(line, "BITMAP", [&] (Const_byte_range_ptr const &) {
			bitmap_data = true;
			curr_line = 0;
		});
	});

	/*
	 * The glyph buffer is allocated on stack by'_apply_glyph',
	 * Limit scale factor to keep the buffer reasonably small.
	 */
	unsigned const max_pixels = unsigned(4*_bounding_box.count());
	if (max_pixels) {
		unsigned const max_buffer_size = 64*1024;
		unsigned const max_scale = max_buffer_size/max_pixels;
		_scale = max(1u, min(attr.scale, max_scale));
	}
}


Bdf_font::~Bdf_font()
{
	while (_glyphs.apply_any<Glyph &>([&] (Glyph &glyph) {
		destroy(_alloc, &glyph); }));
}


void Bdf_font::_apply_glyph(Bdf_font::Glyph const &bdf_glyph, Apply_fn const &fn) const
{
	using Opacity = Glyph_painter::Glyph::Opacity;

	unsigned const x_gap = _bounding_box.w() >= bdf_glyph.bounding_box.w()
	                     ? _bounding_box.w()  - bdf_glyph.bounding_box.w() : 0;
	unsigned const x_shift = min(unsigned(bdf_glyph.x_shift), x_gap);

	unsigned const bdf_w = bdf_glyph.bounding_box.w() + x_shift,
	               bdf_h = bdf_glyph.bounding_box.h();

	if (bdf_w < 1 || bdf_w >= Glyph::MAX_WIDTH || bdf_h < 1 || bdf_h >= Glyph::MAX_HEIGHT) {
		warning("BDF glyph for code ", bdf_glyph.id(), " has invalid bounding box ",
                bdf_glyph.bounding_box);
		return;
	}

	/* slanted glyphs need for horizontal space */
	unsigned const slant_adjusted_bdf_w = (_slanted) ? (bdf_w + bdf_h/3) : bdf_w;

	unsigned const w = _scale*slant_adjusted_bdf_w,
	               h = _scale*bdf_h;

	Opacity values[4*w*h + 4] { };

	unsigned const highest_bit = (bdf_w <= 8)  ?  7u
	                           : (bdf_w <= 16) ? 15u
	                           : (bdf_w <= 24) ? 23u
	                           :                 31u;

	int      const y_bottom = _baseline - bdf_glyph.y_shift;
	int      const y_top    = y_bottom  - bdf_glyph.bounding_box.h();
	unsigned const vpos     = y_top*_scale;

	/* render glyph from bitmask, horizontally stretched by four */
	{
		Opacity *line = values + 4;
		for (unsigned y = 0; y < bdf_h; y++) {
			uint32_t const line_bits = bdf_glyph.lines[y];
			for (unsigned k = 0; k < _scale; k++) { /* repeat same line */
				for (unsigned x = 0; x < bdf_w; x++) {
					unsigned const bit = highest_bit - x;

					bool const color = (line_bits & (1u << bit)) != 0;
					Opacity const value { color ? uint8_t(0xff) : uint8_t(0) };

					for (unsigned i = 0; i < 4*_scale; i++)
						line[4*_scale*x + i] = value;
				}
				line += 4*w;
			}
		}
	}

	if (_shade == Shade::LIGHT) {
		Opacity *dst = values + 4;
		for (unsigned y = 0; y < h; y++) {
			uint8_t const color = ~uint8_t((y*127u)/h);
			for (unsigned x = 0; x < w*4; x++)
				*dst++ = { (dst->value) ? color : uint8_t(0) };
		}
	}

	if (_shade == Shade::METALLIC) {
		Opacity *dst = values + 4;
		for (unsigned y = 0; y < h; y++) {
			uint8_t const color = (y < h/2)
			                    ? uint8_t(127 + (y*127u)         / max(h/2, 1u))
			                    : uint8_t(127 + (127u*(y-(h/2))) / max(h/2, 1u));
			for (unsigned x = 0; x < w*4; x++)
				*dst++ = { (dst->value) ? color : uint8_t(0) };
		}
	}

	if (_slanted) {
		/* x shift at first line so that the base line is not shifted */
		int slant_x_offset = int((bdf_h + bdf_glyph.y_shift)*_scale/3);
		unsigned cycle = 0; /* wraps after three lines */
		Opacity *line = values + 4;
		for (unsigned y = 0; y < h; y++) {

			/* save line values before modifying the line */
			Opacity orig_line[w] { };
			for (unsigned i = 0; i < w; i++)
				orig_line[i] = line[4*i];

			for (unsigned x = 0; x < w; x++) {
				int const x_v0 = x - slant_x_offset + 1,
				          x_v1 = x - slant_x_offset;
				uint8_t const
					v0 = (x_v0 >= 0 && x_v0 < int(w)) ? orig_line[x_v0].value : uint8_t(0),
					v1 = (x_v1 >= 0 && x_v1 < int(w)) ? orig_line[x_v1].value : uint8_t(0);

				uint8_t const color = (cycle == 0) ? uint8_t((v1*6)/7)
				                    : (cycle == 1) ? uint8_t((v0*2 + v1*3)/5)
				                    :                uint8_t((v0*3 + v1*2)/5);

				for (unsigned i = 0; i < 4; i++)
					line[4*x + i] = { color };
			}

			line += 4*w;

			cycle = (cycle == 2u) ? 0 : (cycle + 1u);
			if (cycle == 0)
				slant_x_offset--;
		}
	}

	if (_raised) {
		Opacity *line = values + 4;
		Opacity prev_line[w] { };
		for (unsigned y = 0; y < h; y++) {
			for (unsigned x = 0; x < w; x++) {

				uint8_t const orig_color = line[4*x].value,
				             above_color = prev_line[x].value;

				auto attenuated = [] (uint8_t v) { return uint8_t(min(3u*v/2u, 255u)); };
				auto subdued    = [] (uint8_t v) { return uint8_t(1*v/3u); };
				auto normal     = [] (uint8_t v) { return uint8_t((2*v)/3u); };

				uint8_t const color = (orig_color && !above_color) ? attenuated(orig_color)
				                    : (!orig_color && above_color) ? subdued(above_color)
				                    :                                normal(orig_color);

				prev_line[x] = line[4*x]; /* remember orig value */

				for (unsigned i = 0; i < 4; i++)
					line[4*x + i] = { color };
			}
			line += w*4;
		}
	}

	auto blur = [] (Opacity const *values, unsigned n, unsigned step, auto const &fn)
	{
		/* save line values before modifying the line */
		Opacity orig[n] { };
		for (unsigned i = 0; i < n; i++)
			orig[i] = values[step*i];

		for (unsigned x = 0; x < n; x++) {
			uint8_t const v[5] {  (x >= 2    ? orig[x - 2].value : uint8_t(0)),
			                      (x >= 1    ? orig[x - 1].value : uint8_t(0)),
			                                   orig[x].value,
			                      (x + 1 < n ? orig[x + 1].value : uint8_t(0)),
			                      (x + 2 < n ? orig[x + 2].value : uint8_t(0)) };
			fn(x, v);
		}
	};

	if (_highlighted) {
		{ /* horizontal */
			Opacity *dst = values + 4;
			for (unsigned y = 0; y < h; y++) {
				blur(dst, w, 4, [&] (unsigned const x, uint8_t const v[5]) {
					uint8_t const color = uint8_t((v[0] + 2*v[1] + 3*v[2] + 2*v[3] + v[4]) / 9u);
					for (unsigned i = 0; i < 4; i++)
						dst[4*x + i] = { max(dst[4*x + i].value, color) };
				});
				dst += w*4;
			}
		}
		{ /* vertical */
			Opacity *dst = values + 4;
			for (unsigned x = 0; x < w; x++) {
				blur(dst, h, 4*w, [&] (unsigned const y, uint8_t const v[5]) {
					uint8_t const color = uint8_t((v[0] + 2*v[1] + 3*v[2] + 2*v[3] + v[4]) / 9u);
					for (unsigned i = 0; i < 4; i++)
						dst[4*y*w + i] = { max(dst[4*y*w + i].value, color) };
				});
				dst += 4;
			}
		}
		{
			Opacity *dst = values + 4;
			for (unsigned i = 0; i < w*h*4; i++) {
				unsigned const v = max(unsigned(dst[i].value), 0u) + 0u;
				dst[i] = { uint8_t(min(((v*v)/80u), 255u)) };
			}
		}
	}

	if (_interlaced) {
		Opacity *dst = values + 4;
		for (unsigned y = 0; y < h; y++) {
			bool const odd = (y & 1) == 1;

			blur(dst, w, 4, [&] (unsigned const x, uint8_t const v[5]) {
				uint8_t const color = odd
				                    ? uint8_t((v[0] + v[1] + 2*v[2] + v[3] + v[4]) / 10u)
				                    : uint8_t((v[0] + 2*v[1] + 24*v[2] + 2*v[3] + v[4]) / 32u);

				for (unsigned i = 0; i < 4; i++)
					dst[4*x + i] = { color };
			});
			dst += w*4;
		}
	}

	Text_painter::Glyph const glyph { .width   = w,
	                                  .height  = h,
	                                  .vpos    = vpos,
	                                  .advance = int(bdf_glyph.dwidth*_scale),
	                                  .values  = values };
	fn.apply(glyph);
}


void Bdf_font::_apply_glyph(Codepoint c, Apply_fn const &fn) const
{
	try {
		_glyphs.apply<Glyph const>(Id_space<Glyph>::Id { c.value }, [&] (Glyph const &bdf_glyph) {
			_apply_glyph(bdf_glyph, fn);
		});
	}
	catch (Id_space<Glyph>::Unknown_id) {
		static bool warned_once = false;
		if (!warned_once) {
			warning("BDF font contains no glyph for code ", c.value);
			warned_once = true;
		}

		Text_painter::Glyph::Opacity values[4] { };
		Text_painter::Glyph const glyph { .width = 0, .height = 0, .vpos = 0,
		                                  .advance = int(_scale*_figure_width),
		                                  .values  = values };
		fn.apply(glyph);
	}
}


Text_painter::Font::Advance_info Bdf_font::advance_info(Codepoint c) const
{
	using Result = Text_painter::Font::Advance_info;
	try {
		return _glyphs.apply<Glyph const>(Id_space<Glyph>::Id { c.value },
			[&] (Glyph const &glyph) -> Result
			{
				unsigned const w = _scale*glyph.bounding_box.w();
				return { .width = w, .advance = int(w) };
			});
	}
	catch (Id_space<Glyph>::Unknown_id) { }
	return { .width = 0, .advance = 0 };
}

