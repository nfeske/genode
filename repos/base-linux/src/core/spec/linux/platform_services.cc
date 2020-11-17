/*
 * \brief  Platform specific services for Linux
 * \author Johannes Kliemann
 * \date   2017-11-08
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 * Copyright (C) 2018 Componolit GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/service.h>

/* core includes */
#include <core_env.h>
#include <platform_services.h>
#include <vm_root.h>


void Genode::platform_add_local_services(Rpc_entrypoint         &ep,
                                         Sliced_heap            &heap,
                                         Registry<Service>      &services,
                                         Trace::Source_registry &trace_sources)
{
	static Vm_root vm_root(ep, heap, core_env().ram_allocator(),
	                       core_env().local_rm(), trace_sources);
	static Core_service<Vm_session_component> vm(services, vm_root);
}
