/*
 * \brief  CPU load display
 * \author Norman Feske
 * \date   2015-06-30
 */

/*
 * Copyright (C) 2015 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <os/config.h>
#include <os/attached_rom_dataspace.h>
#include <polygon_gfx/shaded_polygon_painter.h>
#include <polygon_gfx/interpolate_rgb565.h>
#include <os/pixel_alpha8.h>
#include <nano3d/scene.h>
#include <nano3d/sincos_frac16.h>
#include <gems/color_hsv.h>


namespace Cpu_load_display {

	class Timeline;
	class Cpu;
	class Cpu_registry;
	template <typename> class Scene;

	typedef Genode::Xml_node Xml_node;
	typedef Genode::Color    Color;

	using Genode::max;
};


class Cpu_load_display::Timeline : public Genode::List<Timeline>::Element
{
	public:

		enum { HISTORY_LEN = 32 };

		typedef Genode::String<160> Label;

	private:

		unsigned const _subject_id = 0;

		unsigned _activity[HISTORY_LEN];

		unsigned _sum_activity = 0;

		unsigned _saved_activity[HISTORY_LEN];

		unsigned _sum_saved_activity = 0;

		bool _hovered = false;

		Label _label;

		/**
		 * Return hue value based on subject ID
		 */
		unsigned _hue() const
		{
			/*
			 * To get nicely varying hue values, we pass the subject ID
			 * to a hash function.
			 */
			unsigned int const a = 1588635695, q = 2, r = 1117695901;
			return (a*(_subject_id % q) - r*(_subject_id / q)) & 255;
		}

	public:

		Timeline(unsigned subject_id, Label const &label)
		:
			_subject_id(subject_id), _label(label)
		{
			Genode::memset(_activity,       0, sizeof(_activity));
			Genode::memset(_saved_activity, 0, sizeof(_saved_activity));
		}

		void activity(unsigned long recent_activity, unsigned now)
		{
			unsigned const i = now % HISTORY_LEN;

			_sum_activity -= _activity[i];

			_activity[i] = recent_activity;

			_sum_activity += recent_activity;
		}

		void save(unsigned now)
		{
			for (unsigned i = 0; i < HISTORY_LEN; i++)
				_saved_activity[i] = _activity[i];

			_sum_saved_activity = _sum_activity;
		}

		unsigned long saved_activity(unsigned i) const
		{
			return _saved_activity[i % HISTORY_LEN];
		}

		bool has_subject_id(unsigned subject_id) const
		{
			return _subject_id == subject_id;
		}

		bool idle() const
		{
			return _sum_activity == 0 && _sum_saved_activity == 0;
		}

		bool kernel() const
		{
			return _label == Label("kernel");
		}

		enum Color_type { COLOR_TOP, COLOR_BOTTOM };

		Color color(Color_type type) const
		{
			unsigned const brightness = _hovered ? 220 : 140;
			unsigned const saturation = type == COLOR_TOP ? 70 : 140;
			unsigned const alpha      = 230;

			Color const c = color_from_hsv(_hue(), saturation, brightness);
			return Color(c.r, c.g, c.b, alpha);
		}

		unsigned subject_id() const { return _subject_id; }

		void hovered(bool hovered) { _hovered = hovered; }
};


class Cpu_load_display::Cpu : public Genode::List<Cpu>::Element
{
	private:

		Genode::Point<>  const _pos;
		Genode::List<Timeline> _timelines;

		unsigned _saved_now = 0;

		Timeline *_lookup_timeline(Xml_node subject)
		{
			unsigned long const subject_id = subject.attribute_value("id", 0UL);

			char label[sizeof(Timeline::Label)];
			subject.attribute("label").value(label, sizeof(label));

			for (Timeline *t = _timelines.first(); t; t = t->next()) {
				if (t->has_subject_id(subject_id))
					return t;
			}

			/* add new timeline */
			Timeline *t = new (Genode::env()->heap()) Timeline(subject_id, label);
			_timelines.insert(t);
			return t;
		}

		unsigned long _activity(Xml_node subject)
		{
			try {
				Xml_node activity = subject.sub_node("activity");
				return activity.attribute_value("recent", 0UL);
			} catch (Xml_node::Nonexistent_sub_node) { }

			return 0;
		}

	public:

		Cpu(Genode::Point<> pos) : _pos(pos) { }

		bool has_pos(Genode::Point<> pos) const
		{
			return pos == _pos;
		}

		void import_trace_subject(Xml_node subject, unsigned now)
		{
			unsigned long const activity = _activity(subject);

			if (activity)
				_lookup_timeline(subject)->activity(activity, now);
		}

