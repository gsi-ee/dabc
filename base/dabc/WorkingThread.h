/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#ifndef DABC_WorkingThread
#define DABC_WorkingThread

#ifndef DABC_Basic
#include "dabc/Basic.h"
#endif

#ifndef DABC_threads
#include "dabc/threads.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

#ifndef DABC_CommandClient
#include "dabc/CommandClient.h"
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

   class WorkingProcessor;

   class WorkingThread : public Basic,
                         protected Thread,
                         protected Runnable,
                         public CommandReceiver {

      friend class WorkingProcessor;

      public:

         enum EEvents { evntSysCmd = 1,
                        evntProcCmd,
                        evntCheckTmout,
                        evntRebuildProc,
                        evntDoNothing,
                        evntLastSysCmd,
                        evntUser = 10000 };

         enum EStatus { stCreated, stRunning, stStopped, stJoined, stError, stChanging };

      public:

         WorkingThread(Basic* parent, const char* name, unsigned numqueus = 3);

         virtual ~WorkingThread();

         virtual void* MainLoop();

         inline bool IsItself() const { return Thread::IsItself(); }
         void SetPriority(int prio = 0) { Thread::SetPriority(prio); }

         // thread manipulation, thread safe routines
         bool Start(double timeout_sec = -1., bool withoutthrd = false);
         bool Stop(bool waitjoin = true, double timeout_sec = -1);
         bool Sync(double timeout_sec = -1);

         // direct change of working flag
         void SetWorkingFlag(bool on);

         inline bool IsThrdWorking() const { return fThrdWorking; }
         inline bool NoLongerUsed() const { return fNoLongerUsed; }
         inline bool IsRealThrd() const { return fRealThrd; }

         inline TimeStamp_t ThrdTimeStamp() { return fTime.GetTimeStamp(); }

         inline TimeSource* ThrdTimeSource() { return &fTime; }

         virtual bool Submit(Command* cmd);

         virtual const char* ClassName() const { return typeWorkingThread; }

         virtual bool CompatibleClass(const char* clname) const;

         void FireDoNothingEvent();

      protected:

         typedef std::vector<WorkingProcessor*> ProcessorsVector;

         typedef Queue<EventId> EventsQueue;

         // these are method from comman receiver
         virtual int ExecuteCommand(Command* cmd);
         virtual bool IsExecutionThread() { return IsItself(); }

         virtual EventId WaitEvent(double tmout);

         void ProcessEvent(EventId evid);

         inline EventId _GetNextEvent()
         {
            for(int nq=0; nq<fNumQueues; nq++)
              if (fQueues[nq].Size()>0)
                 return fQueues[nq].Pop();
            return NullEventId;
         }

         inline void _PushEvent(EventId arg, int nq)
         {
            #ifdef DO_INDEX_CHECK
            if ((fNumQueues==0) || (fQueues==0) || (nq>=fNumQueues)) {
               EOUT(("False arguments fNumQueues:%d nq:%d", fNumQueues, nq));
               return;
            }
            #endif

            fQueues[nq<0 ? fNumQueues - 1 : nq].Push(arg);

            #ifdef DO_INDEX_CHECK
            if (nq<0) nq = fNumQueues - 1;
            if (fQueues[nq].Size()>maxlimit) {
               EOUT(("Thrd:%s Queue %d exeed limit: %u", GetName(), nq, maxlimit));
               maxlimit = maxlimit*2;
            }
            #endif
         }

         #ifdef DO_INDEX_CHECK
         static unsigned maxlimit;
         #endif

         virtual void _Fire(EventId arg, int nq);

         inline void Fire(EventId arg, int nq)
         {
            LockGuard lock(fWorkMutex);
            _Fire(arg, nq);
         }

         double CheckTimeouts(bool forcerecheck = false);

         bool AddProcessor(WorkingProcessor* proc, bool sync = true);
         void SysCommand(const char* cmdname, WorkingProcessor* proc);
         bool SubmitProcessorCmd(WorkingProcessor* proc, Command* cmd);
         void RemoveProcessor(WorkingProcessor* proc);
         void DestroyProcessor(WorkingProcessor* proc);

         bool SetExplicitLoop(WorkingProcessor* proc);
         void SingleLoop(WorkingProcessor* proc, double tmout = -1) throw (StopException);
         void ExitMainLoop(WorkingProcessor* proc);

         void RunExplicitLoop();
         virtual void ProcessorNumberChanged() {} // function called when number of processors are changed
         virtual bool CheckThreadUsage(); // checks if there is any reasons to continue usage of thread

         EStatus              fState;

         bool                 fThrdWorking; // flag indicates if mainloop of the thread should continue to work
         bool                 fNoLongerUsed; // set to true when thread has nothing to do
         bool                 fRealThrd;    // indicate if we create real thread and not running mainloop from top process

         Mutex                fWorkMutex; // main mutex of the thread

         Condition            fWorkCond; // condition, which is used in default MainLoop implementation

         EventsQueue         *fQueues;
         int                  fNumQueues;

         CommandsQueue        fSysCommands;

         TimeSource           fTime;        // source of time stamps
         TimeStamp_t          fNextTimeout; // indicate when we expects next timeout

         ProcessorsVector     fProcessors;   // vector of all processors

         WorkingProcessor*    fExplicitLoop;
         bool                 fExitExplicitLoop; // set to true, when one should exit from main loop
   };
}

#endif
