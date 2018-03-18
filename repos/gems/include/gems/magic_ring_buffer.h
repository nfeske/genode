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
	template <typename TYPE, Genode::size_t PAGES>
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

	static size_t capacity() { return CAPACITY; }

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
		map_first = env.rm().attach_at(buffer_ds, map_first, BUFFER_SIZE);
		map_second = env.rm().attach_at(buffer_ds, map_second,  BUFFER_SIZE);
		if ((map_first+BUFFER_SIZE) != map_second) {
			error("failed to map ring buffer to consecutive regions");
			throw Exception();
		}

		buffer = (TYPE *)map_first;
	}

	~Magic_ring_buffer()
	{
		env.rm().detach(map_second);
		env.rm().detach(map_first);
		env.ram().free(buffer_ds);
	}

	size_t write_avail() const
	{
		if      (wpos > rpos) return ((rpos - wpos + CAPACITY) % CAPACITY) - 2;
		else if (wpos < rpos) return rpos - wpos;
		else                  return CAPACITY - 2;
	}

	size_t read_avail() const
	{
		if (wpos > rpos) return wpos - rpos;
		else             return (wpos - rpos + CAPACITY) % CAPACITY;
	}

	TYPE *write_addr() const { return &buffer[wpos]; }
	TYPE *read_addr() const { return &buffer[rpos]; }

	void fill(size_t items) {
		wpos = (wpos+items) % CAPACITY; }

	void drain(size_t items) {
		rpos = (rpos+items) % CAPACITY; }
};

#endif
