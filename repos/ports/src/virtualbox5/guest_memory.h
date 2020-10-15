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

#ifndef _GUEST_MEMORY_H_
#define _GUEST_MEMORY_H_

/* Genode includes */
#include <util/list.h>
#include <util/bit_allocator.h>
#include <base/attached_dataspace.h>
#include <rm_session/connection.h>
#include <region_map/client.h>

/* VirtualBox includes */
#include <VBox/vmm/gmm.h>


namespace Guest_memory {

	using namespace Genode;

	enum { ONE_PAGE_SIZE = 4096 };

	struct Vmm_addr  { addr_t value; };
	struct Phys_addr { addr_t value; };
	struct Bytes     { size_t value; };

	class Pool;
}


class Guest_memory::Pool
{
	public:

		struct Region
		{
			Vmm_addr             base;
			Bytes                size;
			Dataspace_capability ds;
		};

	private:

		enum {
			CHUNKID_PAGE_START = 1,
			CHUNKID_PAGE_END   = 2,
			CHUNKID_START      = CHUNKID_PAGE_END + 1,

			PAGES_SUPERPAGE    = GMM_CHUNK_SIZE / ONE_PAGE_SIZE,
			HANDY_PAGES        = PAGES_SUPERPAGE * (CHUNKID_PAGE_END - CHUNKID_PAGE_START + 1),

			MAX_VM_MEMORY      = 16ULL * 1024 * 1024 * 1024, /* 16 Gb */
			MAX_CHUNK_IDS      = MAX_VM_MEMORY / GMM_CHUNK_SIZE,
		};

		Env &_env;

		class Regions
		{
			private:

				struct Region_elem : Region, List<Region_elem>::Element
				{
					Region_elem(Region const &region) : Region(region) { }

					bool contains(Vmm_addr const addr) const
					{
						return (addr.value >= base.value)
						    && (addr.value <= base.value + size.value - 1);
					}
				};

				List<Region_elem> _list { };

				Region_elem *_lookup(Vmm_addr);

			public:

				void add(Region const &regions);

				template <typename FN>
				void with_region_at(Vmm_addr addr, FN const &fn)
				{
					Region * const region_ptr = _lookup(addr);

					if (region_ptr)
						fn(*region_ptr);
				}

		} _regions { };

		struct Linear_area
		{
			Env &_env;

			Bytes const _size;

			Regions &_regions;

			struct Chunk_ids : Bit_allocator<MAX_CHUNK_IDS>
			{
				void reserve(addr_t bit_start, size_t const num) {
					_reserve(bit_start, num); };
			};

			Chunk_ids _chunk_ids { };

			Rm_connection _rm_connection { _env };

			Region_map_client _region_map { _rm_connection.create(_size.value) };

			Attached_dataspace _ds { _env.rm(), _region_map.dataspace() };

			Vmm_addr const _base { (addr_t)_ds.local_addr<void>() };
			Vmm_addr const _end  { _base.value + _size.value - 1 };

			Linear_area(Env &env, Bytes size, Regions &regions);

		} _linear_area;

	public:

		struct Out_of_range : Exception { };

		Pool(Env &env, Bytes linear_size)
		:
			_env(env), _linear_area(env, linear_size, _regions)
		{ }

		/**
		 * Extend memory pool with new non-linear area
		 */
		Vmm_addr extend(Bytes size);

		/**
		 * Return guest-physical address for given VMM-local address
		 *
		 * The VMM-local address must refer to the linear area.
		 *
		 * \throw Out_of_range  VMM-local address is outside of linear area
		 */
		Phys_addr phys_addr(Vmm_addr) const;

		/**
		 * Return VMM-local address for given guest-physical address
		 *
		 * The guest-physical address must refer to the linear area.
		 *
		 * \throw Out_of_range  guest-physical address is outside of linear area
		 */
		Vmm_addr vmm_addr(Phys_addr) const;

		template <typename FN>
		void with_region_at(Vmm_addr addr, FN const &fn)
		{
			_regions.with_region_at(addr, fn);
		}

		unsigned alloc_chunk_id() { return _linear_area._chunk_ids.alloc(); }

		void free_chunk_id(unsigned id) { _linear_area._chunk_ids.free(id); }
};

#endif /* _GUEST_MEMORY_H_ */
