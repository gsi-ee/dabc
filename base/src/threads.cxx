// $Id$

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
#include <cerrno>
#include <cstring>


#if defined(__MACH__) /* Apple OSX section */

// try to provide dummy wrapper for all using functions around affinity

void CPU_ZERO(cpu_set_t *arg) { arg->flag = 0; }

void CPU_SET(int cpu, cpu_set_t *arg) { arg->flag |= (1<<cpu); }

bool CPU_ISSET(int cpu, cpu_set_t *arg) { return arg->flag & (1<<cpu); }

void CPU_CLR(int cpu, cpu_set_t *arg) { arg->flag = arg->flag & ~(1<<cpu); }

int sched_getaffinity(int, int, cpu_set_t* set) { set->flag = 0xFFFF; return 0; }

int sched_setaffinity(int, int, cpu_set_t*) { return 0; }

#endif



dabc::Mutex::Mutex(bool recursive)
{
   if (recursive) {
      pthread_mutexattr_t attr;
      pthread_mutexattr_init(&attr);
      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
      pthread_mutex_init(&fMutex, &attr);
      pthread_mutexattr_destroy(&attr);
   } else {
      pthread_mutex_init(&fMutex, 0);
   }
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

bool dabc::MutexPtr::TryLock()
{
   return fMutex ? pthread_mutex_trylock(fMutex) != EBUSY : false;
}


bool dabc::MutexPtr::IsLocked()
{
   if (fMutex==0) return false;
   int res = pthread_mutex_trylock(fMutex);
   if (res==EBUSY) return true;
   pthread_mutex_unlock(fMutex);
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
   DOUT5("dabc::Runnable::~Runnable destructor %p", this);
}

extern "C" void CleanupRunnable(void* abc)
{
   dabc::Runnable *run = (dabc::Runnable*) abc;

   if (run!=0) run->RunnableCancelled();
}

extern "C" void* StartTRunnable(void* abc)
{
   dabc::Runnable *run = (dabc::Runnable*) abc;

   void* res = nullptr;

   if (run!=0) {

      pthread_cleanup_push(CleanupRunnable, abc);

      res = run->MainLoop();

      pthread_cleanup_pop(1);
   }

   pthread_exit(res);
}

// ====================================================================

cpu_set_t dabc::PosixThread::fSpecialSet;
cpu_set_t dabc::PosixThread::fDfltSet;

bool dabc::PosixThread::SetDfltAffinity(const char* aff)
{
   CPU_ZERO(&fSpecialSet);
   CPU_ZERO(&fDfltSet);

   if (!aff || (*aff==0)) return true;

   if ((*aff=='-') && (strlen(aff)>1)) {
      unsigned numspecial = 0;
      if (!str_to_uint(aff+1, &numspecial) || (numspecial==0)) {
         EOUT("Wrong  default affinity format %s", aff);
         return false;
      }

      int res = sched_getaffinity(0, sizeof(fDfltSet), &fDfltSet);

      if (res!=0) {
         EOUT("sched_getaffinity res = %d", res);
         return false;
      }

      unsigned numset(0);
      for (int cpu=0;cpu<CPU_SETSIZE;cpu++)
         if (CPU_ISSET(cpu, &fDfltSet)) numset++;

      if (numset<=numspecial) {
         EOUT("Cannot reduce affinity on %u processors - only %u assigned for process", numspecial, numset);
         return false;
      }

      unsigned cnt = 0;
      for (int cpu=0;cpu<CPU_SETSIZE;cpu++)
         if (CPU_ISSET(cpu, &fDfltSet)) {
            if (++cnt>numset-numspecial) {
               CPU_CLR(cpu, &fDfltSet);
               CPU_SET(cpu, &fSpecialSet);
            }
         }

      res = sched_setaffinity(0, sizeof(fDfltSet), &fDfltSet);
      if (res!=0) { EOUT("sched_setaffinity failed res = %d", res); return false; }
      return true;
   }


   if ((*aff=='o') || (*aff=='x') || (*aff=='s')) {
      unsigned cpu = 0;
      const char* curr = aff;
      bool isany(false);

      while ((*curr!=0) && (cpu<CPU_SETSIZE)) {
         switch (*curr) {
            case 'x': CPU_SET(cpu, &fDfltSet); isany = true; break;
            case 'o': CPU_CLR(cpu, &fDfltSet); break;
            case 's': CPU_SET(cpu, &fSpecialSet); break;
            default: EOUT("Wrong  default affinity format %s", aff); return false;
         }
         curr++; cpu++;
      }

      if (isany) {
         int res = sched_setaffinity(0, sizeof(fDfltSet), &fDfltSet);
         if (res!=0) { EOUT("sched_setaffinity failed res = %d", res); return false; }
      }

      return true;

   }

   unsigned mask(0);

   if (!str_to_uint(aff, &mask)) {
      EOUT("Wrong  default affinity format %s", aff);
      return false;
   }

   if (mask==0) return true;

   for (unsigned cpu = 0; (cpu < sizeof(mask)*8) && (cpu<CPU_SETSIZE); cpu++)
      if ((mask & (1 << cpu)) != 0)
         CPU_SET(cpu, &fDfltSet);

   int res = sched_setaffinity(0, sizeof(fDfltSet), &fDfltSet);
   if (res!=0) { EOUT("sched_setaffinity failed res = %d", res); return false; }

   return true;
}


dabc::PosixThread::PosixThread() :
   fThrd(),
   fCpuSet()
{
   CPU_ZERO(&fCpuSet);
   for (unsigned cpu=0;cpu<CPU_SETSIZE;cpu++)
      if (CPU_ISSET(cpu, &fDfltSet))
         CPU_SET(cpu, &fCpuSet);
}

dabc::PosixThread::~PosixThread()
{
}

bool dabc::PosixThread::SetAffinity(const char* aff)
{
   CPU_ZERO(&fCpuSet);

   if (!aff || (*aff==0)) {
      for (unsigned cpu=0;cpu<CPU_SETSIZE;cpu++)
         if (CPU_ISSET(cpu, &fDfltSet))
            CPU_SET(cpu, &fCpuSet);
      return true;
   }

   if ((*aff=='+') && (strlen(aff)>1)) {

      unsigned specialid = 0, numspecial = 0;
      if (!str_to_uint(aff+1, &specialid)) {
         EOUT("Wrong affinity format %s", aff);
         return false;
      }

      for (unsigned cpu=0;cpu<CPU_SETSIZE;cpu++)
         if (CPU_ISSET(cpu, &fSpecialSet)) {
            if (specialid == numspecial++)
               CPU_SET(cpu, &fCpuSet);
         }

      if (specialid >= numspecial) {
         EOUT("Where are only %u special processors, cannot assigned id %u", numspecial, specialid);
         return false;
      }

      return true;
   }

   if ((*aff=='o') || (*aff=='x')) {
      unsigned cpu = 0;
      const char* curr = aff;
      bool isany(false);

      while ((*curr!=0) && (cpu<CPU_SETSIZE)) {
         switch (*curr) {
            case 'x': CPU_SET(cpu, &fCpuSet); isany = true; break;
            case 'o': CPU_CLR(cpu, &fCpuSet); break;
            default: EOUT("Wrong  affinity format %s", aff); return false;
         }
         curr++; cpu++;
      }

      if (!isany) { EOUT("Wrong affinity format %s", aff); return false; }

      return true;
   }

   unsigned mask(0);

   if (!str_to_uint(aff, &mask)) {
      EOUT("Wrong  affinity format %s", aff);
      return false;
   }

   if (mask==0) {
      EOUT("Zero affinity mask specified %s", aff);
      return false;
   }

   for (unsigned cpu = 0; (cpu < sizeof(mask)*8) && (cpu<CPU_SETSIZE); cpu++)
      if ((mask & (1 << cpu)) != 0)
         CPU_SET(cpu, &fCpuSet);

   return true;
}


void dabc::PosixThread::Start(Runnable* run)
{
   if (run==0) return;

   bool isany(false);
   for (unsigned cpu=0;cpu<CPU_SETSIZE;cpu++)
      if (CPU_ISSET(cpu, &fCpuSet)) isany = true;

   if (!isany) {
      pthread_create(&fThrd, nullptr, StartTRunnable, run);
   } else {
      pthread_attr_t attr;
      pthread_attr_init(&attr);

#if !defined(__MACH__) && _POSIX_C_SOURCE >= 200112L
      pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &fCpuSet);
#endif

      pthread_create(&fThrd, &attr, StartTRunnable, run);

      pthread_attr_destroy(&attr);
   }
}

