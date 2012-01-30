/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "dabc/threads.h"

#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>

#include "dabc/logging.h"

dabc::Mutex::Mutex(bool recursive)
{
   if (recursive) {
      pthread_mutexattr_t attr;
      pthread_mutexattr_init(&attr);
      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
      pthread_mutex_init(&fMutex, &attr);
      pthread_mutexattr_destroy(&attr);
   } else
      pthread_mutex_init(&fMutex, 0);
}

bool dabc::Mutex::TryLock()
{
   return pthread_mutex_trylock(&fMutex) != EBUSY;
}


bool dabc::Mutex::IsLocked()
{
   int res = pthread_mutex_trylock(&fMutex);
   if (res==EBUSY) return true;
   pthread_mutex_unlock(&fMutex);
   return false;
}

//_____________________________________________________________

dabc::Condition::Condition(Mutex* ext_mtx) :
   fInternCondMutex(),
   fCondMutex(ext_mtx ? ext_mtx : &fInternCondMutex),
   fFiredCounter(0),
   fWaiting(false)
{
   pthread_cond_init(&fCond, 0);
}

dabc::Condition::~Condition()
{
   pthread_cond_destroy(&fCond);
}

bool dabc::Condition::_DoWait(double wait_seconds)
{
   // mutex must be already locked at this point

   // meaning of argument
   // wait_seconds > 0 - time to wait
   // wait_seconds = 0 - do not wait, just check if condition was fired
   // wait_seconds < 0 - wait forever until condition is fired


   if (fFiredCounter==0) {
      if (wait_seconds < 0.) {
         fWaiting = true;
         pthread_cond_wait(&fCond, &fCondMutex->fMutex);
         fWaiting = false;
      } else
      if (wait_seconds > 0.) {
         struct timeval tp;
         gettimeofday(&tp, 0);

         long wait_microsec = long(wait_seconds*1e6);

         tp.tv_sec += (wait_microsec + tp.tv_usec) / 1000000;
         tp.tv_usec = (wait_microsec + tp.tv_usec) % 1000000;

         struct timespec tsp  = { tp.tv_sec, tp.tv_usec*1000 };

         fWaiting = true;
         pthread_cond_timedwait(&fCond, &fCondMutex->fMutex, &tsp);
         fWaiting = false;
      }
   }

   if (fFiredCounter > 0) {
      fFiredCounter--;
      return true;
   }

   return false;
}

//_____________________________________________________________

dabc::Runnable::~Runnable()
{
   DOUT5(("dabc::Runnable::~Runnable destructor %p", this));
}

extern "C" void CleanupRunnable(void* abc)
{
   dabc::Runnable *run = (dabc::Runnable*) abc;

   if (run!=0) run->RunnableCancelled();
}

extern "C" void* StartTRunnable(void* abc)
{
   dabc::Runnable *run = (dabc::Runnable*) abc;

   void* res = 0;

   if (run!=0) {

      pthread_cleanup_push(CleanupRunnable, abc);

      res = run->MainLoop();

      pthread_cleanup_pop(1);
   }

   pthread_exit(res);
}

dabc::PosixThread::PosixThread() :
   fThrd()
{
}

dabc::PosixThread::~PosixThread()
{
}

void dabc::PosixThread::Start(Runnable* run)
{
   if (run==0) return;

   pthread_create(&fThrd, NULL, StartTRunnable, run);
}

void dabc::PosixThread::Start(StartRoutine* func, void* args)
{
   if (func==0) return;

   pthread_create(&fThrd, NULL, func, args);
}

void dabc::PosixThread::Join()
{
   void* res = 0;
   pthread_join(fThrd, &res);
}

void dabc::PosixThread::SetPriority(int prio)
{
   struct sched_param thread_param;
   int ret = 0;
   memset(&thread_param, 0, sizeof(thread_param));
   thread_param.sched_priority = prio;
   ret = pthread_setschedparam(fThrd, (prio>0) ? SCHED_FIFO : SCHED_OTHER,
                                 &thread_param);
   if (ret!=0)
      EOUT(("pthread_setschedparam ret = %d %d %d %d %d\n", ret, (ret==EPERM), (ret==ESRCH), (ret==EINVAL), (ret==EFAULT)));
}

void dabc::PosixThread::Kill(int sig)
{
   pthread_kill(fThrd, sig);
}

void dabc::PosixThread::Cancel()
{
   pthread_cancel(fThrd);
}
