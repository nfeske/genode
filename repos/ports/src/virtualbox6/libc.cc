/*
 * \brief  VirtualBox runtime (RT)
 * \author Norman Feske
 * \date   2013-08-20
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

/* libc includes */
#include <signal.h>
#include <sys/times.h>
#include <unistd.h>
#include <aio.h>

/* local includes */
#include <stub_macros.h>

static bool const debug = true;

extern "C" {

int futimes(int fd, const struct timeval tv[2]) TRACE(0)
int lutimes(const char *filename, const struct timeval tv[2]) TRACE(0)
int lchown(const char *pathname, uid_t owner, gid_t group) TRACE(0)
int mlock(const void *addr, size_t len) TRACE(0)
int aio_fsync(int op, struct aiocb *aiocbp) STOP
ssize_t aio_return(struct aiocb *aiocbp) STOP
int aio_error(const struct aiocb *aiocbp) STOP
int aio_cancel(int fd, struct aiocb *aiocbp) STOP
int aio_suspend(const struct aiocb * const aiocb_list[],
                       int nitems, const struct timespec *timeout) STOP
int lio_listio(int mode, struct aiocb *const aiocb_list[],
               int nitems, struct sigevent *sevp) STOP

} /* extern "C" */
