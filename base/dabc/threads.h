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

namespace dabc {

   class Mutex {
     friend class LockGuard;
     friend class UnlockGuard;
     friend class Condition;
     protected:
        pthread_mutex_t  fMutex;
     public:
        Mutex(bool recursive = false);
        virtual ~Mutex() { pthread_mutex_destroy(&fMutex); }
        inline void Lock() { pthread_mutex_lock(&fMutex); }
        inline void Unlock() { pthread_mutex_unlock(&fMutex); }
        bool TryLock();
        bool IsLocked();
   };

   // ___________________________________________________________

   class LockGuard {
     protected:
        pthread_mutex_t* fMutex;
     public:
        inline LockGuard(pthread_mutex_t& mutex) : fMutex(&mutex)
        {
           pthread_mutex_lock(fMutex);
        }
        inline LockGuard(pthread_mutex_t* mutex) : fMutex(mutex)
        {
           pthread_mutex_lock(fMutex);
        }
        inline LockGuard(const Mutex& mutex) : fMutex((pthread_mutex_t*)&(mutex.fMutex))
        {
           pthread_mutex_lock(fMutex);
        }
        inline LockGuard(const Mutex* mutex) : fMutex(mutex ? (pthread_mutex_t*) &(mutex->fMutex) : 0)
        {
           if (fMutex) pthread_mutex_lock(fMutex);
        }
        inline ~LockGuard()
        {
           if (fMutex) pthread_mutex_unlock(fMutex);
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

         inline void Reset()
         {
            LockGuard lock(fCondMutex);
            if (!fWaiting) fFiredCounter = 0;
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

      protected:

         Mutex           fInternCondMutex;
         Mutex*          fCondMutex;
         pthread_cond_t  fCond;
         long int        fFiredCounter;
         bool            fWaiting;
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
         void         UseCurrentAsSelf() { fThrd = pthread_self(); }
      public:

         typedef void* (StartRoutine)(void*);

         PosixThread();
         virtual ~PosixThread();
         void Start(Runnable* run);
         void Start(StartRoutine* func, void* args);
         void Join();
         void Kill(int sig = 9);
         void Cancel();

         void SetPriority(int prio);
//         inline bool IsItself() const { return pthread_equal(pthread_self(), fThrd) != 0; }
         inline bool IsItself() const { return fThrd == pthread_self(); }

         inline Thread_t Id() const { return fThrd; }

         static Thread_t Self() { return pthread_self(); }
   };

}

#endif
