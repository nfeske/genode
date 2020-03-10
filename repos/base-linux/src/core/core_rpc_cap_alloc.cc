/*
 * \brief  Core-specific back end of the RPC entrypoint
 * \author Norman Feske
 * \author Stefan Thoeni
 * \date   2016-01-19
 */

/*
 * Copyright (C) 2016-2017 Genode Labs GmbH
 * Copyright (C) 2019 gapfruit AG
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/env.h>
#include <util/retry.h>
#include <base/rpc_server.h>
#include <deprecated/env.h>

/* base-internal includes */
#include <base/internal/rpc_cap_alloc_socketpair.h>

using namespace Genode;


Native_capability Rpc_entrypoint::_alloc_rpc_cap(Pd_session&, Native_capability, addr_t)
{
	return alloc_rpc_cap_socketpair(*_native_context);
}


void Rpc_entrypoint::_free_rpc_cap(Pd_session&, Native_capability cap)
{
	free_rpc_cap_socketpair(*_native_context, cap);
}
