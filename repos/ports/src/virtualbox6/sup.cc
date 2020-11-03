/*
 * \brief  Genode backend for VirtualBox Suplib
 * \author Norman Feske
 * \date   2020-10-12
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

/* Genode includes */
#include <base/env.h>
#include <base/log.h>

/* VirtualBox includes */
#include <VBox/err.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/gvmm.h>
#include <SUPLibInternal.h>
#include <SUPDrvIOC.h>

/* local includes */
#include <init.h>
#include <sup_drv.h>
#include <sup_vm.h>
#include <stub_macros.h>

static bool const debug = true;

using namespace Genode;

static Sup::Drv *sup_drv;

void Sup::init(Env &env)
{
	sup_drv = new Sup::Drv(env);
}


/*******************************
 ** Ioctl interface functions **
 *******************************/

/* XXX init in COOKIE */
struct SUPDRVSESSION
{
	SUPDRVSESSION() { }
};

static int ioctl_cookie(SUPCOOKIE &request)
{
	warning(__PRETTY_FUNCTION__, " misses session-object creation");

	request.Hdr.rc = VINF_SUCCESS;
	request.u.Out.u32SessionVersion = SUPDRV_IOC_VERSION;

	return VINF_SUCCESS;
}


static int ioctl_query_funcs(SUPQUERYFUNCS &request)
{
	warning(__PRETTY_FUNCTION__, " reports zero functions");

	request.Hdr.rc = VINF_SUCCESS;
	request.u.Out.cFunctions = 0;

	return VINF_SUCCESS;
}


static int ioctl_gip_map(SUPGIPMAP &request)
{
	request.Hdr.rc = VINF_SUCCESS;
	request.u.Out.HCPhysGip = 0;
	request.u.Out.pGipR3    = sup_drv->gip();
	request.u.Out.pGipR0    = 0;

	return VINF_SUCCESS;
}


static int ioctl_vt_caps(SUPVTCAPS &request)
{
	switch (sup_drv->cpu_virt()) {
	case Sup::Drv::Cpu_virt::VMX:
		request.Hdr.rc      = VINF_SUCCESS;
		request.u.Out.fCaps = SUPVTCAPS_VT_X | SUPVTCAPS_NESTED_PAGING;
		break;
	case Sup::Drv::Cpu_virt::SVM:
		request.Hdr.rc      = VINF_SUCCESS;
		request.u.Out.fCaps = SUPVTCAPS_AMD_V | SUPVTCAPS_NESTED_PAGING;
		break;
	case Sup::Drv::Cpu_virt::NONE:
		request.Hdr.rc      = VERR_UNSUPPORTED_CPU;
		request.u.Out.fCaps = 0;
		break;
	}

	/*
	 * XXX are the following interesting?
	 * SUPVTCAPS_VTX_VMCS_SHADOWING
	 * SUPVTCAPS_VTX_UNRESTRICTED_GUEST
	 */

	return VINF_SUCCESS;
}


static int vmmr0_gvmm_create_vm(GVMMCREATEVMREQ &request)
{
	warning(__PRETTY_FUNCTION__
	       , " pSession=", request.pSession
	       , " cCpus=", request.cCpus
	       , " pVMR3=", request.pVMR3
	       , " pVMR0=", request.pVMR0
	       );

	Sup::Cpu_count cpu_count { request.cCpus };

	request.pVMR3 = &Sup::Vm::create(request.pSession, cpu_count);
	request.pVMR0 = (PVMR0)request.pVMR3;

	return VINF_SUCCESS;
}


static int vmmr0_pgm_pool_grow(PVMR0 pvmr0)
{
	warning(__PRETTY_FUNCTION__
	       , " pvmr0=", (void*)pvmr0
//	       , " PGMPOOL_CFG_MAX_GROW=", PGMPOOL_CFG_MAX_GROW
	       );

	Sup::Vm     &vm   = *(Sup::Vm *)pvmr0;
	PGMPOOL     &pool = *vm.pgm.s.pPoolR3;
	PGMPOOLPAGE &page = pool.aPages[PGMPOOL_IDX_FIRST];

	/* XXX */ void *a_page = RTMemPageAllocZ(4096);

	page.pvPageR3  = (R3PTRTYPE(void *))a_page;
	page.pvPageR0  = (R0PTRTYPE(void *))a_page;
	page.enmKind   = PGMPOOLKIND_FREE;
	pool.iFreeHead = PGMPOOL_IDX_FIRST;

	error("-----"
	     , " vm.pgm.s.pPoolR3=", vm.pgm.s.pPoolR3
	     , " cMaxPages=", pool.cMaxPages
	     , " page.enmKind=", page.enmKind
	     , " iFreeHead=", pool.iFreeHead
	     );

	return VINF_SUCCESS;
}


