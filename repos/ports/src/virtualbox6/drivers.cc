/*
 * \brief  VirtualBox host drivers
 * \author Norman Feske
 * \date   2013-08-20
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

/*
 * VirtualBox defines a 'Log' macro, which would interfere with 'Genode::Log'
 * if we didn't include the header here
 */
#include <base/log.h>

/* VirtualBox includes */
#include <VBoxDD.h>

#include "vmm.h"

#define REGISTER(device)                                       \
	do {                                                       \
		rc = pCallbacks->pfnRegister(pCallbacks, &g_##device); \
		if (RT_FAILURE(rc))                                    \
			return rc;                                         \
	} while (0)


extern "C" DECLEXPORT(int) VBoxDriversRegister(PCPDMDRVREGCB pCallbacks, uint32_t u32Version)
{
	int rc = 0;

	REGISTER(DrvMouseQueue);
	REGISTER(DrvKeyboardQueue);
	REGISTER(DrvVD);
	REGISTER(DrvHostDVD);
	REGISTER(DrvHostInterface);
	REGISTER(DrvAUDIO);
	REGISTER(DrvHostNullAudio);
	REGISTER(DrvACPI);
	REGISTER(DrvAcpiCpu);
	REGISTER(DrvVUSBRootHub);
	REGISTER(DrvNamedPipe);
	REGISTER(DrvTCP);
	REGISTER(DrvUDP);
	REGISTER(DrvRawFile);
	REGISTER(DrvChar);
	REGISTER(DrvHostSerial);
	REGISTER(DrvIfTrace);

	return VINF_SUCCESS;
}

