#ifndef DABC_threads
#define DABC_threads

#include <pthread.h>

namespace dabc {

   class Mutex {
     friend class LockGuard;
     friend class Condition;
     protected:
        pthread_mutex_t  fMutex;
     public:
        Mutex(bool recursive = false);
        virtual ~Mutex() { pthread_mutex_destroy(&fMutex); }
        inline void Lock() { pthread_mutex_lock(&fMutex); }
        inline void Unlock() { pthread_mutex_unlock(&fMutex); }
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

      protected:

         Mutex           fInternCondMutex;
         Mutex*          fCondMutex;
         pthread_cond_t  fCond;
         long int        fFiredCounter;
         bool            fWaiting;
      public:
         long            fWaitCalled;
         long            fWaitDone;
   };

   // ___________________________________________________________

   class Runnable {
      friend class Thread;

      public:
         virtual void* MainLoop() = 0;

         virtual ~Runnable();
   };

   typedef void* (ThreadStartRoutine)(void*);

   // ___________________________________________________________

   typedef pthread_t Thread_t;

   class Thread {
      protected:
         pthread_t    fThrd;
         void         UseCurrentAsSelf() { fThrd = pthread_self(); }
      public:
         Thread();
         ~Thread();
         void Start(Runnable* run);
         void Start(ThreadStartRoutine* func, void* args);
         void Join();
         void Kill(int sig = 9);

         void SetPriority(int prio);
         inline bool IsItself() const { return pthread_equal(pthread_self(), fThrd) != 0; }

         inline Thread_t Id() const { return fThrd; }

         static Thread_t Self() { return pthread_self(); }
   };

}

#endif
