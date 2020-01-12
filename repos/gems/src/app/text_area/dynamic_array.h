/*
 * \brief  Dynamically growing array
 * \author Norman Feske
 * \date   2020-01-12
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _DYNAMIC_ARRAY_H_
#define _DYNAMIC_ARRAY_H_

/* Genode includes */
#include <base/allocator.h>

namespace Text_view {

	using namespace Genode;

	template <typename>
	struct Dynamic_array;
}


template <typename ET>
struct Text_view::Dynamic_array
{
	public:

		struct Index { unsigned value; };

	private:

		Allocator &_alloc;

		struct Element { ET *ptr; };

		Element *_array = nullptr;

		unsigned _capacity    = 0;
		unsigned _upper_bound = 0; /* index after last used element */

		bool _index_valid(Index at) const
		{
			return (at.value < _upper_bound) && (_array[at.value].ptr != nullptr);
		}

		/*
		 * Noncopyable
		 */
		Dynamic_array(Dynamic_array const &);
		void operator = (Dynamic_array const &);

	public:

		Dynamic_array(Allocator &alloc) : _alloc(alloc) { }

		~Dynamic_array()
		{
			if (!_array)
				return;

			clear();

			_alloc.free(_array, _capacity*sizeof(Element));
		}

		void clear()
		{
			if (_upper_bound > 0)
				for (unsigned i = _upper_bound; i > 0; i--)
					destruct(Index{i - 1});
		}

		template <typename... ARGS>
		void insert(Index at, ARGS &&... args)
		{
			/*
			 * Construct element before touching the array to leave the array
			 * unmodified if the constructor throws an exception.
			 */
			ET *new_element_ptr = new (_alloc) ET(args...);

			/* grow array if index exceeds current capacity or if it's full */
			if (at.value >= _capacity || _upper_bound == _capacity) {

				size_t const new_capacity =
					2 * max(_capacity, max(8U, at.value));

				Element *new_array = nullptr;
				try {
					(void)_alloc.alloc(sizeof(Element)*new_capacity, &new_array);

					for (unsigned i = 0; i < new_capacity; i++)
						new_array[i] = Element { nullptr };
				}
				catch (... /* Out_of_ram, Out_of_caps */ ) {
					destroy(_alloc, new_element_ptr);
					throw;
				}

				if (new_array == nullptr) {
					destroy(_alloc, new_element_ptr);
					throw Out_of_ram();
				}

				if (_array) {
					for (unsigned i = 0; i < _upper_bound; i++)
						new_array[i] = _array[i];

					_alloc.free(_array, sizeof(Element)*_capacity);
				}

				_array    = new_array;
				_capacity = new_capacity;
			}

			/* make room for new element */
			if (_upper_bound > 0)
				for (unsigned i = _upper_bound; i > at.value; i--)
					_array[i] = _array[i - 1];

			_array[at.value] = Element { new_element_ptr };

			_upper_bound = max(at.value + 1, _upper_bound + 1);
		}

		template <typename... ARGS>
		void append(ARGS &&... args) { insert(Index{_upper_bound}, args...); }

		bool exists(Index at) const { return _index_valid(at); }

		Index upper_bound() const { return Index { _upper_bound }; }

		void destruct(Index at)
		{
			if (!_index_valid(at))
				return;

			destroy(_alloc, _array[at.value].ptr);
			_array[at.value] = Element { nullptr };

			for (unsigned i = at.value; i < _upper_bound; i++)
				_array[i] = _array[i + 1];

			_upper_bound--;
			_array[_upper_bound] = Element { nullptr };
		}

		template <typename FN>
		void apply(Index at, FN const &fn)
		{
			if (_index_valid(at))
				fn(*_array[at.value].ptr);
		}

		template <typename FN>
		void apply(Index at, FN const &fn) const
		{
			if (_index_valid(at))
				fn(*_array[at.value].ptr);
		}

		struct Range { Index at; unsigned length; };

		template <typename FN>
		void for_each(Range range, FN const &fn) const
		{
			unsigned const first = range.at.value;
			unsigned const limit = min(_upper_bound, first + range.length);

			for (unsigned i = first; i < limit; i++)
				if (_array[i].ptr)
					fn(Index{i}, *_array[i].ptr);
		}

		template <typename FN>
		void for_each(FN const &fn) const
		{
			for_each(Range { .at = { 0U }, .length = ~0U }, fn);
		}

		void print(Output &out) const
		{
			for (unsigned i = 0; i < _upper_bound; i++)
				if (_array[i].ptr)
					Genode::print(out, *_array[i].ptr);
		}
};

#endif /* _DYNAMIC_ARRAY_H_ */
