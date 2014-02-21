// $Id$

#ifndef HADAQ_WORKER_H
#define HADAQ_WORKER_H

#include <limits.h>
#include <signal.h>

#include "psxshm.h"

#define WORKER_MAX_NUM_STATS 850
#define WORKER_MAX_NAME_LEN 32

typedef struct StatisticS {
	char name[WORKER_MAX_NAME_LEN];
	unsigned long value;
}

Statistic;

typedef struct WorkerS {
	char name[_POSIX_PATH_MAX];
	int signal0;
	int signal1;
	int signal2;
	struct sigaction oldSigAction0S;
	struct sigaction *oldSigAction0;
	struct sigaction oldSigAction1S;
	struct sigaction *oldSigAction1;
	struct sigaction oldSigAction2S;
	struct sigaction *oldSigAction2;
	PsxShm *shm;
	Statistic *statistics;
	pid_t pid;
	int isStandalone;
} Worker;

int Worker_start(const char *path, char *const argv[]);
char *Worker_status(const char *name);
void Worker_stop(const char *name, int timeout);
Worker *Worker_initBegin(const char *name, void (*sigHandler) (int), int priority, int isStandalone);
void Worker_initEnd(Worker *my);
void Worker_fini(Worker *my);
unsigned long *Worker_addStatistic(Worker *my, const char *name);
void Worker_dump(Worker *my, time_t interval);
int Worker_getStatistic(const char *name, const char *stat, unsigned long int *value);
int Worker_setStatistic(const char *name, const char *stat, unsigned long int *value);

#endif
