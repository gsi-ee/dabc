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

#ifndef DABC_Thread
#define DABC_Thread

#ifndef DABC_Object
#include "dabc/Object.h"
#endif

#ifndef DABC_defines
#include "dabc/defines.h"
#endif

#ifndef DABC_threads
#include "dabc/threads.h"
#endif

#ifndef DABC_EventId
#include "dabc/EventId.h"
#endif

#ifndef DABC_Queue
#include "dabc/Queue.h"
#endif

#ifndef DABC_Command
#include "dabc/Command.h"
#endif

#ifndef DABC_CommandsQueue
#include "dabc/CommandsQueue.h"
#endif

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#ifndef DABC_Exception
#include "dabc/Exception.h"
#endif

#ifndef DABC_logging
#include "dabc/logging.h"
#endif

#include <vector>

namespace dabc {

   class Worker;
   class WorkerAddon;
   class ThreadRef;

   /** \brief Represent thread functionality.
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    */

   class Thread : public Object,
                  protected PosixThread,
                  protected Runnable {
      protected:

         friend class Worker;
         friend class ThreadRef;

         class RecursionGuard {
            private:
               Thread*  thrd;      ///< we can use direct pointer, reference will be preserved by other means
               unsigned workerid;  ///< worker id which recursion is guarded
            public:
               RecursionGuard(Thread* t, unsigned id) :
                  thrd(t),
                  workerid(id)
               {
                  if (thrd) thrd->ChangeRecursion(workerid, true);
               }

               ~RecursionGuard()
               {
                  if (thrd) thrd->ChangeRecursion(workerid, false);
               }

         };

         struct TimeoutRec {
            TimeStamp      tmout_mark;   ///< time mark when timeout should happen
            double         tmout_interv; ///< time interval was specified by timeout active
            bool           tmout_active; ///< true when timeout active

            TimeStamp      prev_fire;    ///< when previous timeout event was called
            TimeStamp      next_fire;    ///< when next timeout event will be called

            TimeoutRec() : tmout_mark(), tmout_interv(0.), tmout_active(false), prev_fire(), next_fire() {}

            TimeoutRec(const TimeoutRec& src) :
               tmout_mark(src.tmout_mark),
               tmout_interv(src.tmout_interv),
               tmout_active(src.tmout_active),
               prev_fire(src.prev_fire),
               next_fire(src.next_fire) {}

            /** Activating timeout */
            bool Activate(double tmout)
            {
               bool dofire = !tmout_active;
               tmout_mark = dabc::Now();
               tmout_interv = tmout;
               tmout_active = true;
               return dofire;
            }

            /** Method called to check event, submitted when timeout was requested
             * Returns true when check should be done */
            bool CheckEvent(Mutex* thread_mutex)
            {
               TimeStamp mark;
               double interv(0.);

               {
                  LockGuard lock(thread_mutex);
                  if (!tmout_active) return false;
                  mark = tmout_mark;
                  interv = tmout_interv;
                  tmout_active = false;
               }

               if (interv<0) {
                  next_fire.Reset();
                  prev_fire.Reset();
               } else {
                  // if one activate timeout with positive interval, emulate
                  // that one already has previous call to ProcessTimeout
                  if (prev_fire.null() && (interv>0))
                     prev_fire = mark;

                  mark+=interv;

                  // set activation time only in the case if no other active timeout was used
                  // TODO: why such condition was here??
                  // every new activate call should set new marker for timeout processing

//                  if (next_fire.null() || (mark < next_fire)) {
                     next_fire = mark;
                     return true;
//                  }
               }

               return false;
            }

            bool CheckNextProcess(const TimeStamp& now, double& min_tmout, double& last_diff)
            {
               if (next_fire.null()) return false;

               double dist = next_fire - now;

               if (dist>=0.) {
                  if ((min_tmout<0.) || (dist<min_tmout))  min_tmout = dist;
                  return false;
               }

               last_diff = prev_fire.null() ? 0. : now - prev_fire;

               return true;
            }

            void SetNextFire(const TimeStamp& now, double dist, double& min_tmout)
            {
               if (dist>=0.) {
                  prev_fire = now;
                  next_fire = now + dist;
                  if ((min_tmout<0.) || (dist<min_tmout))  min_tmout = dist;
               } else {
                  prev_fire.Reset();
                  next_fire.Reset();
               }
            }
         };



         struct WorkerRec {
            Worker*        work;         ///< pointer on the worker, should we use reference?
            WorkerAddon*   addon;        ///< addon for the worker, maybe thread-specific
            unsigned       doinghalt;    ///< indicates that events will not be longer accepted by the worker, all submitted commands still should be executed
            int            recursion;    ///< recursion calls of the worker
            unsigned       processed;    ///< current number of processed events, when balance between processed and fired is 0, worker can be halted
            CommandsQueue  cmds;         ///< postponed commands, which are waiting until object is destroyed or halted

            TimeoutRec     tmout_worker; ///< timeout handling for worker
            TimeoutRec     tmout_addon;  ///< timeout handling for addon

            WorkerRec(Worker* w, WorkerAddon* a) :
               work(w),
               addon(a),
               doinghalt(0),
               recursion(0),
               processed(0),
               cmds(CommandsQueue::kindSubmit),
               tmout_worker(),
               tmout_addon() {}
         };

         typedef std::vector<WorkerRec*> WorkersVector;

         class EventsQueue : public Queue<EventId, true> {
            public:
               int scaler;

               EventsQueue() :
                  Queue<EventId, true>(),
                  scaler(8)
                  {
                  }
         };

         class ExecWorker;

         friend class ExecWorker;
         friend class Object;
         friend class RecursionGuard;

         /** \brief Internal DABC method, used to verify if worker can be halted now while recursion is over
          * Request indicates that halt action is requested : actDestroy = 1 or actHalt = 2.
          * Returns true when worker is really halted **/

         enum EHaltActions { actDestroy = 1, actHalt = 2 };

         enum EThreadState { stCreated, stRunning, stStopped, stError, stChanging };

         EThreadState         fState;

         bool                 fThrdWorking; // flag indicates if mainloop of the thread should continue to work
         bool                 fRealThrd;    // indicate if we create real thread and not running mainloop from top process

         Condition            fWorkCond; // condition, which is used in default MainLoop implementation

         EventsQueue         *fQueues;
         int                  fNumQueues;

         TimeStamp            fNextTimeout; // indicate when we expects next timeout
         int                  fProcessingTimeouts; // indicate recursion in timeouts processing

         WorkersVector        fWorkers;   // vector of all processors

         unsigned             fExplicitLoop; // id of the worker, selected to run own explicit loop

         ExecWorker*          fExec;             // processor to execute commands in the thread
         bool                 fDidDecRefCnt;     // indicates if object cleanup was called - need in destructor
         bool                 fCheckThrdCleanup; // !< indicates if thread should be checked for clean up

         bool                 fProfiling;        ///! if true, different statistic will be accumulated about thread
         TimeStamp            fLastWaitTime;     ///! when doing profiling, last time when wait was started is remembered
         double               fThreadRunTime;    ///! when doing profiling, total run time is accumulated
         double               fThreadWaitTime;   ///! when doing profiling, wait time is accumulated

         static unsigned      fThreadInstances;


         int CheckWorkerCanBeHalted(unsigned id, unsigned request = 0, Command cmd = 0);

         void IncWorkerFiredEvents(Worker* work);


      public:

         enum EEvents { evntCheckTmoutWorker = 1,  ///< event used to process timeout for specific worker, used by ActivateTimeout
                        evntCheckTmoutAddon, ///< event used to process timeout for addon, used by ActivateTimeout
                        evntCleanupThrd,     ///< event will be generated when thread can be destroyed
                        evntDoNothing,       ///< event fired to wake-up thread and let thread or processor to perform regular checks
                        evntStopThrd,        ///< event should stop thread
                        evntLastThrd,
                        evntUser = 10000 };

         enum EPriority { priorityHighest = 0,
                           priorityNormal = 1,
                           priorityLowest = -1 };

         static unsigned NumThreadInstances() { return fThreadInstances; }

         Thread(Reference parent, const std::string& name, Command cmd = 0);

         virtual ~Thread();

         virtual void* MainLoop();

         inline bool IsItself() const { return PosixThread::IsItself(); }
         void SetPriority(int prio = 0) { PosixThread::SetPriority(prio); }

         // thread manipulation, thread safe routines
         bool Start(double timeout_sec = -1., bool real_thread = true);
         bool Stop(double timeout_sec = 10);
         bool Sync(double timeout_sec = -1);

         virtual const char* ClassName() const { return typeThread; }

         virtual bool CompatibleClass(const std::string& clname) const;

         void FireDoNothingEvent();

         bool Execute(Command cmd, double tmout = -1);

         /** \brief Processes single event from the thread queue.
          * Workerid indicates context where execution is done, one can throw exception if explicit loop works around
          * Returns false when worker should be halted */
         bool SingleLoop(unsigned workerid = 0, double tmout = -1);

         /** \brief Runs thread event loop for specified time.
          * If time less than 0, event function called at least once.
          * Should be used at the places where user want to sleep inside processor */
         void RunEventLoop(double tmout = 1.);

         /** \brief Print thread content on debug output */
         virtual void Print(int lvl = 0);

         /** \brief Return total number of all events in the queues */
         unsigned TotalNumberOfEvents();

      protected:

         inline Mutex* ThreadMutex() const { return ObjectMutex(); }

         /** Returns true is this is temporary thread for command execution */
         bool IsTemporaryThread() const { return GetParent() == 0; }

         virtual int ExecuteThreadCommand(Command cmd);

         virtual bool WaitEvent(EventId&, double tmout);

         void ProcessEvent(const EventId&);

         /** Method to process events which are not processed by Thread class itself
          * Should be used in derived classes for their events processing. */
         virtual void ProcessExtraThreadEvent(const EventId&) {}

         void ProcessNoneEvent();

         bool _GetNextEvent(EventId&);

         virtual void RunnableCancelled();

        #ifdef DABC_EXTRA_CHECKS
          static unsigned maxlimit;
        #endif

         unsigned _TotalNumberOfEvents();

         inline void _PushEvent(const EventId& arg, int nq)
         {
            #ifdef DABC_EXTRA_CHECKS
            if ((fNumQueues==0) || (fQueues==0) || (nq>=fNumQueues)) {
               EOUT("False arguments fNumQueues:%d nq:%d", fNumQueues, nq);
               return;
            }
            #endif

            fQueues[nq<0 ? fNumQueues - 1 : nq].Push(arg);

            #ifdef DABC_EXTRA_CHECKS
            if (nq<0) nq = fNumQueues - 1;
            if (fQueues[nq].Size()>maxlimit) {
               EOUT("Thrd:%s Queue %d Event code:%u item:%u arg:%u exceed limit: %u", GetName(), nq,
                     arg.GetCode(), arg.GetItem(), arg.GetArg(), maxlimit);
               maxlimit *= 2;
            }
            #endif
         }

         virtual void _Fire(const EventId& arg, int nq);

         inline void Fire(const EventId& arg, int nq)
         {
            LockGuard lock(ThreadMutex());
            _Fire(arg, nq);
         }

         double CheckTimeouts(bool forcerecheck = false);

         /** \brief Internal DABC method, Add worker to thread; reference-safe
          * Reference safe means - it is safe to call it as long as reference on thread is exists
          * We use here reference on the worker to ensure that it does not disappear meanwhile*/
         bool AddWorker(Reference ref, bool sync = true);

         /** \brief Halt worker - stops any execution, break recursion */
         bool HaltWorker(Worker* proc);

         /** Called when worker addon changed on the fly */
         void WorkerAddonChanged(Worker* work, bool assign = true);

         bool SetExplicitLoop(Worker* work);

         void RunExplicitLoop();

         /** \brief Virtual method, called from thread context to inform that number of
          * workers are changed. Can be used by derived class to reorganize its structure */
         virtual void WorkersSetChanged() {}

         /** Cleanup object asynchronously.
          * This allows to call object cleanup from the thread where it processed.
          * It can be processor which will be removed
          */
         bool InvokeWorkerDestroy(Worker* work);

         /** \brief Method which allows to control recursion of each worker.
          * If worker must be destroyed or halted, this is allowed only with zero recursion
          */
         void ChangeRecursion(unsigned id, bool inc);

         /** \brief Cleanup thread that manager is allowed to delete it */
         virtual void ObjectCleanup();

         /** */
         virtual bool _DoDeleteItself();

         /** Returns actual number of workers */
         unsigned NumWorkers();

         /** Method used to profile thread load, should be called before waiting of next thread event */
         inline void ProfileBeforeWait()
         {
            fThreadRunTime += fLastWaitTime.SpentTillNow(true);
         }

         /** Method used to profile thread load, should be called after waiting of next thread event */
         inline void ProfileAfterWait()
         {
            fThreadWaitTime += fLastWaitTime.SpentTillNow();
         }

   };

   // __________________________________________________________________________

   /** \brief %Reference on the \ref dabc::Thread class
    *
    * \ingroup dabc_all_classes
    */

   class ThreadRef : public Reference {

      friend class Worker;
      friend class WorkerAddon;

      DABC_REFERENCE(ThreadRef, Reference, Thread)

      protected:
         bool _ActivateWorkerTimeout(unsigned workerid, int priority, double tmout);
         bool _ActivateAddonTimeout(unsigned workerid, int priority, double tmout);

         bool AcquireThreadRef(ThreadRef& ref) { return AcquireRefWithoutMutex(ref); }

      public:
         bool IsItself() const { return GetObject() ? GetObject()->IsItself() : false; }

         bool Execute(Command cmd, double tmout = -1.);

         /** \brief Runs thread event loop for specified time.
          * If time less than 0, event function called at least once.
          * Should be used at the places where user want to sleep inside processor */
         void RunEventLoop(double tmout = 1.);

         inline bool IsRealThrd() const { return GetObject() ? GetObject()->fRealThrd : false; }

         /** Make dummy worker to run addon inside the thread */
         bool MakeWorkerFor(WorkerAddon* addon, const std::string& name = "");
   };

}

#endif
