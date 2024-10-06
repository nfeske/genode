/*
 * \brief  Nitpicker test program
 * \author Norman Feske
 * \date   2006-08-23
 */

/*
 * Copyright (C) 2006-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <base/env.h>
#include <util/list.h>
#include <base/component.h>
#include <gui_session/connection.h>
#include <input/event.h>
#include <os/pixel_rgb888.h>

namespace Test {

	using namespace Genode;

	class  View;
	struct Top_level_view;
	struct Child_view;
	class  View_stack;
	struct Main;
}


class Test::View : private List<View>::Element, Interface
{
	public:

		using Attr = Gui::Session::View_attr;

	private:

		friend class View_stack;
		friend class List<View>;

		using Command = Gui::Session::Command;

		Gui::Connection &_gui;

		Gui::View_id const _id;
		Gui::Area    const _size;
		Gui::Point         _pos;

	public:

		View(Gui::Connection &gui, Gui::View_id const id, Attr const attr)
		:
			_gui(gui), _id(id), _size(attr.rect.area), _pos(attr.rect.at)
		{ }

		Gui::View_capability view_cap()
		{
			return _gui.view_capability(_id);
		}

		void top()
		{
			_gui.enqueue<Command::Front>(_id);
			_gui.execute();
		}

		virtual void move(Gui::Point const pos)
		{
			_pos = pos;
			_gui.enqueue<Command::Geometry>(_id, Gui::Rect { _pos, _size });
			_gui.execute();
		}

		virtual Gui::Point pos() const { return _pos; }

		Gui::Rect rect() const { return { pos(), _size }; }

		bool contains(Gui::Point pos) { return rect().contains(pos); }
};


struct Test::Top_level_view : View
{
	Top_level_view(Gui::Connection &gui, Gui::View_id const id, Attr const &attr)
	:
		View(gui, id, attr)
	{
		gui.view(id, attr);
	}
};


struct Test::Child_view : View
{
	View &_parent;

	Child_view(Gui::Connection &gui, Gui::View_id const id, View &parent, Attr const &attr)
	:
		View(gui, id, attr), _parent(parent)
	{
		Gui::View_id const parent_id { 0 }; /* temporary */
		gui.associate(parent_id, parent.view_cap());
		gui.child_view(id, parent_id, attr);
		gui.release_view_id(parent_id);
	}

	void move(Gui::Point const pos) override
	{
		View::move(pos - _parent.pos());
	}

	Gui::Point pos() const override
	{
		return _parent.pos() + View::pos();
	}
};


class Test::View_stack : Noncopyable
{
	public:

		struct Input_mask_ptr
		{
			Gui::Area size;

			uint8_t *ptr;

			/**
			 * Return true if input at given position is enabled
			 */
			bool hit(Gui::Point const at) const
			{
				if (!ptr)
					return true;

				return Rect(Point(0, 0), size).contains(at)
				    && ptr[size.w*at.y + at.x];
			}
		};

	private:

		Input_mask_ptr const _input_mask_ptr;

		List<View> _views { };

		struct { View const *_dragged = nullptr; };

	public:

		View_stack(Input_mask_ptr input_mask) : _input_mask_ptr(input_mask) { }

		void with_view_at(Gui::Point const pos, auto const &fn)
		{
			View *tv = _views.first();
			for ( ; tv; tv = tv->next()) {

				if (!tv->contains(pos))
					continue;

				Point const rel = pos - tv->pos();

				if (_input_mask_ptr.hit(rel))
					break;
			}
			if (tv)
				fn(*tv);
		}

		void with_dragged_view(auto const &fn)
		{
			for (View *tv = _views.first(); tv; tv = tv->next())
				if (_dragged == tv)
					fn(*tv);
		}

		void insert(View &tv) { _views.insert(&tv); }

		void top(View &tv)
		{
			_views.remove(&tv);
			tv.top();
			_views.insert(&tv);
		}

		void drag(View const &tv) { _dragged = &tv; };

		void release_dragged_view() { _dragged = nullptr; }
};


