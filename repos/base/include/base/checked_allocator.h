/*
 * \brief  Utility for detecting heap overflows
 * \author Norman Feske
 * \date   2024-11-01
 */

/*
 * Copyright (C) 2024 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__BASE__CHECKED_ALLOCATOR_H_
#define _INCLUDE__BASE__CHECKED_ALLOCATOR_H_

#include <util/bit_allocator.h>
#include <base/allocator.h>

namespace Genode { template <unsigned> class Checked_allocator; }


template <unsigned N>
class Genode::Checked_allocator : public Allocator, Noncopyable
{
	private:

		Allocator &_alloc;

		Bit_allocator<N> _bits { };

		struct Canary
		{
			unsigned long value;

			bool operator == (Canary const &other) const { return other.value == value; };
		};

		struct Id { unsigned value; };

		struct Item
		{
			Id     id;
			void  *ptr;
			size_t size; /* size of original allocation w/o canaries */
			void  *caller;;

			unsigned long _tagged_id(uint16_t tag) const
			{
				unsigned long result = tag;
				result = result << (sizeof(unsigned long)*8 - 16);
				result |= id.value;
				return result;
			}

			Canary begin_canary() const { return { _tagged_id(0x1111) }; }
			Canary end_canary()   const { return { _tagged_id(0xffff) }; }

			bool begin_intact() const
			{
				return begin_canary() == *(Canary *)(addr_t(ptr) - sizeof(Canary));
			}

			bool end_intact() const
			{
				return end_canary() == *(Canary *)(addr_t(ptr) + size);
			}

			bool intact() const { return !ptr || (begin_intact() && end_intact()); }

			bool used() const { return ptr != nullptr; }

			void print(Output &out) const
			{
				if (!ptr)
					return;

				using Genode::print;
				print(out, Hex_range(addr_t(ptr), size),
				      " caller=", caller, " id=", id.value, " size=", size);
				if (!begin_intact()) print(out, ", begin corrupt");
				if (!end_intact())   print(out, ", end corrupt");
			}
		};

		Item _items[N] { };

		unsigned _count = 0;

	public:

		Checked_allocator(Allocator &alloc) : _alloc(alloc) { }

		Alloc_result try_alloc(size_t size) override
		{
			void * const caller = __builtin_return_address(0);

			return _alloc.try_alloc(size + 2*sizeof(Canary)).convert<Alloc_result>(
				[&] (void *ptr) -> Alloc_result {
					try {
						Id     const id { ++_count };
						addr_t const i    = _bits.alloc(0);
						addr_t const addr = addr_t(ptr);
						void * const ptr  = (void *)(addr + sizeof(Canary));

						Item &item = _items[i];
						item = Item { .id = id, .ptr = ptr, .size = size, .caller = caller };

						*(Canary *)(addr)                         = item.begin_canary();
						*(Canary *)(addr + sizeof(Canary) + size) = item.end_canary();

						return ptr;

					} catch (... /* Out_of_indices */) {
						error("Checked_allocator recording limit reached");
						return Alloc_error::DENIED;
					}
				},
				[&] (Alloc_error e) -> Alloc_result { return e; }
			);
		}

		void free(void *ptr, size_t size) override
		{
			bool found = false;
			for (Item &item : _items) {
				if (item.ptr == ptr) {
					if (found)
						warning("allocation recorded twice: ", item);

					found = true;
					if (!item.intact())
						warning("corrupt at free time: ", item);

					if (size && item.size != size)
						warning("free called with wrong size, "
						        "expected=", item.size, " got=", size);

					_alloc.free((void *)(addr_t(ptr) - sizeof(Canary)),
					            size + 2*sizeof(Canary));

					item = { };
				}
			}
			if (!found)
				warning("possible double free "
				        "ptr=", ptr, " size=", size);
		}

		void check() const
		{
			for (Item const &item : _items) {
				if (!item.intact())
					warning("corruption ", item);
			}
		}

		void print(Output &out) const
		{
			for (Item const &item : _items)
				if (item.used())
					Genode::print(out, item, "\n");
		}

		bool   need_size_for_free()  const override { return _alloc.need_size_for_free(); }
		size_t consumed()            const override { return _alloc.consumed();     }
		size_t overhead(size_t size) const override { return _alloc.overhead(size); }
};

#endif /* _INCLUDE__BASE__CHECKED_ALLOCATOR_H_ */
