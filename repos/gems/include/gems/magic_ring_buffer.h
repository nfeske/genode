/*
 * \brief  Region magic ring buffer
 * \author Emery Hemingway
 * \date   2018-02-01
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__GEMS__RING_BUFFER_H_
#define _INCLUDE__GEMS__RING_BUFFER_H_

/* Genode includes */
#include <base/attached_ram_dataspace.h>

namespace Genode {
	template <typename TYPE, Genode::size_t SIZE>
	struct Magic_ring_buffer;
}

template <typename TYPE, Genode::size_t SIZE>
class Genode::Magic_ring_buffer
{
	private:

		Magic_ring_buffer(Magic_ring_buffer const &);
		Magic_ring_buffer &operator = (Magic_ring_buffer const &);

		enum {
			BUFFER_SIZE = align_addr(SIZE, 12),
			CAPACITY = BUFFER_SIZE / sizeof(TYPE)
		};

		Genode::Env &env;

		addr_t map_first = 0;
		addr_t map_second = 0;

		TYPE *buffer = nullptr;

		size_t wpos = 0;
		size_t rpos = 0;

		Ram_dataspace_capability buffer_ds { };

	public:

		/**
		 * Ring capacity of TYPE items
		 */
		static size_t capacity() { return CAPACITY; }

		/**
		 * Constructor
		 *
		 * \param TYPE  Ring item type, size of type must be a
		 *              power of two and less than the page size
		 * \param SIZE  Size of items in ring, may be rounded up
		 *              to the next page boundry
		 *
		 * \param env   Env for dataspace allocation and mapping
		 *
		 * \throw Region_map::Region_conflict
		 * \throw Out_of_ram
		 * \throw Out_of_caps
		 *
		 */
		Magic_ring_buffer(Genode::Env &env) : env(env)
		{
			{
				/* a hack to find the right sized void in the address space */
				Attached_ram_dataspace filler(env.pd(), env.rm(), BUFFER_SIZE*2);
				map_first = (addr_t)filler.local_addr<TYPE>();
				map_second = map_first+BUFFER_SIZE;
			}

			buffer_ds = env.pd().alloc(BUFFER_SIZE);

			/* attach the buffer in two consecutive regions */
			try {
				map_first = env.rm().attach_at(buffer_ds, map_first, BUFFER_SIZE);
				map_second = env.rm().attach_at(buffer_ds, map_second,  BUFFER_SIZE);
			} catch (...) {
				Genode::error("ring buffer dataspace double mapping failed");
				if (map_first) env.rm().detach(map_first);
				throw;
			}

			if ((map_first+BUFFER_SIZE) != map_second) {
				error("failed to map ring buffer to consecutive regions");
				throw Region_map::Region_conflict();
			}

			buffer = (TYPE *)map_first;
		}

		~Magic_ring_buffer()
		{
			env.rm().detach(map_second);
			env.rm().detach(map_first);
			env.ram().free(buffer_ds);
		}

		/**
		 * Number of items that may be written to ring
		 */
		size_t write_avail() const
		{
			if      (wpos > rpos) return ((rpos - wpos + CAPACITY) % CAPACITY) - 2;
			else if (wpos < rpos) return rpos - wpos;
			else                  return CAPACITY - 2;
		}

		/**
		 * Number of items that may be read from ring
		 */
		size_t read_avail() const
		{
			if (wpos > rpos) return wpos - rpos;
			else             return (wpos - rpos + CAPACITY) % CAPACITY;
		}

		/**
		 * Pointer to ring write address
		 */
		TYPE *write_addr() const { return &buffer[wpos]; }

		/**
		 * Pointer to ring read address
		 */
		TYPE  *read_addr() const { return &buffer[rpos]; }

		/**
		 * Advance the ring write pointer
		 */
		void fill(size_t items) {
			wpos = (wpos+items) % CAPACITY; }

		/**
		 * Advance the ring read pointer
		 */
		void drain(size_t items) {
			rpos = (rpos+items) % CAPACITY; }
};

#endif
