static char *rcsId = "$Header: /misc/hadesprojects/daq/cvsroot/hadaq/psxshm.c,v 6.6 2004-08-10 11:10:43 hadaq Exp $";

#define _POSIX_C_SOURCE 199309L
#include <unistd.h>
#ifndef _POSIX_SHARED_MEMORY_OBJECTS
#error POSIX_SHARED_MEMORY_OBJECTS not available on this OS
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "psxshm.h"

PsxShm *PsxShm_open(const char *name, int oflag, mode_t mode, off_t size)
{
	PsxShm *my;
	int prot;

	if (NULL == (my = malloc(sizeof(PsxShm)))) {
		goto cleanup0;
	}
	if (0 > (my->fd = shm_open(name, oflag, mode))) {
		goto cleanup1;
	}
	my->size = size;
	if ((oflag & O_CREAT) != 0) {
		if (0 > ftruncate(my->fd, my->size)) {
			goto cleanup2;
		}
	}

	prot = 0;
	if (oflag & O_RDONLY) {
		prot |= PROT_READ;
	}
	if (oflag & O_WRONLY) {
		prot |= PROT_WRITE;
	}
	if (oflag & O_RDWR) {
		prot |= PROT_READ | PROT_WRITE;
	}
	if ((void *) MAP_FAILED == (my->addr = mmap(0, my->size, prot, MAP_SHARED, my->fd, 0L))) {
		goto cleanup2;
	}
	return my;

/* cleanup allocated resources and report error */
  cleanup2:
	{
		int e = errno;

		close(my->fd);
		if ((oflag & O_CREAT) != 0) {	/* let's assume we have created it */
			shm_unlink(name);
		}
		errno = e;
	}
  cleanup1:
	free(my);
  cleanup0:
	return NULL;
}

int PsxShm_close(PsxShm *my)
{
	int ret;

	if (0 > munmap(my->addr, my->size)
		|| 0 > close(my->fd)
		) {
		ret = -1;
	} else {
		ret = 0;
	}
	free(my);
	return ret;
}

int PsxShm_unlink(const char *name)
{
	return shm_unlink(name);
}
