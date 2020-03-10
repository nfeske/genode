/*
 * \brief  File/socket descriptor set for select/poll
 * \author Stefan Thoeni
 * \date   2019-12-13
 */

/*
 * Copyright (C) 2019-2020 Genode Labs GmbH
 * Copyright (C) 2019-2020 gapfruit AG
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__BASE__INTERNAL__NATIVE_CONTEXT_H_
#define _INCLUDE__BASE__INTERNAL__NATIVE_CONTEXT_H_

#include <base/rpc_server.h>


class Genode::Rpc_entrypoint::Native_context
{
	private:

		int _cancel_client_sd = 0;
		int _cancel_server_sd = 0;
		int _epoll_fd = 0;

	public:
		Native_context();

		void clear();

		void add(int fd);

		void remove(int fd);

		int poll();

		void write_cancel();

		bool clear_cancel(int fd);
};

#endif /* _INCLUDE__BASE__INTERNAL__NATIVE_CONTEXT_H_ */
