/*
 * \brief  Guest memory management
 * \author Norman Feske
 * \author Christian Helmuth
 * \author Alexander Boettcher
 * \date   2020-11-09
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

/* VirtualBox includes */
#include <VBox/vmm/gmm.h>

/* local includes */
#include <sup_gmm.h>

namespace {
	using Alloc_return = Genode::Range_allocator::Alloc_return;
}


void Sup::Gmm::_add_one_slice()
{
	/* attach new region behind previous region */
	size_t const slice_size  = _slice_size.value;
	addr_t const attach_base = _size_pages.value << PAGE_SHIFT;
	addr_t const attach_end  = attach_base + (slice_size - 1);

	bool const fits_into_map = attach_end <= _map.end.value;

	if (!fits_into_map)
		throw Out_of_range();

	Ram_dataspace_capability ds = _env.ram().alloc(slice_size);

	_map.connection.retry_with_upgrade(Ram_quota{8192}, Cap_quota{2}, [&] () {
		_map.rm.attach_executable(ds, attach_base, slice_size); });

	_slices[_slice_index(Offset{attach_base})] = ds;

	_alloc.add_range(attach_base, slice_size);

	/* update allocation size */
	_size_pages = { (attach_base + slice_size) >> PAGE_SHIFT };
}


void Sup::Gmm::_update_pool_size()
{
	size_t const size_pages     = _size_pages.value;
	size_t const new_size_pages = _reservation_pages.value + _alloc_ex_pages.value;

	if (new_size_pages <= size_pages) {
		warning("Can't shrink guest memory pool from ",
		        size_pages, " to ", new_size_pages, " pages");
		return;
	}

	size_t const map_pages = _map.size.value >> PAGE_SHIFT;

	if (new_size_pages > map_pages) {
		warning("Can't grow guest memory pool beyond ",
		        map_pages, ", requested ", new_size_pages, " pages");
		return;
	}

	/* grow backing-store allocations to accomodate requirements */
	while (_size_pages.value < new_size_pages)
		_add_one_slice();
}


Sup::Gmm::Vmm_addr Sup::Gmm::_alloc_pages(Pages pages)
{
	void *out_addr = nullptr;

	size_t const alloc_size = pages.value << PAGE_SHIFT;

	Alloc_return const result = _alloc.alloc_aligned(alloc_size, &out_addr,
	                                                 log2(alloc_size));
	if (result.error()) {
		error("Gmm allocation failed");
		throw Allocation_failed();
	}

	return Vmm_addr { _map.base.value + (addr_t)out_addr };
}


void Sup::Gmm::reservation_pages(Pages pages)
{
	_reservation_pages = pages;

	_update_pool_size();
}


Sup::Gmm::Vmm_addr Sup::Gmm::alloc_ex(Pages pages)
{
	_alloc_ex_pages = { _alloc_ex_pages.value + pages.value };

	_update_pool_size();

	return _alloc_pages(pages);
}


Sup::Gmm::Vmm_addr Sup::Gmm::alloc_from_reservation(Pages pages)
{
	return _alloc_pages(pages);
}


Sup::Gmm::Page_id Sup::Gmm::page_id(Vmm_addr addr)
{
	bool const inside_map = (addr.value >= _map.base.value)
	                         && (addr.value <= _map.end.value);

	if (!inside_map)
		throw Out_of_range();

	uint32_t const page_index = (uint32_t) (addr.value - _map.base.value) >> PAGE_SHIFT;

	/* NIL_GMM_CHUNKID kept unused - so offset 0 is chunk ID 1 */
	return { page_index + (1u << GMM_CHUNKID_SHIFT) };
}


Sup::Gmm::Vmm_addr Sup::Gmm::vmm_addr(Page_id page_id)
{
	/* NIL_GMM_CHUNKID kept unused - so offset 0 is chunk ID 1 */
	addr_t const addr = _map.base.value
	                  + ((page_id.value - (1u << GMM_CHUNKID_SHIFT)) << PAGE_SHIFT);

	bool const inside_map = (addr >= _map.base.value)
	                         && (addr <= _map.end.value);

	if (!inside_map)
		throw Out_of_range();

	return { addr };
}


void Sup::Gmm::map_to_guest(Vmm_addr from, Guest_addr to, Pages pages, Protection prot)
{
//	to.value &= ~(PAGE_SIZE - 1);
	if (to.value > 0xe0000000)
		log("attach from ", Hex(from.value), " to ", Hex(to.value), " pages=", pages.value);

	/* revoke existing mappings to avoid overmap */
	_vm_connection.detach(to.value, pages.value << PAGE_SHIFT);

	if (prot.none())
		return;

	Vmm_addr const from_end { from.value + (pages.value << PAGE_SHIFT) - 1 };

	for (unsigned i = _slice_index(from); i <= _slice_index(from_end); i++) {

		addr_t const slice_start = i*_slice_size.value;

		addr_t const first_byte_within_slice =
			max(_offset(from).value, slice_start);

		addr_t const last_byte_within_slice =
			min(_offset(from_end).value, slice_start + _slice_size.value -  1);

		Vm_session::Attach_attr const attr
		{
			.offset     = first_byte_within_slice - slice_start,
			.size       = last_byte_within_slice - first_byte_within_slice + 1,
			.executable = prot.executable,
			.writeable  = prot.writeable
		};

		_vm_connection.attach(_slices[i], to.value, attr);

		to.value += attr.size;
	}
}


Sup::Gmm::Gmm(Env &env, Vm_connection &vm_connection)
:
	_env(env), _vm_connection(vm_connection)
{ }