void dabc::PosixThread::Start(StartRoutine* func, void* args)
{
   if (!func) return;

   pthread_create(&fThrd, nullptr, func, args);
}

void dabc::PosixThread::Join()
{
   void* res = nullptr;
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
      EOUT("pthread_setschedparam ret = %d %d %d %d %d\n", ret, (ret==EPERM), (ret==ESRCH), (ret==EINVAL), (ret==EFAULT));
}

void dabc::PosixThread::Kill(int sig)
{
   pthread_kill(fThrd, sig);
}

void dabc::PosixThread::Cancel()
{
   pthread_cancel(fThrd);
}

void dabc::PosixThread::SetThreadName(const char *thrdname)
{
#if !defined(__MACH__)
   pthread_setname_np(fThrd, thrdname);
#endif
}

bool dabc::PosixThread::GetDfltAffinity(char* buf, unsigned maxbuf)
{
   unsigned last(0);

   if (maxbuf==0) return false;

   for (unsigned cpu=0;cpu<CPU_SETSIZE;cpu++) {
      char symb = 'o';
      if (CPU_ISSET(cpu, &fDfltSet)) symb = 'x'; else
      if (CPU_ISSET(cpu, &fSpecialSet)) symb = 's';

      if (symb!='o') last = cpu;
      if (cpu<maxbuf) buf[cpu] = symb;
   }

   if (last+1 < maxbuf) {
      unsigned wrap = (last / 8 + 1) * 8;
      if (wrap < maxbuf) buf[wrap] = 0;
                    else buf[last+1] = 0;
      return true;
   }

   // it was not enough place to keep all active-special threads
   buf[maxbuf-1] = 0;
   return false;
}

