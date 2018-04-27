/*
 * \brief  Utilities for generating XML
 * \author Norman Feske
 * \date   2018-01-11
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _XML_H_
#define _XML_H_

/* Genode includes */
#include <util/xml_generator.h>
#include <base/log.h>

/* local includes */
#include "types.h"

namespace Sculpt_manager {

	template <typename FN>
	void gen_named_node(Xml_generator &xml,
	                    char const *type, char const *name, FN const &fn)
	{
		xml.node(type, [&] () {
			xml.attribute("name", name);
			fn();
		});
	}

	void gen_named_node(Xml_generator &xml, char const *type, char const *name)
	{
		xml.node(type, [&] () { xml.attribute("name", name); });
	}

	template <size_t N, typename FN>
	void gen_named_node(Xml_generator &xml,
	                    char const *type, String<N> const &name, FN const &fn)
	{
		gen_named_node(xml, type, name.string(), fn);
	}

	template <size_t N>
	void gen_named_node(Xml_generator &xml, char const *type, String<N> const &name)
	{
		gen_named_node(xml, type, name.string());
	}

	template <typename SESSION, typename FN>
	void gen_service_node(Xml_generator &xml, FN const &fn)
	{
		gen_named_node(xml, "service", SESSION::service_name(), fn);
	}

	template <typename SESSION>
	static inline void gen_parent_service(Xml_generator &xml)
	{
		gen_named_node(xml, "service", SESSION::service_name());
	};

	template <typename SESSION>
	static inline void gen_parent_route(Xml_generator &xml)
	{
		gen_named_node(xml, "service", SESSION::service_name(), [&] () {
			xml.node("parent", [&] () { }); });
	}

	static inline void gen_parent_rom_route(Xml_generator  &xml,
	                                        Rom_name const &name,
	                                        Rom_name const &label)
	{
		gen_service_node<Rom_session>(xml, [&] () {
			xml.attribute("label_last", name);
			xml.node("parent", [&] () {
				xml.attribute("label", label); });
		});
	}

	static inline void gen_parent_rom_route(Xml_generator  &xml,
	                                        Rom_name const &name)
	{
		gen_parent_rom_route(xml, name, name);
	}

	template <typename SESSION>
	static inline void gen_provides(Xml_generator &xml)
	{
		xml.node("provides", [&] () {
			gen_named_node(xml, "service", SESSION::service_name()); });
	}

	static inline void gen_common_start_content(Xml_generator   &xml,
	                                            Rom_name  const &name,
	                                            Cap_quota const  caps,
	                                            Ram_quota const  ram)
	{
		xml.attribute("name", name);
		xml.attribute("caps", caps.value);
		gen_named_node(xml, "resource", "RAM", [&] () {
			xml.attribute("quantum", String<64>(Number_of_bytes(ram.value))); });
	}
}

#endif /* _XML_H_ */
