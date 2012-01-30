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

#ifndef DABC_collections
#include "dabc/collections.h"
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
   class ThreadRef;

   class Thread : public Object,
                   protected PosixThread,
                   protected Runnable {
      protected:

      friend class Worker;
      friend class ThreadRef;

      class RecursionGuard {
         private:
            Thread*  thrd;      //!< we can use direct pointer, reference will be preserved by other means
            unsigned workerid;  //!< worker id which recursion is guarded
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

      struct WorkerRec {
         Worker*        work;         //!< pointer on the worker, should we use reference?
         unsigned       doinghalt;    //!< indicates that events will not be longer accepted by the worker, all submitted commands still should be executed
         int            recursion;    //!< recursion calls of the worker
         unsigned       processed;    //!< current number of processed events, when balance between processed and fired is 0, worker can be halted
         CommandsQueue  cmds;         //!< postponed commands, which are waiting until object is destroyed or halted

         TimeStamp      tmout_mark;   //!< time mark when timeout should happen
         double         tmout_interv; //!< time interval was specified by timeout active
         bool           tmout_active; //!< true when timeout active

         TimeStamp      prev_fire;    //!< when previous timeout event was called
         TimeStamp      next_fire;    //!< when next timeout event will be called

         WorkerRec(Worker* w) :
            work(w),
            doinghalt(0),
            recursion(0),
            processed(0),
            cmds(CommandsQueue::kindSubmit),
            tmout_mark(),
            tmout_interv(0.),
            tmout_active(false),
            prev_fire(),
            next_fire() {}
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

         int CheckWorkerCanBeHalted(unsigned id, unsigned request = 0, Command cmd = 0);

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

         ExecWorker*          fExec; // processor to execute commands in the thread
         bool                 fDidDecRefCnt; // indicates if object cleanup was called - need in destructor
         bool                 fCheckThrdCleanup; // !< indicates if thread should be checked for clean up

         static unsigned      fThreadInstances;

      public:

         enum EEvents { evntCheckTmout = 1,  //!< event used to process timeout for specific processor, used by ActivateTimeout
                         evntCleanupThrd,     //!< event will be generated when thread can be destroyed
                         evntDoNothing,       //!< event fired to wake-up thread and let thread or processor to perform regular checks
                         evntStopThrd,        //!< event should stop thread
                         evntLastThrd,
                         evntUser = 10000 };

         enum EPriority { priorityHighest = 0,
                           priorityNormal = 1,
                           priorityLowest = -1 };

         static unsigned NumThreadInstances() { return fThreadInstances; }

         Thread(Reference parent, const char* name, unsigned numqueus = 3);

         virtual ~Thread();

         virtual void* MainLoop();

         inline bool IsItself() const { return PosixThread::IsItself(); }
         void SetPriority(int prio = 0) { PosixThread::SetPriority(prio); }

         // thread manipulation, thread safe routines
         bool Start(double timeout_sec = -1., bool real_thread = true);
         bool Stop(double timeout_sec = 10);
         bool Sync(double timeout_sec = -1);

         virtual const char* ClassName() const { return typeThread; }

         virtual bool CompatibleClass(const char* clname) const;

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
               EOUT(("False arguments fNumQueues:%d nq:%d", fNumQueues, nq));
               return;
            }
            #endif

            fQueues[nq<0 ? fNumQueues - 1 : nq].Push(arg);

            #ifdef DABC_EXTRA_CHECKS
            if (nq<0) nq = fNumQueues - 1;
            if (fQueues[nq].Size()>maxlimit) {
               EOUT(("Thrd:%s Queue %d Event code:%u item:%u arg:%u exceed limit: %u", GetName(), nq,
                     arg.GetCode(), arg.GetItem(), arg.GetArg(), maxlimit));
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

         bool SetExplicitLoop(Worker* proc);

         void RunExplicitLoop();

         /** \brief Virtual method, called from thread context to inform that number of
          * workers are changed. Can be used by derived class to reorganize its structure */
         virtual void WorkersNumberChanged() {}

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
   };


   class ThreadRef : public Reference {

      friend class Worker;

      DABC_REFERENCE(ThreadRef, Reference, Thread)

      protected:
         bool _ActivateWorkerTimeout(unsigned workerid, int priority, double tmout);

      public:
         bool IsItself() const { return GetObject() ? GetObject()->IsItself() : false; }

         bool Execute(Command cmd, double tmout = -1.);

         /** \brief Runs thread event loop for specified time.
          * If time less than 0, event function called at least once.
          * Should be used at the places where user want to sleep inside processor */
         void RunEventLoop(double tmout = 1.);

         inline bool IsRealThrd() const { return GetObject() ? GetObject()->fRealThrd : false; }
   };

}

#endif
