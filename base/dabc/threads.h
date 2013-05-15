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

#ifndef DABC_threads
#define DABC_threads

#include <pthread.h>

#include <stdio.h>

#ifndef DABC_defines
#include "dabc/defines.h"
#endif

#ifndef DABC_logging
#include "dabc/logging.h"
#endif

namespace dabc {

   class Mutex {
     friend class LockGuard;
     friend class UnlockGuard;
     friend class Condition;
     friend class MutexPtr;
     protected:
        pthread_mutex_t  fMutex;
     public:
        Mutex(bool recursive = false);
        inline ~Mutex() { pthread_mutex_destroy(&fMutex); }
        inline void Lock() { pthread_mutex_lock(&fMutex); }
        inline void Unlock() { pthread_mutex_unlock(&fMutex); }
        bool TryLock();
        bool IsLocked();
   };

   class MutexPtr {
      protected:
         pthread_mutex_t*  fMutex;
      public:
         inline MutexPtr(pthread_mutex_t& mutex) : fMutex(&mutex) {}
         inline MutexPtr(pthread_mutex_t* mutex) : fMutex(mutex) {}
         inline MutexPtr(const Mutex& mutex) : fMutex((pthread_mutex_t*)&(mutex.fMutex)) {}
         inline MutexPtr(const Mutex* mutex) : fMutex(mutex ? (pthread_mutex_t*) &(mutex->fMutex) : 0) {}
         inline MutexPtr(const MutexPtr& src) : fMutex(src.fMutex) {}

         inline ~MutexPtr() {}

         bool null() const { return fMutex == 0; }
         void clear() { fMutex = 0; }

         inline void Lock() { if (fMutex) pthread_mutex_lock(fMutex); }
         inline void Unlock() { if (fMutex) pthread_mutex_unlock(fMutex); }
         bool TryLock();
         bool IsLocked();
   };

   // ___________________________________________________________

#ifdef DABC_EXTRA_CHECKS

//#define DABC_LOCKGUARD(mutex,info)  dabc::LockGuard dabc_guard(mutex)

#define DABC_LOCKGUARD(mutex,info) \
   dabc::LockGuard dabc_guard(mutex, false); \
   while (!dabc_guard.TryingLock()) EOUT(info);

#else

#define DABC_LOCKGUARD(mutex,info)  dabc::LockGuard dabc_guard(mutex)

#endif


   class LockGuard {
     protected:
         MutexPtr fMutex;
     public:
        inline LockGuard(pthread_mutex_t& mutex) :
           fMutex(mutex)
        {
           fMutex.Lock();
        }
        inline LockGuard(pthread_mutex_t* mutex) :
           fMutex(mutex)
        {
           fMutex.Lock();
        }
        inline LockGuard(const Mutex& mutex) :
           fMutex(mutex)
        {
           fMutex.Lock();
        }
        inline LockGuard(const Mutex* mutex) :
           fMutex(mutex)
        {
           fMutex.Lock();
        }

        inline LockGuard(pthread_mutex_t& mutex, bool) :
           fMutex(mutex)
        {
        }
        inline LockGuard(pthread_mutex_t* mutex, bool) :
           fMutex(mutex)
        {
        }
        inline LockGuard(const Mutex& mutex, bool) :
           fMutex(mutex)
        {
        }
        inline LockGuard(const Mutex* mutex, bool) :
           fMutex(mutex)
        {
        }

        inline ~LockGuard()
        {
           fMutex.Unlock();
        }

        bool TryingLock()
        {
           if (fMutex.null()) return true;

           int cnt=1000000;
           while (cnt-->0)
              if (fMutex.TryLock()) return true;

           // return false if many attempts to lock mutex fails
           return false;
        }


   };

   class UnlockGuard {
     protected:
        pthread_mutex_t* fMutex;
     public:
        inline UnlockGuard(const Mutex* mutex) : fMutex(mutex ? (pthread_mutex_t*) &(mutex->fMutex) : 0)
        {
           if (fMutex) pthread_mutex_unlock(fMutex);
        }
        inline ~UnlockGuard()
        {
           if (fMutex) pthread_mutex_lock(fMutex);
        }
   };


   class IntGuard {
      private:
         int*  fInt;