struct Test::Main
{
	Env &_env;

	struct Config { bool alpha; } _config { .alpha = false };

	Gui::Connection _gui { _env, "testnit" };

	Constructible<Attached_dataspace> _fb_ds { };

	Constructible<View_stack> _view_stack { };

	/*
	 * View '_v1' is used as coordinate origin of '_v2' and '_v3'.
	 */
	Top_level_view _v1 { _gui,  { 1 }, { .title = "Eins",
	                                     .rect  = { { 150, 100 }, { 230, 200 } },
	                                     .front = true } };

	Child_view _v2 { _gui, { 2 }, _v1, { .title = "Zwei",
	                                     .rect  = { {  20,  20 }, { 230, 210 } },
	                                     .front = true } };

	Child_view _v3 { _gui, { 3 }, _v1, { .title = "Drei",
	                                     .rect  = { {  40,  40 }, { 230, 220 } },
	                                     .front = true } };

	Signal_handler<Main> _input_handler { _env.ep(), *this, &Main::_handle_input };

	int _mx = 0, _my = 0, _key_cnt = 0;

	void _handle_input();

	Main(Env &env);
};


Test::Main::Main(Genode::Env &env) : _env(env)
{
	_gui.input.sigh(_input_handler);
//	_gui.input.exclusive(true);

	Gui::Area const size { 256, 256 };

	Framebuffer::Mode const mode { .area = size, .alpha = _config.alpha };

	log("screen is ", mode);

	_gui.buffer(mode);

	_fb_ds.construct(_env.rm(), _gui.framebuffer.dataspace());

	/*
	 * Paint into pixel buffer, fill alpha channel and input-mask buffer
	 *
	 * Input should refer to the view if the alpha value is more than 50%.
	 */

	using PT = Pixel_rgb888;
	PT * const pixels = _fb_ds->local_addr<PT>();

	uint8_t * const alpha      = (uint8_t *)&pixels[size.count()];
	uint8_t * const input_mask = _config.alpha ? alpha + size.count() : 0;

	for (unsigned i = 0; i < size.h; i++)
		for (unsigned j = 0; j < size.w; j++) {
			pixels[i*size.w + j] = PT((3*i)/8, j, i*j/32);
			if (_config.alpha) {
				alpha[i*size.w + j] = (uint8_t)((i*2) ^ (j*2));
				input_mask[i*size.w + j] = alpha[i*size.w + j] > 127;
			}
		}

	_gui.framebuffer.refresh({ { 0, 0 }, size });

	_view_stack.construct(View_stack::Input_mask_ptr { .size = size,
	                                                   .ptr  = input_mask });

	_view_stack->insert(_v1);
	_view_stack->insert(_v2);
	_view_stack->insert(_v3);
}


void Test::Main::_handle_input()
{
	while (_gui.input.pending()) {

		_gui.input.for_each_event([&] (Input::Event const &ev) {

			log("ev: ", ev);

			unsigned const orig_key_cnt = _key_cnt;

			if (ev.press())   _key_cnt++;
			if (ev.release()) _key_cnt--;

			if (!orig_key_cnt && _key_cnt)
				_gui.input.exclusive(true);

			if (orig_key_cnt && !_key_cnt)
				_gui.input.exclusive(false);

			ev.handle_absolute_motion([&] (int x, int y) {

				/* move selected view */
				_view_stack->with_dragged_view([&] (View &tv) {
					tv.move(tv.pos() + Point(x, y) - Point(_mx, _my)); });

				_mx = x; _my = y;
			});

			/* find selected view and bring it to front */
			if (ev.press() && _key_cnt == 1) {
				_view_stack->with_view_at(Point(_mx, _my), [&] (View &tv) {
					_view_stack->top(tv);
					_view_stack->drag(tv);
				});
			}

			if (ev.release() && _key_cnt == 0)
				_view_stack->release_dragged_view();
		});
	}
}


void Component::construct(Genode::Env &env) { static Test::Main main { env }; }
