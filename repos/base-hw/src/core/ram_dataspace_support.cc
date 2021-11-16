/*
 * \brief  Export RAM dataspace as shared memory object (dummy)
 * \author Martin Stein
 * \author Stefan Kalkowski
 * \date   2012-02-12
 */

/*
 * Copyright (C) 2012-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/log.h>

/* core includes */
#include <ram_dataspace_factory.h>
#include <platform.h>
#include <map_local.h>

using namespace Genode;


void Ram_dataspace_factory::_export_ram_ds(Dataspace_component &) { }


void Ram_dataspace_factory::_revoke_ram_ds(Dataspace_component &) { }


void Ram_dataspace_factory::_clear_ds (Dataspace_component &ds)
{
	size_t page_rounded_size = (ds.size() + get_page_size() - 1) & get_page_mask();

	struct Guard
	{
		Range_allocator &virt_alloc;
		struct { void *virt_ptr = nullptr; };

		Guard(Range_allocator &virt_alloc) : virt_alloc(virt_alloc) { }

		~Guard() { if (virt_ptr) virt_alloc.free(virt_ptr); }

	} guard(platform().region_alloc());

	/* allocate range in core's virtual address space */
	platform().region_alloc().try_alloc(page_rounded_size).with_result(
		[&] (void *ptr) { guard.virt_ptr = ptr; },
		[&] (Range_allocator::Alloc_error e) {
			error("could not allocate virtual address range in core of size ",
			      page_rounded_size, ", error=", e); });

	if (!guard.virt_ptr)
		return;

	/* map the dataspace's physical pages to corresponding virtual addresses */
	size_t num_pages = page_rounded_size >> get_page_size_log2();
	if (!map_local(ds.phys_addr(), (addr_t)guard.virt_ptr, num_pages)) {
		error("core-local memory mapping failed");
		return;
	}

	/* dependent on the architecture, cache maintainance might be necessary */
	Cpu::clear_memory_region((addr_t)guard.virt_ptr, page_rounded_size,
	                         ds.cacheability() != CACHED);

	/* unmap dataspace from core */
	if (!unmap_local((addr_t)guard.virt_ptr, num_pages))
		error("could not unmap core-local address range at ", guard.virt_ptr);
}