      public:
         inline IntGuard(const int* value) { fInt = (int*) value; if (fInt) (*fInt)++; }
         inline IntGuard(const int& value) { fInt = (int*) &value; (*fInt)++; }
         inline ~IntGuard() { if (fInt) (*fInt)--; }

         inline int* ptr() { return fInt; }
   };



   // ___________________________________________________________

   class Condition {
      protected:
         Mutex           fInternCondMutex;
         Mutex*          fCondMutex;
         pthread_cond_t  fCond;
         long int        fFiredCounter;
         bool            fWaiting;
      public:
         Condition(Mutex* ext_mtx = 0);
         virtual ~Condition();

         inline void DoFire()
         {
            LockGuard lock(fCondMutex);
            _DoFire();
         }

         inline bool DoWait(double wait_seconds = -1.)
         {
            LockGuard lock(fCondMutex);
            return _DoWait(wait_seconds);
         }

         inline void _DoReset()
         {
            if (!fWaiting) fFiredCounter = 0;
         }


         inline void Reset()
         {
            LockGuard lock(fCondMutex);
            _DoReset();
         }

         inline void _DoFire()
         {
           // mutex must be already locked at this point
           fFiredCounter++;
           if (fWaiting)
              pthread_cond_signal(&fCond);
         }

         bool _DoWait(double wait_seconds);

         Mutex* CondMutex() const { return fCondMutex; }

         bool _Waiting() const { return fWaiting; }

         long int _FiredCounter() const { return fFiredCounter; }
   };

   // ___________________________________________________________

   class Runnable {
      friend class Thread;

      public:
         virtual void* MainLoop() = 0;

         virtual void RunnableCancelled() {}

         virtual ~Runnable();
   };

   // ___________________________________________________________

   typedef pthread_t Thread_t;

   class PosixThread {
      protected:
         pthread_t    fThrd;

         cpu_set_t    fCpuSet;   //

         static cpu_set_t fDfltSet;
         static cpu_set_t fSpecialSet; // set of processors, which can be used for special threads

         void         UseCurrentAsSelf() { fThrd = pthread_self(); }

      public:

         typedef void* (StartRoutine)(void*);

         PosixThread();
         virtual ~PosixThread();

         /** Sets affinity mask for the thread
          * Should be called before thread is started
          * Can be:
          *   - unsigned value with processors mask
          *   - string like "xxxoooxxx" were x and o identified enabled and disabled processors,
          *           first element in string corresponds to first processor
          *   -string like "+M" where M is processor number in special processors set, before SetDfltAffinity("-N") should be called  (M<N)  */
         bool SetAffinity(const char* aff);

         /** Returns thread affinity in form of "xxxooooo"
          * If actual==true, request will be done to the thread,
          * otherwise configured value will be provided
          * See SetAffinity method for more information */
         bool GetAffinity(bool actual, char* buf, unsigned maxbuf);

         /** Start thread with provided runnable */
         void Start(Runnable* run);

         /** Start thread with provided routine and call arguments */
         void Start(StartRoutine* func, void* args);

         /** Join thread - method waits until thread execution is completed */
         void Join();

         /** Kill thread with specified signal */
         void Kill(int sig = 9);

         /** Try to cancel thread execution */
         void Cancel();

         /** Change thread priority */
         void SetPriority(int prio);

         inline Thread_t Id() const { return fThrd; }

         static Thread_t Self() { return pthread_self(); }

         //         inline bool IsItself() const { return pthread_equal(pthread_self(), fThrd) != 0; }
         inline bool IsItself() const { return fThrd == pthread_self(); }


         /** Sets default affinity for next threads to be created and for main process.
          * Can be:
          *    - unsigned value with processors mask
          *    - string like "xxxoooxxxss" with allowed symbols 'x', 'o' and 's'.
          *            'x' - enabled, 'o' - disabled, 's' - special purpose
          *             first element in string corresponds to first processor
          *    - string like "-N" where N is processors number which should be
          *             reserved for special purposes, these processors could be
          *             later assigned with SetAffinity("+M") call (M<N) */
         static bool SetDfltAffinity(const char* aff = 0);

         /** Returns default affinity mask in form "xxxooosss".
          * See SetDfltAffinity for more details */
         static bool GetDfltAffinity(char* buf, unsigned maxbuf);
   };

}

#endif
