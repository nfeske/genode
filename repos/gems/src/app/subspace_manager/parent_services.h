/*
 * \brief  Cached information about services provided by the subspace parent
 * \author Norman Feske
 * \date   2017-08-23
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _PARENT_SERVICES_H_
#define _PARENT_SERVICES_H_

/* local includes */
#include <binding_list.h>

namespace Subspace_manager { struct Parent_services; }


/*
 * The list of parent services is inferred from the resources appearing in the
 * <global> routes and the top-level resources declared in <scene>.
 */
struct Subspace_manager::Parent_services
{
	typedef String<32> Service_name;

	Managed_list<Service_name> _service_names;

	bool _service_name_exists(Service_name const &name)
	{
		bool result = false;
		_service_names.for_each([&] (Service_name const &n) {
			result |= (n == name); });
		return result;
	}

	void _add_service(Resource::Type type)
	{
		if (!type.valid())
			return;

		Service_name const service_name(type);
		if (!_service_name_exists(service_name))
			_service_names.append(service_name);
	}

	Parent_services(Allocator &alloc, Binding_list const &global, Xml_node scene)
	:
		_service_names(alloc)
	{
		global._bindings.for_each([&] (Binding const &binding) {
			_add_service(binding.type()); });

		scene.for_each_sub_node([&] (Xml_node node) {
			_add_service(Resource::Type::from_xml(node)); });
	}
};

#endif /* _PARENT_SERVICES_H_ */
