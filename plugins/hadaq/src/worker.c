// $Id$

//static char *rcsId = "$Header: /misc/hadesprojects/daq/cvsroot/hadaq/worker.c,v 6.31 2010-07-06 15:35:08 hadaq Exp $";

#define WORKER_NEW_PROTOCOL

#define _POSIX_C_SOURCE 199309L
#include <unistd.h>
#ifndef _POSIX_PRIORITY_SCHEDULING
#error POSIX_PRIORITY_SCHEDULING not available on this OS
#endif

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <sched.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>

#include "worker.h"

static int sigReceived = 0;

static void noopHandler(int sig)
{
	sigReceived = sig;
}

static int changePriority(int p)
{
	struct sched_param spS, *sp = &spS;;

	if (sched_getscheduler(0) != SCHED_FIFO) {
#if defined(__osf__)
		/* FIFO priorities are not all real time under DEC-UNIX */
		sp->sched_priority = SCHED_PRIO_RT_MIN + 16;
#else
		sp->sched_priority = sched_get_priority_min(SCHED_FIFO) + 16;
#endif
	} else {
		sched_getparam(0, sp);
	}
	sp->sched_priority += p;

	return sched_setscheduler(0, SCHED_FIFO, sp);
}

static int createStatShm(Worker *my)
{
	int retVal;
	char ipcNameS[_POSIX_PATH_MAX];
	char ipcFullNameS[_POSIX_PATH_MAX] = "/dev/shm/";
	char *ipcName;

	strcpy(ipcNameS, my->name);
	ipcName = basename(ipcNameS);
	strcat(ipcName, ".shm");
	strcat(ipcFullNameS, ipcName);

	if (-1 == PsxShm_unlink(ipcName) && errno != ENOENT) {
		syslog(LOG_DEBUG, "%s:%d: %s", __FILE__, __LINE__, strerror(errno));
		retVal = -1;
	} else {
		my->shm = PsxShm_open(ipcName, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO, WORKER_MAX_NUM_STATS * sizeof(Statistic));
		if (NULL == my->shm) {
			syslog(LOG_DEBUG, "%s:%d: %s", __FILE__, __LINE__, strerror(errno));
			retVal = -1;
		} else {
			my->statistics = my->shm->addr;
			strcpy(my->statistics[0].name, "");

			if (-1 == chmod(ipcFullNameS, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
				syslog(LOG_ERR, "%s:%d: %s", __FILE__, __LINE__, strerror(errno));
			}

			retVal = 0;
		}
	}

	return retVal;
}

static int removeStatShm(Worker *my)
{
	char ipcNameS[_POSIX_PATH_MAX];
	char *ipcName;

	PsxShm_close(my->shm);

	strcpy(ipcNameS, my->name);
	ipcName = basename(ipcNameS);
	strcat(ipcName, ".shm");

	PsxShm_unlink(ipcName);

	return 0;
}

static int openStatShm(Worker *my)
{
	int retVal;
	char ipcNameS[_POSIX_PATH_MAX];
	char *ipcName;

	strcpy(ipcNameS, my->name);
	ipcName = basename(ipcNameS);
	strcat(ipcName, ".shm");

	my->shm = PsxShm_open(ipcName, O_RDWR, 0, WORKER_MAX_NUM_STATS * sizeof(Statistic));
	if (NULL == my->shm) {
		retVal = -1;
	} else {
		my->statistics = my->shm->addr;
		retVal = 0;
	}

	return retVal;
}

static int closeStatShm(Worker *my)
{
	return PsxShm_close(my->shm);
}

static int removeSigHandlers(Worker *my)
{
  return 0; // test
  // jam2017 we disable signal handles of shared memory workers and just use dabc native ctrl-c handler

	return sigaction(my->signal0, my->oldSigAction0, NULL)
		| sigaction(my->signal1, my->oldSigAction1, NULL)
		| sigaction(my->signal2, my->oldSigAction2, NULL);
}

static int installSigHandlers(Worker *my, int s0, int s1, int s2, void (*sigHandler) (int))
{
  return 0; // test
  // jam2017: ? may we disable signal handles of shared memory workers and just use dabc native ctrl-c handler
	int retVal;
	struct sigaction actS, *act = &actS;

	act->sa_handler = sigHandler;
	sigfillset(&act->sa_mask);
	act->sa_flags = 0;

	my->oldSigAction0 = &my->oldSigAction0S;
	my->signal0 = s0;
	if (0 > sigaction(my->signal0, act, my->oldSigAction0)) {
		syslog(LOG_DEBUG, "%s:%d: %s", __FILE__, __LINE__, strerror(errno));
		my->oldSigAction0 = NULL;
	}
	my->signal1 = s1;
	my->oldSigAction1 = &my->oldSigAction1S;
	if (0 > sigaction(my->signal1, act, my->oldSigAction1)) {
		syslog(LOG_DEBUG, "%s:%d: %s", __FILE__, __LINE__, strerror(errno));
		my->oldSigAction1 = NULL;
	}

	my->signal2 = s2;
	my->oldSigAction2 = &my->oldSigAction2S;
	if (0 > sigaction(my->signal2, act, my->oldSigAction2)) {
		syslog(LOG_DEBUG, "%s:%d: %s", __FILE__, __LINE__, strerror(errno));
		my->oldSigAction2 = NULL;
	}

	if (my->oldSigAction0 == NULL || my->oldSigAction1 == NULL || my->oldSigAction2 == NULL) {
		removeSigHandlers(my);
		retVal = -1;
	} else {
		retVal = 0;
	}

	return retVal;
}

Worker *Worker_initBegin(const char *name, void (*sigHandler) (int), int priority, int isStandalone)
{
	Worker *retVal;
	Worker *my;

	if (NULL == (my = malloc(sizeof(Worker)))) {
		syslog(LOG_ERR, "%s:%d: %s", __FILE__, __LINE__, strerror(errno));
		return NULL;
	}
	strcpy(my->name, name);
	my->pid = getpid();
	my->isStandalone = isStandalone;

	if (-1 == installSigHandlers(my, SIGINT, SIGTERM, SIGHUP, sigHandler)) {
		syslog(LOG_DEBUG, "%s:%d: %s", __FILE__, __LINE__, strerror(errno));
		Worker_fini(my);
		retVal = NULL;
	} else if (-1 == createStatShm(my)) {
		syslog(LOG_DEBUG, "%s:%d: %s", __FILE__, __LINE__, strerror(errno));
		Worker_fini(my);
		retVal = NULL;
	} else {
#ifndef WORKER_NEW_PROTOCOL
		unsigned long *pidP;

		pidP = Worker_addStatistic(my, "pid");
		*pidP = my->pid;
#else
		/* pid must always be first statistic */
		Worker_addStatistic(my, "pid");
		my->statistics[0].value = 0;
#endif

#if 0
		if (0 > changePriority(priority)) {
			syslog(LOG_WARNING, "changeing priority: %s", strerror(errno));
		}
#endif
		retVal = my;
	}

	return retVal;
}

void Worker_initEnd(Worker *my)
{
#ifndef WORKER_NEW_PROTOCOL
	if (!my->isStandalone) {
		if (0 > kill(getppid(), SIGUSR1)) {
			syslog(LOG_DEBUG, "%s:%d: %s", __FILE__, __LINE__, strerror(errno));
		}
	}
#else
	my->statistics[0].value = my->pid;
#endif
}

void Worker_fini(Worker *my)
{
	removeStatShm(my);
	removeSigHandlers(my);

	free(my);
}

int Worker_start(const char *path, char *const argv[])
{
	int retVal;
	Worker myS, *my = &myS;
	struct timespec t = { 1, 0 };

	strcpy(my->name, argv[0]);
#ifndef WORKER_NEW_PROTOCOL

	installSigHandlers(my, SIGCHLD, SIGUSR1, SIGHUP, noopHandler);

#endif
	my->pid = fork();
	if (0 > my->pid) {
		syslog(LOG_DEBUG, "%s:%d: %s", __FILE__, __LINE__, strerror(errno));
		retVal = -1;
	} else {
		if (my->pid == 0) {		/* This is the child, we can not get out of */
			/* this block */
#if 0
			/* BUGBUG this syslog causes multithreaded agent (IOC) to stop */
			syslog(LOG_INFO, "Starting worker %d (%s)", getpid(), path);
#endif
			if (0 > execvp(path, argv)) {
				syslog(LOG_DEBUG, "%s:%d: %s", __FILE__, __LINE__, strerror(errno));
				syslog(LOG_EMERG, "Starting %s: %s", path, strerror(errno));
				abort();
			}
		} else {
#ifndef WORKER_NEW_PROTOCOL
			sigset_t sigMaskS, *sigMask = &sigMaskS;

			/* BUGBUG there should be a timeout here */
			sigemptyset(sigMask);	/* This is the parent, so wait for the */
			sigsuspend(sigMask);	/* child to initialize */

			if (sigReceived == SIGCHLD) {
				nanosleep(&t, NULL);
				wait(NULL);
				syslog(LOG_DEBUG, "%s:%d: %s", __FILE__, __LINE__, strerror(errno));
				retVal = -1;
			} else {
				retVal = 0;
			}
#else
			/* wait until worker opens statShm and reports pid */
			const int timeout = 60;
			int workerIsStarted = 0;
			int i;

			for (i = 0; !workerIsStarted && i < timeout; i++) {
				if (0 == openStatShm(my)) {
					if (my->pid == my->statistics[0].value) {
						workerIsStarted = 1;
					}
					closeStatShm(my);
				}
				nanosleep(&t, NULL);
			}
			if (i == timeout) {
				retVal = -1;
			} else {
				retVal = 0;
			}
#endif
		}
	}
#ifndef WORKER_NEW_PROTOCOL

	removeSigHandlers(my);

#endif
	return retVal;
}

char *Worker_status(const char *name)
{
	char *retVal;
	Worker myS, *my = &myS;
	static char buf[WORKER_MAX_NUM_STATS * 80];
	char *p;
	int i;

	strcpy(my->name, name);

	if (-1 == openStatShm(my)) {
		syslog(LOG_DEBUG, "%s:%d: %s", __FILE__, __LINE__, strerror(errno));
		retVal = NULL;
	} else {
		strcpy(buf, "{}");
		for (i = 0, p = buf; i < WORKER_MAX_NUM_STATS && strcmp(my->statistics[i].name, "") != 0; i++) {
			p += sprintf(p, "{ %s %lu } ", my->statistics[i].name, my->statistics[i].value);
		}
		closeStatShm(my);
		retVal = buf;
	}

	return retVal;
}

void Worker_stop(const char *name, int timeout)
{
	Worker myS, *my = &myS;

	strcpy(my->name, name);

	if (0 == openStatShm(my)) {
		my->pid = my->statistics[0].value;
		if (my->pid > 0) {
			syslog(LOG_INFO, "Stopping worker %d", my->pid);
			if (0 == kill(my->pid, SIGTERM)) {
				while (0 == kill(my->pid, 0)) {
					struct timespec t = { 1, 0 };

					waitpid(my->pid, NULL, WNOHANG);
					nanosleep(&t, NULL);
				}
			}
		}
		closeStatShm(my);
	}
}

unsigned long *Worker_addStatistic(Worker *my, const char *name)
{
	unsigned long *retVal;
	int i;

	for (i = 0; i < WORKER_MAX_NUM_STATS && strcmp(my->statistics[i].name, "") != 0; i++) {
	}
	if (i == WORKER_MAX_NUM_STATS) {
		syslog(LOG_ERR, "%s:%d: %s", __FILE__, __LINE__, "Too many statistic counters");
		retVal = NULL;
	} else {
		strcpy(my->statistics[i].name, name);
		my->statistics[i].value = 0;
		strcpy(my->statistics[i + 1].name, "");

		retVal = &(my->statistics[i].value);
	}
	return retVal;
}

void Worker_dump(Worker *my, time_t interval)
{
	if (my->isStandalone) {
		static time_t lastTime = 0;
		time_t curTime;

		curTime = time(NULL);
		if (curTime >= lastTime + interval) {
			fprintf(stderr, "%s\n", Worker_status(my->name));
			lastTime = curTime;
		}
	}
}

int Worker_getStatistic(const char *name, const char *stat, unsigned long int *value)
{
	int retVal = -1;
	int i;
	Worker myS, *my = &myS;
	strcpy(my->name, name);

	if (-1 == openStatShm(my)) {
		*value = 0;
	} else {
		for (i = 0; i < WORKER_MAX_NUM_STATS && strcmp(my->statistics[i].name, "") != 0; i++) {
			if (strcasecmp(stat, my->statistics[i].name) == 0) {
				*value = my->statistics[i].value;
				retVal = 0;
				i = WORKER_MAX_NUM_STATS;
			}
		}
		closeStatShm(my);
	}
	return retVal;
}

int Worker_setStatistic(const char *name, const char *stat, unsigned long int *value)
{
	int retVal = -1;
	int i;
	Worker myS, *my = &myS;
	strcpy(my->name, name);

	if (-1 == openStatShm(my)) {
		*value = 0;
	} else {
		for (i = 0; i < WORKER_MAX_NUM_STATS && strcmp(my->statistics[i].name, "") != 0; i++) {
			if (strcasecmp(stat, my->statistics[i].name) == 0) {
				my->statistics[i].value = *value;
				retVal = 0;
				i = WORKER_MAX_NUM_STATS;
			}
		}
		closeStatShm(my);
	}
	return retVal;
}
