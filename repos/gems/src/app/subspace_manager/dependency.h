/*
 * \brief  Child dependency
 * \author Norman Feske
 * \date   2017-08-23
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _DEPENDENCY_H_
#define _DEPENDENCY_H_

/* local includes */
#include <runtime.h>

namespace Subspace_manager { struct Dependency; }


struct Subspace_manager::Dependency
{
	enum Primary { PRIMARY, SECONDARY } const _primary;

	Runtime::Arg const _arg;

	Weak_ptr<Resource> _resource;

	Dependency(Primary primary, Runtime::Arg const &arg, Weak_ptr<Resource> resource)
	:
		_primary(primary), _arg(arg), _resource(resource)
	{ }

	bool matches(Runtime::Arg const &arg) const { return arg == _arg; }
};

#endif /* _DEPENDENCY_H_ */
