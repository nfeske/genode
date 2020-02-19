/*
 * \brief  Native context for base-linux with select/poll
 * \author Stefan Thoeni
 * \date   2019-12-13
 */

/*
 * Copyright (C) 2006-2020 Genode Labs GmbH
 * Copyright (C) 2019 gapfruit AG
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <base/internal/native_context.h>
#include <linux_syscalls.h>

Genode::Rpc_entrypoint::Native_context::Native_context()
{
}


void Genode::Rpc_entrypoint::Native_context::clear()
{
	_epoll_fd = lx_epoll_create();
	if (_epoll_fd < 0) {
		raw(lx_getpid(), ":", lx_gettid(), " lx_epoll_create failed with ", _epoll_fd);
		throw Ipc_error();
	}

	enum { LOCAL_SOCKET = 0, REMOTE_SOCKET = 1 };
	int sd[2];
	sd[LOCAL_SOCKET] = -1; sd[REMOTE_SOCKET] = -1;

	int ret = lx_socketpair(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0, sd);
	if (ret < 0) {
		raw(lx_getpid(), ":", lx_gettid(), " lx_socketpair failed with ", ret);
		throw Ipc_error();
	}

	_cancel_client_sd = sd[REMOTE_SOCKET];
	_cancel_server_sd = sd[LOCAL_SOCKET];
	add(_cancel_server_sd);
}


void Genode::Rpc_entrypoint::Native_context::add(int fd)
{
	epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = fd;
	int ret = lx_epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event);
	if (ret < 0) {
		raw(lx_getpid(), ":", lx_gettid(), " lx_epoll_ctl add failed with ", ret);
		throw Ipc_error();
	}
}


void Genode::Rpc_entrypoint::Native_context::remove(int fd)
{
	epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = fd;
	int ret = lx_epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, &event);
	if (ret == -2) {
		/* ignore file already closed */
	} else if (ret == -9) {
		/* ignore file already closed */
	} else if (ret < 0) {
		raw(lx_getpid(), ":", lx_gettid(), " lx_epoll_ctl remove failed with ", ret);
		throw Ipc_error();
	}
}


int Genode::Rpc_entrypoint::Native_context::poll()
{
	epoll_event events[1];
	int event_count = lx_epoll_wait(_epoll_fd, events, 1, -1);
	if (event_count < 0) {
		raw(lx_getpid(), ":", lx_gettid(), " lx_epoll_ctl failed with ", event_count);
		throw Ipc_error();
	}
	else if (event_count == 0)
	{
		return -1;
	}
	else if (event_count == 1)
	{
		auto e = events[0].events;
		switch (e) {
			case 0:
				return -1;
			case POLLIN:
				return events[0].data.fd;
			default:
				raw(lx_getpid(), ":", lx_gettid(), " unknown revent ", e, " from epoll_wait");
				return -1;
		}
	}
	else
	{
		raw(lx_getpid(), ":", lx_gettid(), " to many event on epoll_wait");
		throw Ipc_error();
	}
}


void Genode::Rpc_entrypoint::Native_context::write_cancel()
{
	msghdr msg;
	Genode::memset(&msg, 0, sizeof(msghdr));
	int const ret = lx_sendmsg(_cancel_client_sd, &msg, 0);
	if (ret < 0) {
		raw(lx_getpid(), ":", lx_gettid(), " write_cancel ep_nd=", this, " cc_sd(", &_cancel_client_sd, ")=", _cancel_client_sd, " lx_sendmsg failed ", ret);
		throw Ipc_error();
	}
}


bool Genode::Rpc_entrypoint::Native_context::clear_cancel(int fd)
{
	if (fd == _cancel_server_sd) {
		msghdr msg;
		Genode::memset(&msg, 0, sizeof(msghdr));
		int const ret = lx_recvmsg(_cancel_server_sd, &msg, 0);
		if (ret < 0) {
			raw(lx_getpid(), ":", lx_gettid(), " read_cancel lx_recvmsg failed ", ret);
			throw Ipc_error();
		}
		return false;
	} else {
		return true;
	}
}

