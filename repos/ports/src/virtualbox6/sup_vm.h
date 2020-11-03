/*
 * \brief  Suplib VM implementation
 * \author Norman Feske
 * \author Christian Helmuth
 * \date   2020-10-12
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

#ifndef _SUP_VM_H_
#define _SUP_VM_H_

/* VirtualBox includes */
#include <PGMInternal.h>
#include <VBox/vmm/vm.h>

/* local includes */
#include <sup.h>

namespace Sup { struct Vm; }

struct Sup::Vm : VM
{
	void init(PSUPDRVSESSION psession, Cpu_count cpu_count)
	{
		enmVMState       = VMSTATE_CREATING;
		pVMR0ForCall     = (PVMR0)this;
		pSession         = psession;
		cbSelf           = sizeof(VM);
		cCpus            = cpu_count.value;
		uCpuExecutionCap = 100;  /* expected by 'vmR3CreateU()' */

		for (uint32_t i = 0; i < cpu_count.value; ++i) {
			apCpusR3[i] = (VMCPU *)RTMemAllocZ(sizeof(VMCPU));
			log(this, ": apCpusR3[", i, "]=", apCpusR3[i]);

			apCpusR3[i]->pVMR3           = this;
			apCpusR3[i]->idHostCpu       = NIL_RTCPUID;
			apCpusR3[i]->hNativeThreadR0 = NIL_RTNATIVETHREAD;
		}

//		pVM->apCpusR3[0]->hNativeThreadR0 = RTThreadNativeSelf();
	}

	static Vm & create(PSUPDRVSESSION psession, Cpu_count cpu_count)
	{
		/*
		 * Allocate and initialize VM struct
		 *
		 * The original R0 GVM struct inherits VM and is also followed by the
		 * variable-sized array of GVMCPU objects. We only allocate and maintain
		 * the R3 VM struct, which must be page-aligned and contains an array of
		 * VMCPU pointers in apCpusR3.
		 */
		Vm *vm_ptr = (Vm *)RTMemPageAllocZ(sizeof(Vm));

		vm_ptr->init(psession, cpu_count);

		return *vm_ptr;
	}
};

#endif /* _SUP_VM_H_ */

