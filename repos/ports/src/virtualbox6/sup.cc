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

/* VirtualBox includes */
#include <VBox/err.h>
#include <VBox/sup.h>
#include <SUPLibInternal.h>
#include <SUPDrvIOC.h>

/* local includes */
#include <init.h>
#include <sup_drv.h>
#include <stub_macros.h>

/* Genode includes */
#include <base/env.h>
#include <base/log.h>

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
	log(__PRETTY_FUNCTION__, " called");

	request.Hdr.rc = VINF_SUCCESS;
	request.u.Out.u32SessionVersion = SUPDRV_IOC_VERSION;

	return VINF_SUCCESS;
}


static int ioctl_query_funcs(SUPQUERYFUNCS &request)
{
	warning(__PRETTY_FUNCTION__, " is incomplete");

	request.Hdr.rc = VINF_SUCCESS;
	request.u.Out.cFunctions = 0;

	return VINF_SUCCESS;
}


static int ioctl_gip_map(SUPGIPMAP &request)
{
	log(__PRETTY_FUNCTION__, " called");

	request.Hdr.rc = VINF_SUCCESS;
	request.u.Out.HCPhysGip = 0;
	request.u.Out.pGipR3    = sup_drv->gip();
	request.u.Out.pGipR0    = 0;

	return VINF_SUCCESS;
}


static int ioctl_vt_caps(SUPVTCAPS &request)
{
	log(__PRETTY_FUNCTION__, " called");

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
	switch (opcode) {

	case SUP_IOCTL_COOKIE:        return ioctl_cookie(*(SUPCOOKIE *)req);
	case SUP_IOCTL_QUERY_FUNCS(): return ioctl_query_funcs(*(SUPQUERYFUNCS *)req);
	case SUP_IOCTL_GIP_MAP:       return ioctl_gip_map(*(SUPGIPMAP *)req);
	case SUP_IOCTL_VT_CAPS:       return ioctl_vt_caps(*(SUPVTCAPS *)req);

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

