/*
 * \brief  Allocate an RPC capability via socketpair on Linux
 * \author Norman Feske
 * \author Stefan Thoeni
 * \date   2016-01-19
 */

/*
 * Copyright (C) 2019 gapfruit AG
 * Copyright (C) 2019 Genode Labs GmbH
 */

/* Genode includes */
#include <base/env.h>
#include <util/retry.h>
#include <base/rpc_server.h>
#include <pd_session/client.h>
#include <deprecated/env.h>

/* base-internal includes */
#include <base/internal/globals.h>
#include <linux_syscalls.h>
#include <base/internal/rpc_destination.h>
#include <base/internal/rpc_obj_key.h>
#include <base/internal/capability_space_tpl.h>
#include <base/internal/native_context.h>
#include <base/internal/rpc_cap_alloc_socketpair.h>

using namespace Genode;


Native_capability Genode::_alloc_rpc_cap_socketpair(Rpc_entrypoint::Native_context& native_context)
{
	enum { LOCAL_SOCKET = 0, REMOTE_SOCKET = 1 };
	int sd[2];
	sd[LOCAL_SOCKET] = -1; sd[REMOTE_SOCKET] = -1;

	int ret = lx_socketpair(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0, sd);
	if (ret < 0) {
		raw(lx_getpid(), ":", lx_gettid(), " lx_socketpair failed with ", ret);
		throw Ipc_error();
	}

	Rpc_destination dst(sd[REMOTE_SOCKET]);
	Rpc_obj_key key(sd[LOCAL_SOCKET]);
	auto cap = Capability_space::import(dst, key);

	native_context.add(sd[LOCAL_SOCKET]);

	return cap;
}


void Genode::_free_rpc_cap_socketpair(Rpc_entrypoint::Native_context& native_context, Native_capability cap)
{
	Capability_space::Ipc_cap_data cap_data =
		Capability_space::ipc_cap_data(cap);

	int ret0 = lx_close(cap_data.dst.socket);
	if (ret0 < 0) {
		raw(lx_getpid(), ":", lx_gettid(), " lx_close client socket failed with ", ret0);
	}

	int ret1 = lx_close(cap_data.rpc_obj_key.value());
	if (ret1 < 0) {
		raw("[", lx_gettid(), "] lx_close server socket failed with ", ret1);
	}

	native_context.remove(cap_data.rpc_obj_key.value());
}
