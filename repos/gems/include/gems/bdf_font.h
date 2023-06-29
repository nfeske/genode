/*
 * \brief  BDF 'Text_painter::Font'
 * \author Norman Feske
 * \date   2023-06-20
 */

/*
 * Copyright (C) 2023 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__GEMS__BDF_FONT_T_
#define _INCLUDE__GEMS__BDF_FONT_T_

#include <util/xml_node.h>
#include <base/allocator.h>
#include <base/id_space.h>
#include <nitpicker_gfx/text_painter.h>

class Bdf_font : public Text_painter::Font
{
	public:

		enum class Shade { OPAQUE, LIGHT, METALLIC };

	private:

		using Allocator            = Genode::Allocator;
		using Codepoint            = Text_painter::Codepoint;
		using Const_byte_range_ptr = Genode::Const_byte_range_ptr;
		using Area                 = Text_painter::Area;
		using uint32_t             = Genode::uint32_t;

		Allocator &_alloc;

		unsigned _figure_width = 0; /* if code 32 is absent */
		unsigned _baseline = 0;
		Area     _bounding_box { };

		struct Glyph;

		/* ID space using codepoints as keys */
		using Glyphs = Genode::Id_space<Glyph>;

		struct Glyph : Glyphs::Element
		{
			static constexpr unsigned MAX_WIDTH  = 32, /* bits in 'uint32_t' */
			                          MAX_HEIGHT = 32;
			Area bounding_box { };

			unsigned dwidth = 0;

			int x_shift = 0, y_shift = 0;

			uint32_t lines[MAX_HEIGHT] { };

			Glyph(Glyphs &glyphs, Codepoint c)
			:
				Glyphs::Element(*this, glyphs, Glyphs::Id { c.value })
			{ }
		};

		Glyphs mutable _glyphs { };

		unsigned _scale = 1;

		Shade const _shade;
		bool  const _slanted;
		bool  const _raised;
		bool  const _highlighted;
		bool  const _interlaced;

		void _apply_glyph(Bdf_font::Glyph const &, Apply_fn const &) const;

	public:

		struct Unsupported_data  : Genode::Exception { };

		struct Attr
		{
			unsigned scale;
			Shade    shade;
			bool     slanted;
			bool     raised;
			bool     highlighted;
			bool     interlaced;

			static Attr from_xml(Genode::Xml_node const &node)
			{
				using namespace Genode;

				auto shade_from_xml = [] (Xml_node const &node)
				{
					using Value = String<16>;
					Value const value = node.attribute_value("shade", Value("opaque"));

					if (value == "light")    return Shade::LIGHT;
					if (value == "metallic") return Shade::METALLIC;
					if (value == "opaque")   return Shade::OPAQUE;

					warning("unsupported attribute value 'shade=\"", value, "\"'");
					return Shade::OPAQUE;
				};

				return {
					.scale       = node.attribute_value("scale", 1u),
					.shade       = shade_from_xml(node),
					.slanted     = node.attribute_value("slanted",     false),
					.raised      = node.attribute_value("raised",      false),
					.highlighted = node.attribute_value("highlighted", false),
					.interlaced  = node.attribute_value("interlaced",  false),
				};
			}
		};

		/**
		 * Constructor
		 *
		 * \param alloc  allocator for dynamic allocations
		 * \param bdf    BDF font data
		 *
		 * \throw Unsupported_data   unable to parse 'bdf' data
		 */
		Bdf_font(Allocator &alloc, Const_byte_range_ptr const &bdf, Attr);

		~Bdf_font();

		void _apply_glyph(Codepoint, Apply_fn const &) const override;

		Advance_info advance_info(Codepoint) const override;

		unsigned baseline() const override { return _scale*_baseline; }
		unsigned   height() const override { return _scale*_bounding_box.h(); }
		Area bounding_box() const override { return Area { _scale*_bounding_box.w(),
		                                                   _scale*_bounding_box.h() }; }
};

#endif /* _INCLUDE__GEMS__BDF_FONT_T_ */
