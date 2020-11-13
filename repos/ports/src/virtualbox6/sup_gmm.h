/*
 * \brief  Suplib GMM implementation
 * \author Norman Feske
 * \author Christian Helmuth
 * \date   2020-11-09
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

#ifndef _SUP_GMM_H_
#define _SUP_GMM_H_

/* Genode includes */
#include <base/allocator_avl.h>
#include <base/attached_dataspace.h>
#include <rm_session/connection.h>
#include <region_map/client.h>
#include <libc/allocator.h>

/* local includes */
#include <sup.h>


namespace Sup { struct Gmm; }

/**
 * Global (guest-memory) manager.(GMM)
 *
 * Layers in this pool are (top-down)
 *
 * - Page-sized allocation from Allocator_avl
 * - Linear map of 32 GiB in VMM address space (Vmm_addr)
 * - VirtualBox GMM chunks of GMM_CHUNK_SIZE (2 MiB) filled with pages and
 *   referenced with PAGEIDs (offset in linear area)
 * - Slices of 128 MiB RAM dataspaces as backing store
 *
 * Notes
 *
 * - expect that guest-physical address not required here
 * - NIL_GMM_CHUNKID kept unused - so offset 0 is chunk ID 1
 * - we have to allocate from pool - so we need an allocator
 */
class Sup::Gmm
{
	public:

		struct Vmm_addr { addr_t value; };
		struct Offset   { addr_t value; };
		struct Bytes    { size_t value; };
		struct Pages    { size_t value; };
		struct Page_id  { uint32_t value; }; /* CHUNKID | PAGEIDX */

	private:

		Env &_env;

		static constexpr Bytes _slice_size {     128*1024*1024ul };
		static constexpr Bytes _map_size   { 32*1024*1024*1024ul };
		static constexpr auto  _num_slices { _map_size.value / _slice_size.value };

		Dataspace_capability _slices[_num_slices];

		Bytes _size { 0 }; /* current size of backing-store allocations */

		struct Map
		{
			Bytes const size;

			Rm_connection connection;

			Region_map_client rm { connection.create(size.value) };

			Attached_dataspace ds;

			Vmm_addr const base { (addr_t)ds.local_addr<void>() };
			Vmm_addr const end  { base.value + size.value - 1 };

			Map(Env &env, Bytes size)
			: size(size), connection(env), ds(env.rm(), rm.dataspace())
			{ }

		} _map { _env, _map_size };

		void _add_one_slice();

		unsigned _slice_index(Offset offset)
		{
			unsigned const index = offset.value / _slice_size.value;

			if (index > _num_slices)
				throw Out_of_range();

			return index;
		}

		Libc::Allocator _md_alloc;
		Allocator_avl   _alloc { &_md_alloc };

	public:

		struct Out_of_range : Exception { };

		Gmm(Env &env);

		/**
		 * Update the size of max  backing store allocations
		 */
		void pool_size(Pages);

		/**
		 * Allocate pages from pool
		 */
		Vmm_addr alloc(Pages);

		/**
		 * Free pages in pool
		 */
		void free(Vmm_addr, Pages);

		/**
		 * Get PAGEID for VMM address inside linear area
		 */
		Page_id page_id(Vmm_addr);
};

#endif /* _SUP_GMM_H_ */
