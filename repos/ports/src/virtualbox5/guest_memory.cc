/*
 * \brief  Guest memory management
 * \author Norman Feske
 * \author Alexander Boettcher
 * \date   2020-10-15
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

/* Genode includes */
#include <util/flex_iterator.h>
#include <base/attached_ram_dataspace.h>

/* local includes */
#include <guest_memory.h>


Guest_memory::Pool::Linear_area::Linear_area(Env &env, Bytes const size, Regions &regions)
:
	_env(env), _size(size), _regions(regions)
{
	/* iterator for aligned allocation and attachment of memory */
	Flexpage_iterator iterator(0, size.value, 0, ~0UL, 0);

	for (Flexpage flexpage = iterator.page(); flexpage.valid(); flexpage = iterator.page()) {

		addr_t const flexpage_size = 1UL << flexpage.log2_order;

		for (addr_t allocated = 0; allocated < flexpage_size; ) {

			Bytes const alloc_size { min(128*1024*1024UL,
			                             flexpage_size - allocated) };

			Ram_dataspace_capability ds = _env.ram().alloc(alloc_size.value);

			Phys_addr const guest_phys_addr { flexpage.addr + allocated };

			_rm_connection.retry_with_upgrade(Ram_quota{8192}, Cap_quota{2}, [&] () {
				_region_map.attach_executable(ds,
				                              guest_phys_addr.value,
				                              alloc_size.value); });

			Vmm_addr const vmm_addr { _base.value + guest_phys_addr.value };

			_regions.add(Region{ .base = vmm_addr,
			                     .size = alloc_size,
			                     .ds   = ds });

			allocated += alloc_size.value;
		}
	}

	/* reserve chunk IDs that are special or unused */
	addr_t const unused_id    = CHUNKID_START + size.value/GMM_CHUNK_SIZE;
	addr_t const unused_count = MAX_CHUNK_IDS - unused_id - 1;

	_chunk_ids.reserve(0, CHUNKID_START);
	_chunk_ids.reserve(unused_id, unused_count);
}


Guest_memory::Pool::Regions::Region_elem *
Guest_memory::Pool::Regions::_lookup(Vmm_addr const addr)
{
	for (Region_elem *region = _list.first(); region; region = region->next())
		if (region->contains(addr))
			return region;

	return nullptr;
}


void Guest_memory::Pool::Regions::add(Region const &region)
{
	Region_elem *region_ptr = new Region_elem(region);

	_list.insert(region_ptr);
}


Guest_memory::Vmm_addr Guest_memory::Pool::extend(Bytes size)
{
	Attached_ram_dataspace &ds = *new
		Attached_ram_dataspace(_env.ram(), _env.rm(), size.value);

	Vmm_addr const base { (addr_t)ds.local_addr<void>() };

	_regions.add(Region{ .base = base,
	                     .size = size,
	                     .ds   = ds.cap() });
	return base;
}


Guest_memory::Phys_addr Guest_memory::Pool::phys_addr(Vmm_addr addr) const
{

	bool const in_linear_area = (addr.value >= _linear_area._base.value)
	                         && (addr.value <= _linear_area._end.value);

	if (!in_linear_area)
		throw Out_of_range();

	return Phys_addr { addr.value - _linear_area._base.value };
}


Guest_memory::Vmm_addr Guest_memory::Pool::vmm_addr(Phys_addr addr) const
{
	if (addr.value >= _linear_area._size.value)
		throw Out_of_range();

	return Vmm_addr { _linear_area._base.value + addr.value };

}
