// $Id$

#ifndef PSXSHM_H
#define PSXSHM_H

#include <sys/types.h>

typedef struct PsxShmS {
	int fd;
	void *addr;
	off_t size;
}

PsxShm;

PsxShm *PsxShm_open(const char *name, int oflag, mode_t mode, off_t size);
int PsxShm_close(PsxShm *my);
int PsxShm_unlink(const char *name);

#endif