bool dabc::PosixThread::GetAffinity(bool actual, char* buf, unsigned maxbuf)
{

   if (maxbuf==0) return false;

   cpu_set_t mask;
   cpu_set_t *arg = nullptr;
   CPU_ZERO(&mask);

   if (!actual) {
      arg = &fCpuSet;
   } else {

#if !defined(__MACH__) && _POSIX_C_SOURCE >= 200112L
      int s;
      pthread_attr_t attr;
      s = pthread_getattr_np(pthread_self(), &attr);
      if (s != 0) { EOUT("pthread_getattr_np failed"); return false; }

      s = pthread_attr_getaffinity_np(&attr, sizeof(cpu_set_t), &mask);
      if (s != 0) EOUT("pthread_attr_getaffinity_np failed");

      s = pthread_attr_destroy(&attr);
      if (s != 0) EOUT("pthread_attr_destroy failed");

      arg = &mask;
#else
      buf[0] = 0;
      return false;
#endif
   }

   unsigned last(0);

   for (unsigned cpu=0;cpu<CPU_SETSIZE;cpu++) {
      char symb = 'o';
      if (CPU_ISSET(cpu, arg)) symb = 'x';
      if (symb!='o') last = cpu;
      if (cpu<maxbuf) buf[cpu] = symb;
   }

   if (last+1 < maxbuf) {
      unsigned wrap = (last / 8 + 1) * 8;
      if (wrap < maxbuf) buf[wrap] = 0;
                    else buf[last+1] = 0;
      return true;
   }

   // it was not enough place to keep all active-special threads
   buf[maxbuf-1] = 0;
   return false;
}

