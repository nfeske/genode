/*
 * \brief  Representation of connectors reported by the framebuffer driver
 * \author Norman Feske
 * \date   2024-10-23
 */

/*
 * Copyright (C) 2024 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _MODEL__FB_CONNECTORS_H_
#define _MODEL__FB_CONNECTORS_H_

#include <types.h>

namespace Sculpt { struct Fb_connectors; };


struct Sculpt::Fb_connectors : Noncopyable
{
	using Area = Gui::Area;
	using Name = String<16>;

	struct Connector;
	using  Connectors = List_model<Connector>;

	struct Brightness
	{
		bool     defined;
		unsigned percent;
	};

	struct Connector : Connectors::Element
	{
		Name const name;
		Area       mm { };
		Brightness brightness { };
		
		struct Mode;
		using  Modes = List_model<Mode>;

		Modes _modes { };

		struct Mode : Modes::Element
		{
			using Id = String<16>;

			Id const id;
			Name     name { };
			Area     px { };
			Area     mm { };
			bool     used = false;
			unsigned hz = 0;

			Mode(Id id) : id(id) { };

			void update(Xml_node const &node)
			{
				name = node.attribute_value("mode_name", Name());
				px.w = node.attribute_value("width",     0u);
				px.h = node.attribute_value("height",    0u);
				mm.w = node.attribute_value("width_mm",  0u);
				mm.h = node.attribute_value("height_mm", 0u);
				used = node.attribute_value("used",      false);
				hz   = node.attribute_value("hz",        0u);
			}

			static Id id_from_xml(Xml_node const &node)
			{
				return node.attribute_value("mode_id", Mode::Id());
			}

			bool matches(Xml_node const &node) const
			{
				return id_from_xml(node) == id;
			}

			static bool type_matches(Xml_node const &node)
			{
				return node.has_type("mode");
			}

		};

		Connector(Name const &name) : name(name) { }

		void update(Allocator &alloc, Xml_node const &node)
		{
			mm.w               = node.attribute_value("width_mm",  0u);
			mm.h               = node.attribute_value("height_mm", 0u);
			brightness.defined = node.has_attribute("brightness");
			brightness.percent = node.attribute_value("brightness", 0u);

			_modes.update_from_xml(node,
				[&] (Xml_node const &node) -> Mode & {
					return *new (alloc) Mode(Mode::id_from_xml(node));
				},
				[&] (Mode &mode) { destroy(alloc, &mode); },
				[&] (Mode &mode, Xml_node const &node) { mode.update(node); });
		}

		bool matches(Xml_node const &node) const
		{
			return node.attribute_value("name", Name()) == name;
		}

		static bool type_matches(Xml_node const &node)
		{
			return node.has_type("connector")
			    && node.attribute_value("connected", false);
		}
	};

	Env                 &_env;
	Allocator           &_alloc;
	Session_label const &_label;

	Connectors _merged   { };
	Connectors _discrete { };

	Rom_handler<Fb_connectors> _connectors {
		_env, _label, *this, &Fb_connectors::_handle_connectors };

	void _handle_connectors(Xml_node const &connectors)
	{
		auto update = [&] (Connectors &model, Xml_node const &node)
		{
			model.update_from_xml(node,
				[&] (Xml_node const &node) -> Connector & {
					return *new (_alloc) Connector(node.attribute_value("name", Name()));
				},
				[&] (Connector &conn) { destroy(_alloc, &conn); },
				[&] (Connector &conn, Xml_node const &node) { conn.update(_alloc, node); });
		};

		update(_discrete, connectors);

		connectors.with_sub_node("merge",
			[&] (Xml_node const &merge) { update(_merged, merge); },
			[&]                         { update(_merged, Xml_node("<merge/>")); });
	}

	static unsigned _count(Connectors const &connectors)
	{
		unsigned count = 0;
		connectors.for_each([&] (Connector const &) { count++; });
		return count;
	}

	unsigned num_merged() const { return _count(_merged);   }

	Fb_connectors(Env &env, Allocator &alloc, Session_label const &label)
	: _env(env), _alloc(alloc), _label(label) { }
};

#endif /* _MODEL__FB_CONNECTORS_H_ */
