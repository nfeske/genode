/*
 * \brief  Playground for painting text
 * \author Norman Feske
 * \date   2018-03-08
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <framebuffer_session/connection.h>
#include <base/component.h>
#include <base/attached_rom_dataspace.h>
#include <base/log.h>
#include <base/heap.h>
#include <os/pixel_rgb888.h>
#include <os/surface.h>
#include <nitpicker_gfx/tff_font.h>
#include <nitpicker_gfx/box_painter.h>
#include <util/bezier.h>
#include <timer_session/connection.h>
#include <os/vfs.h>

/* gems includes */
#include <gems/ttf_font.h>
#include <gems/bdf_font.h>
#include <gems/vfs_font.h>
#include <gems/cached_font.h>

namespace Test {
	using namespace Genode;

	typedef Surface_base::Point Point;
	typedef Surface_base::Area  Area;
	typedef Surface_base::Rect  Rect;
	struct Main;
};


/**
 * Statically linked binary data
 */
extern char _binary_droidsansb10_tff_start[];
extern char _binary_default_tff_start[];


struct Test::Main
{
	Env &_env;

	Framebuffer::Connection _fb { _env, Framebuffer::Mode { } };

	Attached_dataspace _fb_ds { _env.rm(), _fb.dataspace() };

	typedef Pixel_rgb888 PT;

	Surface_base::Area const _size = _fb.mode().area;

	Surface<PT> _surface { _fb_ds.local_addr<PT>(), _size };

	char _glyph_buffer_array[8*1024];

	Tff_font::Glyph_buffer _glyph_buffer { _glyph_buffer_array, sizeof(_glyph_buffer_array) };

	Heap _heap { _env.ram(), _env.rm() };

	Attached_rom_dataspace _config { _env, "config" };

	Root_directory _root { _env, _heap, _config.xml().sub_node("vfs") };

	File_content _bdf { _heap, _root, "font.bdf", File_content::Limit(1024*1024) };

	int _y = 10;

	void test(Bdf_font::Attr attr)
	{
		_bdf.bytes([&] (char const * const start, size_t size) {
			Bdf_font font { _heap, Const_byte_range_ptr(start, size), attr };
			Text_painter::paint(_surface, { 10, _y }, font, Color(255, 255, 255),
			                    "Text aligned at the top-left corner");

			_y += font.height();
		});
	}

	void _refresh() { _fb.refresh(0, 0, _size.w(), _size.h()); }

	Main(Env &env) : _env(env)
	{
		_surface.clip(Rect(Point(0, 0), _size));

		test({ .scale = 1, .shade = Bdf_font::Shade::OPAQUE,   .slanted = 0, .raised = 0, .highlighted = 0, .interlaced = 0 });
		test({ .scale = 1, .shade = Bdf_font::Shade::OPAQUE,   .slanted = 0, .raised = 0, .highlighted = 1, .interlaced = 0 });
		test({ .scale = 1, .shade = Bdf_font::Shade::OPAQUE,   .slanted = 1, .raised = 0, .highlighted = 0, .interlaced = 0 });
		test({ .scale = 1, .shade = Bdf_font::Shade::METALLIC, .slanted = 0, .raised = 0, .highlighted = 0, .interlaced = 0 });
		test({ .scale = 2, .shade = Bdf_font::Shade::OPAQUE,   .slanted = 0, .raised = 0, .highlighted = 0, .interlaced = 0 });
		test({ .scale = 2, .shade = Bdf_font::Shade::OPAQUE,   .slanted = 0, .raised = 0, .highlighted = 1, .interlaced = 0 });
		test({ .scale = 2, .shade = Bdf_font::Shade::OPAQUE,   .slanted = 0, .raised = 0, .highlighted = 0, .interlaced = 1 });
		test({ .scale = 2, .shade = Bdf_font::Shade::METALLIC, .slanted = 0, .raised = 0, .highlighted = 0, .interlaced = 0 });
		test({ .scale = 2, .shade = Bdf_font::Shade::LIGHT,    .slanted = 0, .raised = 1, .highlighted = 0, .interlaced = 0 });
		test({ .scale = 3, .shade = Bdf_font::Shade::OPAQUE,   .slanted = 0, .raised = 0, .highlighted = 0, .interlaced = 1 });
		test({ .scale = 3, .shade = Bdf_font::Shade::LIGHT,    .slanted = 0, .raised = 1, .highlighted = 0, .interlaced = 0 });
		test({ .scale = 3, .shade = Bdf_font::Shade::OPAQUE,   .slanted = 1, .raised = 0, .highlighted = 0, .interlaced = 0 });
		test({ .scale = 4, .shade = Bdf_font::Shade::METALLIC, .slanted = 0, .raised = 1, .highlighted = 0, .interlaced = 1 });

		_refresh();
	}
};


void Component::construct(Genode::Env &env)
{
	static Test::Main main(env);
}


/*
 * Resolve symbol required by libc. It is unused as we implement
 * 'Component::construct' directly instead of initializing the libc.
 */

#include <libc/component.h>

void Libc::Component::construct(Libc::Env &) { }

