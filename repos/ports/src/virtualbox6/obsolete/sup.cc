/*
 * \brief  VirtualBox SUPLib supplements
 * \author Norman Feske
 * \date   2013-08-20
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

/* Genode includes */
#include <base/attached_ram_dataspace.h>
#include <trace/timestamp.h>

/* Genode/Virtualbox includes */
#include "sup.h"
#include "vmm.h"

/* VirtualBox includes */
#include <iprt/ldr.h>
#include <iprt/semaphore.h>
#include <VBox/err.h>


SUPR3DECL(bool) SUPR3IsNemSupportedWhenNoVtxOrAmdV(void)
{
	Genode::warning(__func__, " called, not completely implemented");
	return false;
}


SUPR3DECL(int) SUPR3InitEx(bool fUnrestricted, PSUPDRVSESSION *ppSession)
{
	Genode::warning(__func__, " called, forward call to SUPR3Init");
	return SUPR3Init(ppSession);
}


SUPR3DECL(int) SUPR3QueryVTCaps(uint32_t *pfCaps)
{
	AssertPtrReturn(pfCaps, VERR_INVALID_POINTER);

	Genode::warning(__func__, " called, returning empty pfCaps");

	*pfCaps = 0;

	return VINF_SUCCESS;
}


SUPR3DECL(SUPPAGINGMODE) SUPR3GetPagingMode(void)
{
	return sizeof(void *) == 4 ? SUPPAGINGMODE_32_BIT : SUPPAGINGMODE_AMD64_NX;
}


int SUPR3Term(bool) { return VINF_SUCCESS; }


int SUPR3HardenedLdrLoadAppPriv(const char *pszFilename, PRTLDRMOD phLdrMod,
                               uint32_t fFlags, PRTERRINFO pErrInfo)
{
	return RTLdrLoad(pszFilename, phLdrMod);
}


SUPR3DECL(int) SUPR3PageFreeEx(void *pvPages, size_t cPages)
{
	Genode::log(__func__, " pvPages=", pvPages, " pages=", cPages);
	return VINF_SUCCESS;
}


int SUPR3QueryMicrocodeRev(uint32_t *puMicrocodeRev)
{
	return E_FAIL;
}

uint32_t SUPSemEventMultiGetResolution(PSUPDRVSESSION)
{
	return 100000*10; /* called by 'vmR3HaltGlobal1Init' */
}


int SUPSemEventCreate(PSUPDRVSESSION pSession, PSUPSEMEVENT phEvent)
{
	return RTSemEventCreate((PRTSEMEVENT)phEvent);
}


int SUPSemEventClose(PSUPDRVSESSION pSession, SUPSEMEVENT hEvent)
{
	Assert (hEvent);

	return RTSemEventDestroy((RTSEMEVENT)hEvent);
}


int SUPSemEventSignal(PSUPDRVSESSION pSession, SUPSEMEVENT hEvent)
{
	Assert (hEvent);

	return RTSemEventSignal((RTSEMEVENT)hEvent);
}


int SUPSemEventWaitNoResume(PSUPDRVSESSION pSession, SUPSEMEVENT hEvent,
                            uint32_t cMillies)
{
	Assert (hEvent);

	return RTSemEventWaitNoResume((RTSEMEVENT)hEvent, cMillies);
}


int SUPSemEventMultiCreate(PSUPDRVSESSION, PSUPSEMEVENTMULTI phEventMulti)
{
    RTSEMEVENTMULTI sem;

    /*
     * Input validation.
     */
    AssertPtrReturn(phEventMulti, VERR_INVALID_POINTER);

    /*
     * Create the event semaphore object.
     */
	int rc = RTSemEventMultiCreate(&sem);

	static_assert(sizeof(sem) == sizeof(*phEventMulti), "oi");
	*phEventMulti = reinterpret_cast<SUPSEMEVENTMULTI>(sem);
	return rc;
}


