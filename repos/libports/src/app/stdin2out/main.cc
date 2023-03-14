/*
 * \brief  Utility for forwarding data from stdin to stdout
 * \author Norman Feske
 * \date   2020-03-25
 *
 * This program fulfils the purpose of 'tail -f' for the log view of Sculpt OS.
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>

extern "C" int main(int, char **)
{
	while (true) {

		/* wait until stdin becomes ready to read */
		fd_set fds { };
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);
		select(1, &fds, NULL, NULL, NULL);

		char buffer[4096] { };
		ssize_t const bytes = read(STDIN_FILENO, buffer, sizeof(buffer));

		if (bytes == -1)
			fprintf(stderr, "read failed, errno=%d\n", errno);

		if (bytes > 0)
			write(STDOUT_FILENO, buffer, bytes);
	}
}
