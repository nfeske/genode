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
#include <iprt/err.h>
#include <VBox/sup.h>
#include <SUPLibInternal.h>
#include <SUPDrvIOC.h>

/* local includes */
#include <stub_macros.h>

/* Genode includes */
#include <base/log.h>

static bool const debug = true;

using namespace Genode;


/*******************************
 ** Ioctl interface functions **
 *******************************/

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

	default:

		/*
		 * Ioctl not handled, print diagnostic info and spin.
		 */
		error(__func__, " function=", Hex(opcode));
		STOP
	}

	return VERR_NOT_IMPLEMENTED;
}


int suplibOsIOCtlFast(PSUPLIBDATA pThis, uintptr_t uFunction,
                      uintptr_t idCpu) STOP


int suplibOsPageAlloc(PSUPLIBDATA pThis, size_t cPages, void **ppvPages) STOP


int suplibOsPageFree(PSUPLIBDATA pThis, void *pvPages, size_t cPages) STOP

