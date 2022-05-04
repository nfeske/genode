/*
 * \brief  Linux's 'vmap' using a managed dataspace
 * \author Sebastian Sumpf
 * \date   2022-05-31
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

#include <lx_kit/env.h>
#include <lx_kit/memory.h>

#include <region_map/client.h>
#include <rm_session/connection.h>

using addr_t = Genode::addr_t;
using size_t = Genode::size_t;

namespace Lx_emul { class Vmap; }

class Lx_emul::Vmap
{
	private:

		Lx_kit::Env &_lx_env { Lx_kit::env() };

		size_t _size;

		Genode::Rm_connection     _rm_connection { _lx_env.env };
		Genode::Region_map_client _rm { _rm_connection.create(_size) };

		addr_t _virt_addr { _attach() };

		Genode::Id_space<Vmap>::Element const _elem;

		addr_t _attach()
		{
			using Region_map = Genode::Region_map;

			addr_t addr = 0;
			Region_map::Attr attr { };
			attr.writeable = true;
			_rm.attach(_rm.dataspace(), attr).with_result(
				[&] (Region_map::Range range) { addr = range.start; },
				[&] (Region_map::Attach_error e) {
					if (e == Region_map::Attach_error::OUT_OF_RAM)  throw Genode::Out_of_ram();
					if (e == Region_map::Attach_error::OUT_OF_CAPS) throw Genode::Out_of_caps();
					throw Genode::Attached_dataspace::Region_conflict();
				}
			);

			return addr;
		}

	public:

		Vmap(size_t size,
		     Genode::Id_space<Vmap> &space)
		:
		  _size(size),
		  _elem (*this, space, Genode::Id_space<Vmap>::Id { .value = _virt_addr })
		{ }

		addr_t virt_region_start(void *virt_addr)
		{
			addr_t vm_start =_lx_env.memory.virt_region_start(virt_addr);
			if (!vm_start)
				vm_start = _lx_env.uncached_memory.virt_region_start(virt_addr);

			if (!vm_start) {
				Genode::error(__func__, " could not find region for virt_addr ", virt_addr);
		}

			return vm_start;
		}

		Genode::Dataspace_capability attached_dataspace_cap(void *virt_addr)
		{
			Genode::Dataspace_capability cap  =
				_lx_env.memory.attached_dataspace_cap(virt_addr);
			if (cap.valid() == false)
				cap = _lx_env.uncached_memory.attached_dataspace_cap(virt_addr);
			return cap;
		}

		int attach(void *virt_addr, unsigned long offset)
		{
			Genode::Dataspace_capability cap = attached_dataspace_cap(virt_addr);
			if (cap.valid() == false)
				return -1;

			addr_t vm_start = virt_region_start(virt_addr);
			if (!vm_start)
				return -2;

			_rm_connection.retry_with_upgrade(Genode::Ram_quota{4096},
			                                  Genode::Cap_quota{8}, [&]() {
				_rm.attach(cap, {
					.size       = 0x1000, .offset    = offset,
					.use_at     = true,   .at        = (addr_t)virt_addr - vm_start,
					.executable = false,  .writeable = true });
			});

			return 0;
		}

		addr_t virt_addr() const { return _virt_addr; }
};


extern "C" int lx_vmap_attach(void *vmap, void *virt_addr, unsigned long offset)
{
	if (!vmap) return -3;

	Lx_emul::Vmap *map = static_cast<Lx_emul::Vmap *>(vmap);

	return map->attach(virt_addr, offset);
}


extern "C" void *lx_vmap_address(void *vmap)
{
	if (!vmap) return nullptr;

	Lx_emul::Vmap *map = static_cast<Lx_emul::Vmap *>(vmap);

	return (void *)map->virt_addr();
}


static Genode::Id_space<Lx_emul::Vmap>  &vmap_registry()
{
	static Genode::Id_space<Lx_emul::Vmap> space { };
	return space;
}


extern "C" void *lx_vmap_create(unsigned size)
{
	return new (Lx_kit::env().heap) Lx_emul::Vmap(size, vmap_registry());
}


extern "C" void lx_vmap_destroy(void *virt_addr)
{
	using Id_space = Genode::Id_space<Lx_emul::Vmap>;

	Lx_emul::Vmap *vmap = nullptr;
	Id_space::Id id { .value = (unsigned long)virt_addr };

	try {
		vmap_registry().apply<Lx_emul::Vmap>(id, [&](Lx_emul::Vmap &map) {
			vmap = &map; });
	} catch (Id_space::Unknown_id) { }

	if (vmap)
		destroy(Lx_kit::env().heap, static_cast<Lx_emul::Vmap *>(vmap));
}
