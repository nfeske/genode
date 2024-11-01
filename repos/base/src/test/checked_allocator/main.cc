/*
 * \brief  Test for 'Checked_allocator' utility
 * \author Norman Feske
 * \date   2024-11-01
 *
 */

/*
 * Copyright (C) 2024 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <base/component.h>
#include <base/log.h>
#include <base/heap.h>
#include <base/checked_allocator.h>


void Component::construct(Genode::Env &env)
{
	using namespace Genode;

	Heap orig_heap { env.ram(), env.rm() };

	static Checked_allocator<100> heap { orig_heap };

	char * const p1 = (char *)heap.alloc(16);
	char * const p2 = (char *)heap.alloc(40000);
	char * const p3 = (char *)heap.alloc(1024*1042);
	char * const p4 = (char *)heap.alloc(64);
	char * const p5 = (char *)heap.alloc(64);

	log(heap);

	p4[-1] = 77;
	heap.free(p4, 64);
	heap.free(p4, 64);

	p5[64] = 11;

	heap.check();

	heap.free(p1, 16);
	heap.free(p2, 40000);
	heap.free(p3, 1024*1042);
	heap.free(p5, 62);

	log(heap);

	log("Test done.");
}
