/*
 * \brief  Default version of platform-specific part of RPC framework
 * \author Norman Feske
 * \author Stefan Thoeni
 * \date   2006-05-12
 */

/*
 * Copyright (C) 2006-2017 Genode Labs GmbH
 * Copyright (C) 2019 gapfruit AG
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <util/retry.h>
#include <base/rpc_server.h>

/* base-internal includes */
#include <base/internal/ipc_server.h>
#include <base/internal/native_context.h>

using namespace Genode;


/***********************
 ** Server entrypoint **
 ***********************/

Untyped_capability Rpc_entrypoint::_manage(Rpc_object_base *obj)
{
	/* don't manage RPC object twice */
	if (obj->cap().valid()) {
		warning("attempt to manage RPC object twice");
		return obj->cap();
	}

	Untyped_capability new_obj_cap = _alloc_rpc_cap(_pd_session, _cap);

	/* add server object to object pool */
	obj->cap(new_obj_cap);
	insert(obj);

	_native_context->write_cancel();

	/* return capability that uses the object id as badge */
	return new_obj_cap;
}

