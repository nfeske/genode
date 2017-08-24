/*
 * \brief  Representation of a resource (a session provided by a service)
 * \author Norman Feske
 * \date   2017-08-23
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _RESOURCE_H_
#define _RESOURCE_H_

/* Genode includes */
#include <util/xml_node.h>
#include <base/weak_ptr.h>

/* local includes */
#include <types.h>

namespace Subspace_manager { struct Resource; }


struct Subspace_manager::Resource : Weak_object<Resource>
{
	struct Type
	{
		enum _Type { NITPICKER, ROM, PD, CPU, TIMER, LOG, BLOCK, FS, UNKNOWN };

		_Type value;

		bool operator == (Type const &other) const { return other.value == value; }

		bool valid() const { return value != UNKNOWN; }

		static Type from_xml(Xml_node node)
		{
			return node.has_type("nitpicker") ? Type { NITPICKER } :
			       node.has_type("rom")       ? Type { ROM       } :
			       node.has_type("pd")        ? Type { PD        } :
			       node.has_type("cpu")       ? Type { CPU       } :
			       node.has_type("timer")     ? Type { TIMER     } :
			       node.has_type("log")       ? Type { LOG       } :
			       node.has_type("block")     ? Type { BLOCK     } :
			       node.has_type("fs")        ? Type { FS        } :
			                                    Type { UNKNOWN   };
		}

		void print(Output &out) const
		{
			using Genode::print;
			switch (value) {
			case NITPICKER: print(out, "Nitpicker");   return;
			case ROM:       print(out, "ROM");         return;
			case PD:        print(out, "PD");          return;
			case CPU:       print(out, "CPU");         return;
			case TIMER:     print(out, "Timer");       return;
			case LOG:       print(out, "LOG");         return;
			case BLOCK:     print(out, "Block");       return;
			case FS:        print(out, "File_system"); return;
			case UNKNOWN:   break;
			}
			print(out, "<unknown>");
		}
	};

	enum Multi { SINGLE, MULTI };

	typedef String<32> Label;

	Type  const _type;
	Multi const _multi;
	Label const _label;

	Resource(Type type, Multi multi, Label const &label)
	: _type(type), _multi(multi), _label(label) { }

	~Resource() { Weak_object_base::lock_for_destruction(); }
};

#endif /* _RESOURCE_H_ */
