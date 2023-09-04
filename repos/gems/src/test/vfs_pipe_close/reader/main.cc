/*
 * \brief  Test for abrupt closing a named pipe
 * \author Norman Feske
 * \date   2023-09-04
 */

/*
 * Copyright (C) 2023 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <libc/component.h>

/* libc includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

namespace Reader {

	using namespace Genode;

	struct Main;
}


struct Reader::Main
{
	Libc::Env &_env;

	Main(Libc::Env &env) : _env(env)
	{
		int const fd = open("/named", O_RDONLY);
		log("open returned fd ", fd);

		fd_set read_fds { };

		FD_SET(fd, &read_fds);

		while (1) {
			log("calling select");

			int const select_res = select(fd + 1, &read_fds, NULL, NULL, NULL);

			log("select returned ", select_res);

			char read_buf[1024] { };

			log("read from fd ", fd);
			ssize_t const read_res = read(fd, read_buf, sizeof(read_buf));
			log("received ", read_res, " bytes");
		}
	}
};


void Libc::Component::construct(Libc::Env &env)
{
	with_libc([&] {
		static Reader::Main main(env);
	});
}