		void advance(unsigned now)
		{
			Timeline *next = nullptr;
			for (Timeline *t = _timelines.first(); t; t = next) {

				next = t->next();

				t->activity(0, now);

				if (t->idle()) {

					PINF("discard timeline");
					_timelines.remove(t);
					Genode::destroy(Genode::env()->heap(), t);
				}
			}
		}

		unsigned long activity_sum(unsigned i) const
		{
			unsigned long sum = 0;

			for (Timeline const *t = _timelines.first(); t; t = t->next())
				sum += t->saved_activity(i);

			return sum;
		}

		void save_timelines(unsigned now)
		{
			for (Timeline *t = _timelines.first(); t; t = t->next())
				t->save(now);

			_saved_now = now;
		}

		unsigned saved_now() const { return _saved_now; }

		template <typename FN>
		void for_each_timeline(FN const &fn) const
		{
			for (Timeline const *t = _timelines.first(); t; t = t->next())
				fn(*t);
		}

		void hovered(unsigned subject_id, bool hovered)
		{
			for (Timeline *t = _timelines.first(); t; t = t->next())
				if (t->has_subject_id(subject_id))
					t->hovered(hovered);
		}
};


class Cpu_load_display::Cpu_registry
{
	private:

		Genode::List<Cpu> _cpus;

		static Genode::Point<> _cpu_pos(Xml_node subject)
		{
			try {
				Xml_node affinity = subject.sub_node("affinity");
				return Genode::Point<>(affinity.attribute_value("xpos", 0UL),
				                       affinity.attribute_value("ypos", 0UL));
			} catch (Xml_node::Nonexistent_sub_node) { }

			return Genode::Point<>(0, 0);
		}

		Cpu *_lookup_cpu(Xml_node subject)
		{
			/* find CPU that matches the affinity of the subject */
			Genode::Point<> cpu_pos = _cpu_pos(subject);
			for (Cpu *cpu = _cpus.first(); cpu; cpu = cpu->next()) {
				if (cpu->has_pos(cpu_pos))
					return cpu;
			}

			/* add new CPU */
			Cpu *cpu = new (Genode::env()->heap()) Cpu(cpu_pos);
			_cpus.insert(cpu);
			return cpu;
		}

		void _import_trace_subject(Xml_node subject, unsigned now)
		{
			Cpu *cpu = _lookup_cpu(subject);

			cpu->import_trace_subject(subject, now);
		}

	public:

		void import_trace_subjects(Xml_node node, unsigned now)
		{
			node.for_each_sub_node("subject", [&] (Xml_node subject) {
				_import_trace_subject(subject, now); });
		}

		void save_timelines(unsigned now)
		{
			for (Cpu *cpu = _cpus.first(); cpu; cpu = cpu->next())
				cpu->save_timelines(now);
		}

		template <typename FN>
		void for_each_cpu(FN const &fn) const
		{
			for (Cpu const *cpu = _cpus.first(); cpu; cpu = cpu->next())
				fn(*cpu);
		}

		void advance(unsigned now)
		{
			for (Cpu *cpu = _cpus.first(); cpu; cpu = cpu->next())
				cpu->advance(now);
		}

		void hovered(unsigned subject_id, bool hovered)
		{
			for (Cpu *cpu = _cpus.first(); cpu; cpu = cpu->next())
				cpu->hovered(subject_id, hovered);
		}
};