static int vmmr0_gmm_initial_reservation(PVMR0 pvmr0)
{
	warning(__PRETTY_FUNCTION__
	       , " pvmr0=", (void*)pvmr0
	       );
	return VINF_SUCCESS;
}


static int ioctl_call_vmmr0(SUPCALLVMMR0 &request)
{
	warning(__PRETTY_FUNCTION__
	       , " cbIn=", request.Hdr.cbIn
	       , " uOperation=", request.u.In.uOperation
	       , " u64Arg=", request.u.In.u64Arg
	       , " pVMR0=", (void*)request.u.In.pVMR0
	       );

	VMMR0OPERATION const operation = VMMR0OPERATION(request.u.In.uOperation);

	switch (operation) {
	case VMMR0_DO_GVMM_CREATE_VM:
		request.Hdr.rc = vmmr0_gvmm_create_vm(*(GVMMCREATEVMREQ *)request.abReqPkt);
		return VINF_SUCCESS;

	case VMMR0_DO_PGM_POOL_GROW:
		request.Hdr.rc = vmmr0_pgm_pool_grow(request.u.In.pVMR0);
		return VINF_SUCCESS;

	case VMMR0_DO_GMM_INITIAL_RESERVATION:
		request.Hdr.rc = vmmr0_gmm_initial_reservation(request.u.In.pVMR0);
		return VINF_SUCCESS;

	default:
		error(__func__, " operation=", (int)operation);
		request.Hdr.rc = VERR_NOT_IMPLEMENTED;
		STOP
	}

	return VERR_NOT_IMPLEMENTED;
}


int ioctl_get_hmvirt_msrs(SUPGETHWVIRTMSRS &request)
{
	warning(__PRETTY_FUNCTION__
	       , " fForce=", request.u.In.fForce
	       );

	request.Hdr.rc = VINF_SUCCESS;
	::memset(&request.u.Out, 0, sizeof(request.u.Out));

	return VINF_SUCCESS;
}


int ioctl_ucode_rev(SUPUCODEREV &request)
{
	warning(__PRETTY_FUNCTION__);

	/* fake most recent revision possible */

	request.Hdr.rc = VINF_SUCCESS;
	request.u.Out.MicrocodeRev = ~0u;

	return VINF_SUCCESS;
}


int ioctl_get_paging_mode(SUPGETPAGINGMODE &request)
{
	warning(__PRETTY_FUNCTION__);

	request.Hdr.rc = VINF_SUCCESS;
	request.u.Out.enmMode = sizeof(long) == 32 ? SUPPAGINGMODE_32_BIT_GLOBAL
	                                           : SUPPAGINGMODE_AMD64_GLOBAL_NX;

	return VINF_SUCCESS;
}


/*********************************
 ** VirtualBox suplib interface **
 *********************************/

int suplibOsInit(PSUPLIBDATA pThis, bool fPreInited, bool fUnrestricted,
                 SUPINITOP *penmWhat, PRTERRINFO pErrInfo) TRACE(VINF_SUCCESS)


int suplibOsTerm(PSUPLIBDATA pThis) TRACE(VINF_SUCCESS)


int suplibOsInstall(void) TRACE(VERR_NOT_IMPLEMENTED)


int suplibOsUninstall(void) TRACE(VERR_NOT_IMPLEMENTED)


int suplibOsIOCtl(PSUPLIBDATA pThis, uintptr_t opcode, void *req, size_t len)
{
	switch (SUP_CTL_CODE_NO_SIZE(opcode)) {

	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_COOKIE):               return ioctl_cookie(*(SUPCOOKIE *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_QUERY_FUNCS()):        return ioctl_query_funcs(*(SUPQUERYFUNCS *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_GIP_MAP):              return ioctl_gip_map(*(SUPGIPMAP *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_VT_CAPS):              return ioctl_vt_caps(*(SUPVTCAPS *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_CALL_VMMR0_NO_SIZE()): return ioctl_call_vmmr0(*(SUPCALLVMMR0 *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_GET_HWVIRT_MSRS):      return ioctl_get_hmvirt_msrs(*(SUPGETHWVIRTMSRS *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_UCODE_REV):            return ioctl_ucode_rev(*(SUPUCODEREV *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_GET_PAGING_MODE):      return ioctl_get_paging_mode(*(SUPGETPAGINGMODE *)req);

	default:

		/*
		 * Ioctl not handled, print diagnostic info and spin.
		 * opcode number in lowest 7 bits
		 */
		error(__func__, " function=", opcode & 0x7f);
		STOP
	}

	return VERR_NOT_IMPLEMENTED;
}


int suplibOsIOCtlFast(PSUPLIBDATA pThis, uintptr_t uFunction,
                      uintptr_t idCpu) STOP


int suplibOsPageAlloc(PSUPLIBDATA pThis, size_t cPages, void **ppvPages) STOP


int suplibOsPageFree(PSUPLIBDATA pThis, void *pvPages, size_t cPages) STOP

