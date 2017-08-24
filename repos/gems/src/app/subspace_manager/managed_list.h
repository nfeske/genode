/*
 * \brief  Safe wrapper around 'Genode::List'
 * \author Norman Feske
 * \date   2017-08-23
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _MANAGED_LIST_H_
#define _MANAGED_LIST_H_

/* Genode includes */
#include <util/list.h>
#include <util/noncopyable.h>
#include <base/allocator.h>

/* local includes */
#include <types.h>

namespace Subspace_manager { template <typename> struct Managed_list; }


template <typename ELEM>
class Subspace_manager::Managed_list : Noncopyable
{
	private:

		Allocator &_alloc;

		struct Element : List<Element>::Element
		{
			ELEM elem;

			template <typename... ARGS>
			Element(ARGS &&... args) : elem(args...) { }
		};

		List<Element> _list;

		Element *_last = nullptr;  /* used for appending elements */

	public:

		Managed_list(Allocator &alloc) : _alloc(alloc) { }

		~Managed_list()
		{
			while (Element *e = _list.first())
				destroy(_alloc, e);
		}

		template <typename... ARGS>
		void append(ARGS &&... args)
		{
			Element *e = new (_alloc) Element(args...);
			_list.insert(e, _last);
			_last = e;
		}

		template <typename FN>
		void for_each(FN const &fn) const
		{
			for (Element const *e = _list.first(); e; e = e->next())
				fn(e->elem);
		}

		template <typename FN>
		void for_each(FN const &fn)
		{
			for (Element *next = nullptr, *e = _list.first(); e; e = next) {
				next = e->next();
				fn(e->elem);
			}
		}
};

#endif /* _MANAGED_LIST_H_ */
