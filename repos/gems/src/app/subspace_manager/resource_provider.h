/*
 * \brief  Common code for the parent and service-providing child subsystems
 * \author Norman Feske
 * \date   2017-08-23
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _RESOURCE_PROVIDER_H_
#define _RESOURCE_PROVIDER_H_

/* local includes */
#include <managed_list.h>
#include <resource.h>

namespace Subspace_manager { struct Resource_provider; }


struct Subspace_manager::Resource_provider
{
	Managed_list<Resource> _resources;

	/*
	 * Add resource, ignore unknown XML node types
	 */
	void _try_add_resource(Xml_node node)
	{
		Resource::Type const type = Resource::Type::from_xml(node);
		if (!type.valid())
			return;

		Resource::Multi const multi =
			node.attribute_value("multi", false) ? Resource::MULTI : Resource::SINGLE;

		Resource::Label const label =
			node.attribute_value("label", Resource::Label());

		_resources.append(type, multi, label);
	}

	Resource_provider(Allocator &alloc, Xml_node node) : _resources(alloc)
	{
		node.for_each_sub_node([&] (Xml_node sub_node) {
			_try_add_resource(sub_node); });
	}
};

#endif /* _RESOURCE_PROVIDER_H_ */
