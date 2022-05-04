/*
 * \brief  Linux wireless stack
 * \author Josef Soentgen
 * \date   2018-06-29
 */

/*
 * Copyright (C) 2018-2023 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

/* Genode includes */
#include <base/log.h>
#include <base/attached_rom_dataspace.h>

/* local includes */
#include <lx_kit/env.h>

using size_t = Genode::size_t;

struct Firmware_list
{
	char const *requested_name;
	size_t      size;
};


Firmware_list fw_list[] = {
	{ "sof-tgl.ri", 525056 },
	{ "sof-hda-generic-2ch.tplg", 44247 },
};


size_t fw_list_len = sizeof(fw_list) / sizeof(fw_list[0]);


/**********************
 ** linux/firmware.h **
 **********************/

extern "C" int lx_emul_request_firmware_nowait(const char *name, void **dest, size_t *result)
{
	if (!dest || !result)
		return -1;

	/* only try to load known firmware images */
	Firmware_list *fwl = 0;
	for (size_t i = 0; i < fw_list_len; i++) {
		if (Genode::strcmp(name, fw_list[i].requested_name) == 0) {
			fwl = &fw_list[i];
			break;
		}
	}

	if (!fwl) {
		Genode::error("firmware '", name, "' is not in the firmware white list");
		return -1;
	}

	char const *fw_name = fwl->requested_name;

	Genode::Attached_rom_dataspace ds { Lx_kit::env().env, fw_name };

	if (!ds.valid()) {
		Genode::error("could not get firmware ROM dataspace");
		return -1;
	}

	/* use allocator because fw is too big for slab */
	void *data = Lx_kit::env().heap.alloc(fwl->size);
	if (!data)
		return -1;

	Genode::memcpy(data, ds.local_addr<void>(), fwl->size);

	*dest   = data;
	*result = fwl->size;

	return 0;
}


extern "C" void lx_emul_release_firmware(void const *data, size_t size)
{
	Lx_kit::env().heap.free(const_cast<void *>(data), size);
}
