/*
 * \brief  ARM specific fault info
 * \author Alexander Boettcher
 * \date   2017-07-11
 */

/*
 * Copyright (C) 2017-2023 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

struct Fault_info
{
	Genode::addr_t ip;
	Genode::addr_t pf;
	bool           data_abort;
	bool const     write;
	bool const     align;

	enum {
		IFSR_FAULT = 1,
		IFSR_FAULT_PERMISSION = 0xf,
		DFSR_ALIGN_FAULT = 1UL << 0,
		DFSR_WRITE_FAULT = 1UL << 11
	};

	Fault_info(seL4_MessageInfo_t info)
	:
		ip(seL4_GetMR(0)),
		pf(seL4_GetMR(1)),
		data_abort(seL4_GetMR(2) != IFSR_FAULT),
		/* Instruction Fault Status Register (IFSR) resp. Data FSR (DFSR) */
		write(data_abort && (seL4_GetMR(3) & DFSR_WRITE_FAULT)),
		align(data_abort && (seL4_GetMR(3) == DFSR_ALIGN_FAULT))
	{
		if (!data_abort && seL4_GetMR(3) != IFSR_FAULT_PERMISSION)
			data_abort = true;

		auto const fault_type = seL4_MessageInfo_get_label(info);
		if (fault_type == seL4_Fault_UserException) {
			auto const fault = seL4_getFault(info);
			ip = seL4_Fault_UserException_get_FaultIP(fault);
			pf = seL4_Fault_UserException_get_Number(fault);
		}
	}

	bool exec_fault() const { return !data_abort; }
	bool align_fault() const { return align; }
};
