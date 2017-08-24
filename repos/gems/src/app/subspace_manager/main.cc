/*
 * \brief  Subspace manager
 * \author Norman Feske
 * \date   2017-08-23
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* local includes */
#include <resource_provider.h>
#include <parent_services.h>
#include <macro.h>
#include <child.h>

/* Genode includes */
#include <base/weak_ptr.h>
#include <base/component.h>
#include <base/attached_rom_dataspace.h>
#include <base/heap.h>
#include <os/reporter.h>


namespace Subspace_manager { struct Main; }


struct Subspace_manager::Main
{
	Env &_env;

	Attached_rom_dataspace _config { _env, "config" };

	Heap _heap { _env.ram(), _env.rm() };

	Xml_node _config_xml = _config.xml();

	struct Invalid_config : Exception { };

	Xml_node _config_sub_node(char const *type)
	{
		if (_config_xml.has_sub_node(type))
			return _config_xml.sub_node(type);

		warning("configuration lacks '<", type, ">' node");
		throw Invalid_config();
	}

	Resource_provider _global_resources { _heap, _config_sub_node("scene") };

	Binding_list _global_bindings { _heap, _config_sub_node("global") };

	Parent_services _parent_services { _heap, _global_bindings, _config_sub_node("scene") };

	Managed_list<Child> _children { _heap };

	Main(Env &env) : _env(env)
	{
		Xml_node scene = _config_sub_node("scene");
		scene.for_each_sub_node("child", [&] (Xml_node node) {
			Child::Name const name  = node.attribute_value("name",  Child::Name());
			Macro::Name const macro = node.attribute_value("macro", Macro::Name());

			if (node.num_sub_nodes() == 0) {
				warning("child '", name, "' lacks primary dependency sub node");
				return;
			}

			_children.append(_heap, name, macro, Binding(node.sub_node(0U)));
		});

		_children.for_each([&] (Child &child) {

			if (!child.macro_defined()) {
				Macro::Name const name = child.macro_name();
				_config_xml.for_each_sub_node("macro", [&] (Xml_node node) {
					if (node.attribute_value("name", Macro::Name()) == name)
						child.macro(node); });
			}

			if (child.macro_defined() && !child.runtime_defined()) {
				Runtime::Name const name = child.runtime_name();
				_config_xml.for_each_sub_node("runtime", [&] (Xml_node node) {
					if (node.attribute_value("name", Runtime::Name()) == name)
						child.runtime(node); });
			}
		});

		_children.for_each([&] (Child &child) {
			log("child ", child._name, " runtime_defined: ", child.runtime_defined());
			child.connect(_global_bindings, [&] (Binding::Target const &target) {

				log("looking up resource for target ", target.type);
				// XXX lookup resource by target description
				return Weak_ptr<Resource>();
			});
		});
	}
};


void Component::construct(Genode::Env &env) { static Subspace_manager::Main main(env); }