int SUPSemEventMultiWaitNoResume(PSUPDRVSESSION, SUPSEMEVENTMULTI event,
                                 uint32_t ms)
{
	RTSEMEVENTMULTI const rtevent = reinterpret_cast<RTSEMEVENTMULTI>(event);
	return RTSemEventMultiWait(rtevent, ms);
}

int SUPSemEventMultiSignal(PSUPDRVSESSION, SUPSEMEVENTMULTI event) {
	return RTSemEventMultiSignal(reinterpret_cast<RTSEMEVENTMULTI>(event)); }


int SUPSemEventMultiReset(PSUPDRVSESSION, SUPSEMEVENTMULTI event) {
	return RTSemEventMultiReset(reinterpret_cast<RTSEMEVENTMULTI>(event)); }


int SUPSemEventMultiClose(PSUPDRVSESSION, SUPSEMEVENTMULTI event) {
	return RTSemEventMultiDestroy(reinterpret_cast<RTSEMEVENTMULTI>(event)); }


int SUPR3CallVMMR0(PVMR0 pVMR0, VMCPUID idCpu, unsigned uOperation,
                   void *pvArg)
{
	Genode::log(__func__, ": uOperation=", uOperation);
	return VERR_GENERAL_FAILURE;
}


void genode_VMMR0_DO_GVMM_CREATE_VM(PSUPVMMR0REQHDR pReqHdr)
{
	GVMMCREATEVMREQ &req = reinterpret_cast<GVMMCREATEVMREQ &>(*pReqHdr);

	size_t const cCpus = req.cCpus;

	/*
	 * Allocate and initialize VM struct
	 *
	 * The VM struct is followed by the variable-sizedA array of VMCPU
	 * objects.
	 *
	 * VM struct must be page-aligned, which is checked at least in
	 * PDMR3CritSectGetNop().
	 */
	size_t const cbVM = sizeof(VMCPU);

	static Genode::Attached_ram_dataspace vm(genode_env().ram(),
	                                         genode_env().rm(),
	                                         cbVM);
	Assert (vm.size() >= cbVM);

	VM *pVM = vm.local_addr<VM>();
	Genode::memset(pVM, 0, cbVM);

	pVM->enmVMState       = VMSTATE_CREATING;
	pVM->pVMRC            = (RTGCUINTPTR)pVM;
	pVM->pSession         = req.pSession;
	pVM->cbSelf           = cbVM;
	pVM->cCpus            = cCpus;
	pVM->uCpuExecutionCap = 100;  /* expected by 'vmR3CreateU()' */

	using Genode::log;
	log("cCpus=", cCpus);

	for (uint32_t i = 0; i < cCpus; i++) {
		log("pVM=", pVM);
		log("apCpusR3[", i, "]=", pVM->apCpusR3[i]);
		pVM->apCpusR3[i]->pVMR3           = pVM;
		pVM->apCpusR3[i]->idHostCpu       = NIL_RTCPUID;
		pVM->apCpusR3[i]->hNativeThreadR0 = NIL_RTNATIVETHREAD;
	}

	pVM->apCpusR3[0]->hNativeThreadR0 = RTThreadNativeSelf();

	/* out parameters of the request */
	req.pVMR3 = pVM;
}


void genode_VMMR0_DO_GVMM_REGISTER_VMCPU(PVMR0 pVMR0, VMCPUID idCpu)
{
	PVM pVM = reinterpret_cast<PVM>(pVMR0);
	pVM->apCpusR3[idCpu]->hNativeThreadR0 = RTThreadNativeSelf();
}


HRESULT genode_check_memory_config(ComObjPtr<Machine>,
                                   size_t const memory_vmm)
{
	/* Request max available memory */
	size_t const memory_available = genode_env().pd().avail_ram().value;

	if (memory_vmm <= memory_available)
		return S_OK;

	size_t const MB = 1024*1024;

	Genode::error("Available memory too low to start the VM - available: ",
	              memory_available/MB, " MB < ", memory_vmm/MB, " MB requested");
	return E_FAIL;
}
