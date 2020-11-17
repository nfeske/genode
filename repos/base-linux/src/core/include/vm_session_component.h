/*
 * \brief  Linux stub for the VM session interface
 * \author Norman Feske
 * \date   2020-11-17
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__VM_SESSION_COMPONENT_H_
#define _CORE__VM_SESSION_COMPONENT_H_

/* Genode includes */
#include <base/session_object.h>
#include <vm_session/vm_session.h>

/* core-internal includes */
#include <region_map_component.h>

namespace Genode { class Vm_session_component; }


class Genode::Vm_session_component
:
	public Session_object<Vm_session, Vm_session_component>
{
	public:

		using Ram_quota_guard::upgrade;
		using Cap_quota_guard::upgrade;

		Vm_session_component(Rpc_entrypoint &ep, Resources resources,
		                     Label const &label, Diag diag,
		                     Ram_allocator &, Region_map &, unsigned,
		                     Trace::Source_registry &)
		:
			Session_object(ep, resources, label, diag)
		{ }

		~Vm_session_component() { }

		Dataspace_capability _cpu_state(Vcpu_id)
		{
			return Dataspace_capability();
		}

		void _exception_handler(Signal_context_capability, Vcpu_id) { }

		void _run(Vcpu_id) { }

		void _pause(Vcpu_id) { }

		Vcpu_id _create_vcpu(Thread_capability)
		{
			diag("create VCPU");
			return Vcpu_id { };
		}

		void attach(Dataspace_capability, addr_t at, Attach_attr attr) override
		{
			diag("attach at=",  Hex(at),          " "
			     "offset=",     Hex(attr.offset), " "
			     "size=",       Hex(attr.size),   " "
			     "writeable=",  attr.writeable,   " "
			     "executable=", attr.executable);
		}

		void attach_pic(addr_t) override { }

		void detach(addr_t at, size_t size) override
		{
			diag("detach at=", (void *)at, " size=", Hex(size));
		}
};

#endif /* _CORE__VM_SESSION_COMPONENT_H_ */