template <typename PT>
class Cpu_load_display::Scene : public Nano3d::Scene<PT>,
                                public Nano3d::Input_handler
{
	private:

		Nitpicker::Area const _size;

		void _handle_config(unsigned)
		{
			Genode::config()->reload();
		}

		Genode::Signal_dispatcher<Scene> _config_dispatcher;

		Genode::Attached_rom_dataspace _trace_subjects { "trace_subjects" };

		unsigned _now = 0;

		Cpu_registry _cpu_registry;

		void _handle_trace_subjects(unsigned)
		{
			_trace_subjects.update();

			if (!_trace_subjects.valid())
				return;

			_cpu_registry.advance(++_now);

			try {
				Xml_node subjects(_trace_subjects.local_addr<char>());
				_cpu_registry.import_trace_subjects(subjects, _now);
			} catch (...) { PWRN("failed to import trace subjects"); }
		}

		Genode::Signal_dispatcher<Scene> _trace_subjects_dispatcher;

		Polygon::Shaded_painter _shaded_painter {
			*Genode::env()->heap(), _size.h() };

		bool _hovered = false;

		unsigned _hovered_subject_id = 0;

		long _activity_sum[Timeline::HISTORY_LEN];
		long _y_level[Timeline::HISTORY_LEN];
		long _y_curr[Timeline::HISTORY_LEN];

		void _plot_cpu(Genode::Surface<PT>                   &pixel,
		               Genode::Surface<Genode::Pixel_alpha8> &alpha,
		               Cpu const &cpu, Nitpicker::Rect rect)
		{
			enum { HISTORY_LEN = Timeline::HISTORY_LEN };

			/* calculate activity sum for each point in history */
			for (unsigned i = 0; i < HISTORY_LEN; i++)
				_activity_sum[i] = cpu.activity_sum(i);

			for (unsigned i = 0; i < HISTORY_LEN; i++)
				_y_level[i] = 0;

			int const h = rect.h();
			int const w = rect.w();

			cpu.for_each_timeline([&] (Timeline const &timeline) {

				if (timeline.kernel())
					return;

				bool first = true;

				/* reset values of the current timeline */
				for (unsigned i = 0; i < HISTORY_LEN; i++)
					_y_curr[i] = 0;

				Color const top_color    = timeline.color(Timeline::COLOR_TOP);
				Color const bottom_color = timeline.color(Timeline::COLOR_BOTTOM);

				unsigned const timeline_now = cpu.saved_now();

				for (unsigned i = 0; i < HISTORY_LEN; i++) {

					unsigned const t      = (timeline_now - i - 0) % HISTORY_LEN;
					unsigned const prev_t = (timeline_now - i + 1) % HISTORY_LEN;

					unsigned long const activity = timeline.saved_activity(t);

					int const dy = _activity_sum[t] ? (activity*h) / _activity_sum[t] : 0;

					_y_curr[t] = _y_level[t] + dy;

					if (!first) {

						/* draw polygon */
						int const n  = HISTORY_LEN - 1;
						int const x0 = ((n - i + 0)*w)/n + rect.x1();
						int const x1 = ((n - i + 1)*w)/n + rect.x1();

						int const y0 = rect.y1() + h - _y_curr[t];
						int const y1 = rect.y1() + h - _y_curr[prev_t];
						int const y2 = rect.y1() + h - _y_level[prev_t];
						int const y3 = rect.y1() + h - _y_level[t];

						typedef Polygon::Shaded_painter::Point Point;
						Point points[4];
						points[0] = Point(x0, y0, top_color);
						points[1] = Point(x1, y1, top_color);
						points[2] = Point(x1, y2, y1 == y2 ? top_color : bottom_color);
						points[3] = Point(x0, y3, y3 == y0 ? top_color : bottom_color);
						_shaded_painter.paint(pixel, alpha, points, 4);

						/* drop shadow */
						Color const black       (0, 0, 0, 100);
						Color const translucent (0, 0, 0,   0);

						points[0] = Point(x0, y3 - 5, translucent);
						points[1] = Point(x1, y2 - 5, translucent);
						points[2] = Point(x1, y2, black);
						points[3] = Point(x0, y3, black);

						_shaded_painter.paint(pixel, alpha, points, 4);
					}
					first = false;
				}

				/* raise level by the values of the current timeline */
				for (unsigned i = 0; i < HISTORY_LEN; i++)
					_y_level[i] = _y_curr[i];

			});
		}

		/**
		 * Apply functor to each visible CPU
		 *
		 * The functor is called with the geomery of the graph and the CPU
		 * as arguments.
		 */
		template <typename FN>
		void _for_each_visible_cpu(FN const &fn)
		{
			/* determine number of CPUs */
			unsigned num_cpus = 0;
			_cpu_registry.for_each_cpu([&] (Cpu const &cpu) { num_cpus++; });
			if (num_cpus == 0)
				return;

			/* visit graphs for the CPUs below each other */
			enum { GAP = 8 };
			Nitpicker::Point const step(0, _size.h()/num_cpus);
			Nitpicker::Area  const size(_size.w(), step.y() - GAP);
			Nitpicker::Point       point(0, GAP/2);

			_cpu_registry.for_each_cpu([&] (Cpu const &cpu) {
				fn(Nitpicker::Rect(point, size), cpu);
				point = point + step;
			});
		}

	public:

		Scene(Genode::Signal_receiver &sig_rec, unsigned update_rate_ms,
		      Nitpicker::Point pos, Nitpicker::Area size)
		:
			Nano3d::Scene<PT>(sig_rec, update_rate_ms, pos, size), _size(size),
			_config_dispatcher(sig_rec, *this, &Scene::_handle_config),
			_trace_subjects_dispatcher(sig_rec, *this, &Scene::_handle_trace_subjects)
		{
			Genode::config()->sigh(_config_dispatcher);
			_handle_config(0);

			_trace_subjects.sigh(_trace_subjects_dispatcher);

			Nano3d::Scene<PT>::input_handler(this);
		}

		/**
		 * Scene interface
		 */
		void render(Genode::Surface<PT>                   &pixel,
		            Genode::Surface<Genode::Pixel_alpha8> &alpha) override
		{
			/*
			 * Update visible timelines only when the pointer is outside the
			 * window.
			 */
			if (!_hovered)
				_cpu_registry.save_timelines(_now);

			/* background */
			Color const top_color    = Color(10, 10, 10, 20);
			Color const bottom_color = Color(10, 10, 10, 100);

			unsigned const w = pixel.size().w();
			unsigned const h = pixel.size().h();

			typedef Polygon::Shaded_painter::Point Point;
			Point points[4];
			points[0] = Point(0,     0,     top_color);
			points[1] = Point(w - 1, 0,     top_color);
			points[2] = Point(w - 1, h - 1, bottom_color);
			points[3] = Point(0,     h - 1, bottom_color);
			_shaded_painter.paint(pixel, alpha, points, 4);

			_for_each_visible_cpu([&] (Nitpicker::Rect rect, Cpu const &cpu) {
				_plot_cpu(pixel, alpha, cpu, rect);
			});
		}

		void _handle_pointer_at(Nitpicker::Point pos)
		{
			unsigned subject_id_at_pos = 0;

			/* determine the subject ID at the specified position */
			_for_each_visible_cpu([&] (Nitpicker::Rect rect, Cpu const &cpu) {

				if (!rect.contains(pos))
					return;

				/* stop search once we found the subject at the position */
				if (subject_id_at_pos)
					return;

				/* shortcuts */
				enum { HISTORY_LEN = Timeline::HISTORY_LEN,
				       XSTEPS      = HISTORY_LEN - 1};

				/* position relative to the graph of the current CPU */
				unsigned const dx = pos.x() - rect.x1();
				unsigned const dy = rect.h() - (pos.y() - rect.y1() + 1);

				/* determine index 't' within the timelines */
				unsigned const saved_now = cpu.saved_now();
				unsigned const i = XSTEPS - 1 - (XSTEPS*dx) / rect.w();
				unsigned const t = (saved_now - i) % HISTORY_LEN;

				/*
				 * Now that we have found the point in time, we need
				 * to accumulate the the timeline heights at this point
				 * until we cross the y coordinate of the position.
				 */
				unsigned const activity_sum = cpu.activity_sum(t);
				if (activity_sum == 0)
					return;

				/* accumulated timeline heights */
				unsigned activity_level_dy = 0;

				cpu.for_each_timeline([&] (Timeline const &timeline) {

				 	/* search no longer once we found the timeline */
					if (subject_id_at_pos)
						return;

					activity_level_dy +=
						(timeline.saved_activity(t)*rect.h()) / activity_sum;

					if (activity_level_dy >= dy)
						subject_id_at_pos = timeline.subject_id();
				});
			});

			if (_hovered_subject_id != subject_id_at_pos) {
				_cpu_registry.hovered(_hovered_subject_id, false);
				_hovered_subject_id = subject_id_at_pos;
				_cpu_registry.hovered(_hovered_subject_id, true);
			}
		}

		/**
		 * Input_handler interface
		 */
		void handle_input(Input::Event const events[], unsigned num) override
		{
			for (unsigned i = 0; i < num; i++) {

				Input::Event const &ev = events[i];

				if (ev.type() == Input::Event::MOTION) {
					_hovered = true;

					_handle_pointer_at(Nitpicker::Point(ev.ax(), ev.ay()));
				}

				if (ev.type() == Input::Event::LEAVE) {
					_hovered = false;
					_cpu_registry.hovered(_hovered_subject_id, false);
					_hovered_subject_id = 0;
				}
			}
		}
};


int main(int argc, char **argv)
{
	static Genode::Signal_receiver sig_rec;

	enum { UPDATE_RATE_MS = 250 };

	static Cpu_load_display::Scene<Genode::Pixel_rgb565>
		scene(sig_rec, UPDATE_RATE_MS,
		      Nitpicker::Point(0, 0), Nitpicker::Area(400, 400));

	scene.dispatch_signals_loop(sig_rec);

	return 0;
}
