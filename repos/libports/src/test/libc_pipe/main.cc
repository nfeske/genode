/*
 * \brief  libc_pipe test
 * \author Christian Prochaska
 * \date   2016-04-24
 */

/*
 * Copyright (C) 2016-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */


/* libc includes */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <base/log.h>
using namespace Genode;

enum { BUF_SIZE = 16*1024 };
static char buf[BUF_SIZE];

static int pipefd[2];

static volatile bool reader_finished = false;

void *read_pipe(void *arg)
{
	static char read_buf[BUF_SIZE];

	ssize_t num_bytes_read = 0;

	while (num_bytes_read < BUF_SIZE) {

	log("pipe read");
		ssize_t res = read(pipefd[0],
		                   &read_buf[num_bytes_read],
		                   BUF_SIZE - num_bytes_read);

	log("pipe read returned");
		if (res < 0) {
			fprintf(stderr, "Error reading from pipe\n");
			exit(1);
		}

		num_bytes_read += res;
	}

	if ((read_buf[0] != buf[0]) ||
	    (read_buf[BUF_SIZE - 1] != buf[BUF_SIZE - 1])) {
		fprintf(stderr, "Error: data mismatch\n");
		exit(1);
	}

	reader_finished = true;


	log("pipe read thread returns");
	return 0;
}


int main(int argc, char *argv[])
{
	/* test values */
	buf[0] = 1;
	buf[BUF_SIZE - 1] = 255;

	log("pipe");
	int res = pipe(pipefd);
	log("pipe2");
	if (res != 0) {
		fprintf(stderr, "Error creating pipe\n");
		exit(1);
	}
	log("pipe3");

	pthread_t tid;
	pthread_create(&tid, 0, read_pipe, 0);
	log("pipe4");

	ssize_t bytes_written = write(pipefd[1], buf, BUF_SIZE);
	log("pipe5");

	if (bytes_written != BUF_SIZE) {
		fprintf(stderr, "Error writing to pipe (bytes_written=%zd, BUF_SIZE=%zd)\n",
		        bytes_written, (size_t)BUF_SIZE);
		exit(1);
	}
	log("pipe6 wait for pthread_join");

	pthread_join(tid, NULL);
	log("pipe7");

	printf("--- test finished ---\n");

	return 0;
}
