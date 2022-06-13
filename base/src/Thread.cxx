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

#include "dabc/Thread.h"

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/resource.h>
//#include <linux/version.h>

#include "dabc/Manager.h"
#include "dabc/Configuration.h"


#ifdef DABC_EXTRA_CHECKS
unsigned dabc::Thread::maxlimit = 1000;
#endif



std::string dabc::EventId::asstring() const
{
   return dabc::format("Code:%x Item:%x Arg:%x", GetCode(), GetItem(), GetArg());
}


/** \brief  - helper class to use methods, available in dabc::Worker in thread itself
 *
 *   Delivers commands, manage parameters and so on
 */



class dabc::Thread::ExecWorker : public dabc::Worker {
   protected:

      friend class Thread;

      bool fPublish;     ///! if true, different thread parameters will be published

      int fCnt;

   public:
      ExecWorker(Thread* parent, Command cmd) :
         dabc::Worker(parent, "Exec"),
         fPublish(false),
         fCnt(0)
      {
         SetWorkerPriority(0);
         // special case - thread keep only pointer
         SetAutoDestroy(false);

         // all threads configuration should be found on the top-level
         SetFlag(flTopXmlLevel, true);

         fThread = ThreadRef(parent);
         fThreadMutex = parent->ThreadMutex();
         fWorkerId = parent->fWorkers.size();
         fWorkerActive = true;

         if (!fThread()->IsTemporaryThread()) {
            fThread()->fProfiling = Cfg("prof", cmd).AsBool(false);
            fPublish = fThread()->fProfiling || Cfg("publ", cmd).AsBool(false);
            DOUT2("Exec %s publ %s", fThread.GetName(), DBOOL(fPublish));
         }
      }

      virtual ~ExecWorker() {
         DOUT3("Destroy EXEC worker %p", this);
      }

      /** Just workaround to check if execution is still performed */
      bool DirtyWorkaround()
      {
         if (fWorkerCommandsLevel <= 0) return false;
         fThread.Release();
         fThreadMutex = nullptr;
         return true;
      }


      const char* ClassName() const  override { return "Thread"; }

      int ExecuteCommand(Command cmd) override
      {
         int res = cmd_ignore;

         if (!fThread.null())
             res = fThread()->ExecuteThreadCommand(cmd);

         if (res == cmd_ignore) res = dabc::Worker::ExecuteCommand(cmd);

         return res;
      }

      bool Find(ConfigIO &cfg) override
      {
         while (cfg.FindItem(xmlThreadNode)) {
            // DOUT0("Worker found thread node");
            if (cfg.CheckAttr(xmlNameAttr, fThread.GetName())) return true;
         }
         return false;
      }

      void BeforeHierarchyScan(Hierarchy &) override
      {
      }

      double ProcessTimeout(double) override
      {
         // timeout is used to update published hierarchy
         if (!fPublish || fThread.null()) return -1;

         // we need to wait for the publisher itself (if it anytime will be created)
         if (GetPublisher().null()) return 1.;

         if (fWorkerHierarchy.null()) {
            fWorkerHierarchy.Create("Thread");
            dabc::Hierarchy item = fWorkerHierarchy.CreateHChild("NumWorkers");
            item.SetField(dabc::prop_kind, "rate");
            item.EnableHistory(100);

            item = fWorkerHierarchy.CreateHChild("Workers");
            item.SetField(dabc::prop_kind, "log");
            item.EnableHistory(100);

            item = fWorkerHierarchy.CreateHChild("pid");
            item.SetField(dabc::prop_kind, "log");

            item = fWorkerHierarchy.CreateHChild("threadid");
            item.SetField(dabc::prop_kind, "log");

            if (fThread()->fProfiling) {
               item = fWorkerHierarchy.CreateHChild("Load");
               item.SetField(dabc::prop_kind, "rate");
               item.SetField("min", 0);
               item.SetField("max", 1);
               item.EnableHistory(100);
            }

            Publish(fWorkerHierarchy, std::string("$MGR$") + fThread.ItemName());
         }

         unsigned num(0);
         std::vector<std::string> names;

         for (unsigned n=1;n<fThread()->fWorkers.size();n++) {
            Worker* work = fThread()->fWorkers[n]->work;
            if (work==0) continue;
            num++;
            names.push_back(work->GetName());
         }

         fWorkerHierarchy.GetHChild("NumWorkers").SetField("value", num);
         fWorkerHierarchy.GetHChild("Workers").SetField("value", names);
         fWorkerHierarchy.GetHChild("pid").SetField("value", getpid());
         fWorkerHierarchy.GetHChild("threadid").SetField("value", (uint64_t) Self());

         if (fThread()->fProfiling) {

            double real_tm = fThread()->fLastProfileTime.SpentTillNow(true);
            double run_tm = 0.;
            // #if  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
            #ifdef RUSAGE_THREAD
            struct rusage usage;

            if (getrusage(RUSAGE_THREAD, &usage) == 0) {

               double curr =
                     usage.ru_utime.tv_sec * 1. +
                     usage.ru_utime.tv_usec * 1e-6 +
                     usage.ru_stime.tv_sec * 1. +
                     usage.ru_stime.tv_usec * 1e-6;

               run_tm = curr - fThread()->fThreadRunTime;
               fThread()->fThreadRunTime = curr;
            }
            #endif

            if ((real_tm>0) && (run_tm>0)) {
               double load = run_tm / real_tm;
               if (load > 1) load  = 1.;
               fWorkerHierarchy.GetHChild("Load").SetField("value", load);
            }
         }

         fWorkerHierarchy.MarkChangedItems();

         return 1.;
      }

};

unsigned dabc::Thread::fThreadInstances = 0;

dabc::Thread::Thread(Reference parent, const std::string &name, Command cmd) :
   Object(parent, name),
   PosixThread(),
   Runnable(),
   fState(stCreated),
   fThrdWorking(false),
   fRealThrd(true),
   fWorkCond(fObjectMutex),
   fQueues(0),
   fNumQueues(3),
   fNextTimeout(),
   fProcessingTimeouts(0),
   fWorkers(),
   fExplicitLoop(0),
   fExec(0),
   fDidDecRefCnt(false),
   fCheckThrdCleanup(false),
   fProfiling(false),
   fLastProfileTime(),
   fThreadRunTime(0.),
   fThrdStopTimeout(0.)
{
   fThreadInstances++;

   // hide all possible thread childs from hierarchy scan
   SetFlag(flChildsHidden, true);

   DOUT3("---------- CNT:%2d Thread %s %p created", fThreadInstances, GetName(), this);

   fWorkers.push_back(new WorkerRec(0,0)); // exclude id==0

   fExec = new ExecWorker(this, cmd);
   //fExec->SetLogging(true);

   // keep numqueues 3 for the moment
   //fNumQueues = fExec->Cfg("NumQueues", cmd).AsUInt(fNumQueues);

   if (fNumQueues>0) {
     fQueues = new EventsQueue[fNumQueues];
     for (int n=0;n<fNumQueues;n++) {
        fQueues[n].Init(256);
        fQueues[n].scaler = 8;
     }
   }

   std::string affinity = fExec->Cfg(xmlAffinity, cmd).AsStr();

   if (!affinity.empty()) {
      SetAffinity(affinity.c_str());
      char sbuf[200];
      if (GetAffinity(false, sbuf, sizeof(sbuf)))
         DOUT0("Thread %s specified affinity %s mask %s", GetName(), affinity.c_str(), sbuf);
   }

   fThrdStopTimeout = fExec->Cfg(xmlThrdStopTime, cmd).AsDouble();
   if ((fThrdStopTimeout <= 0) && !dabc::mgr.null()) fThrdStopTimeout = dabc::mgr()->cfg()->GetThrdStopTime();
   if (fThrdStopTimeout <= 0) fThrdStopTimeout = 5.;

   fWorkers.push_back(new WorkerRec(fExec,0));

//   SetLogging(true);

   DOUT2("-------- THRD %s Constructed -------------- ", GetName());
}

dabc::Thread::~Thread()
{
   // !!!!!!!! Do not forgot stopping thread in destructors of inherited classes too  !!!!!

   DOUT3("~~~~~~~~~~~~~~ THRD %s destructor with timeout %3.1f s", GetName(), GetStopTimeout());

   // we stop thread in destructor, in all inherited classes stop also should be called
   // otherwise one get problem here if stop will use inherited methods which is no longer available

   //Stop(1.); JAM 6.7.2017 - try with larger timeout for ltsm
   Stop(GetStopTimeout());

   ExecWorker* exec = 0;

   {
      LockGuard lock(ObjectMutex());
      // Workaround - Exec processor is keeping one reference
      // We decrease it during cleanup, now increase it again
      if (fDidDecRefCnt) fObjectRefCnt++;
      exec = fExec;
      fExec = 0;
   }

   exec->ClearThreadRef();
   if (exec->DirtyWorkaround())
      EOUT("Execution instance for the thread %s is blocked - KEEP EXEC ALIVE AND FIX LATER", GetName());
   else
      dabc::Object::Destroy(exec); // normally destroy should be called

   for (unsigned n=0;n<fWorkers.size();n++) {
      if (fWorkers[n]) {
         //EOUT("Still non-empty worker rec %u  in thread %s destructor", n, GetName());
         delete fWorkers[n];
         fWorkers[n] = 0;
      }
   }

   LockGuard guard(ThreadMutex());
   if (fState==stError) {
      EOUT("Kill thread in error state, nothing better can be done");
      Kill();
      fState = stStopped;
   }

#ifdef DABC_EXTRA_CHECKS
   unsigned totalsize = 0;
   for (int n=0;n<fNumQueues;n++) {
      totalsize+=fQueues[n].Size();
      for (unsigned k=0; k<fQueues[n].Size(); k++) {
         EventId evnt = fQueues[n].Item(k);
         DOUT0("THRD %s Queue:%u Item:%u Event:%s", GetName(), n, k, evnt.asstring().c_str());
      }
   }
   if (totalsize>0)
      EOUT("THRD %s %u events are not processed", GetName(), totalsize);
#endif

   delete [] fQueues; fQueues = 0;
   fNumQueues = 0;

   DOUT3("~~~~~~~~~~~~~~ THRD %s destroyed cnt:%d", GetName(), fThreadInstances);

   fThreadInstances--;
}


void dabc::Thread::IncWorkerFiredEvents(Worker* work)
{
   work->fWorkerFiredEvents++;
}

unsigned dabc::Thread::_TotalNumberOfEvents()
{
   unsigned totalsize = 0;
   for (int n=0;n<fNumQueues;n++)
      totalsize+=fQueues[n].Size();
   return totalsize;
}

unsigned dabc::Thread::TotalNumberOfEvents()
{
   LockGuard lock(ThreadMutex());
   return _TotalNumberOfEvents();
}

void dabc::Thread::ProcessNoneEvent()
{
   LockGuard lock(ThreadMutex());

   if (!fCheckThrdCleanup) return;

   fCheckThrdCleanup = false;

   DOUT3("THREAD %s check cleanup", GetName());

   // here we doing cleanup when no any events there

   unsigned new_size(fWorkers.size());

   while ((new_size>1) && (fWorkers[new_size-1]->work==0)) {
      new_size--;
      delete fWorkers[new_size];
      fWorkers[new_size] = 0;
   }

   DOUT3("THREAD %s oldsize %u newsize %u", GetName(), fWorkers.size(), new_size);

   if (new_size==fWorkers.size()) return;

   fWorkers.resize(new_size);
   DOUT3("Thrd:%s Shrink processors size to %u normal state %s refcnt %d", GetName(), new_size, DBOOL(_IsNormalState()), fObjectRefCnt);

   // we check that object is in normal state,
   // otherwise it means that destroyment is already started and will be done in other means
   if ((new_size==2) && _IsNormalState()) {
      DOUT3("THREAD %s generate cleanup", GetName());
      _Fire(EventId(evntCleanupThrd, 0, 0), priorityLowest);
   }
}

bool dabc::Thread::_GetNextEvent(dabc::EventId& evnt)
{
   // return next event from the queues
   // in general, events returned according their priority
   // but in rare cases lower-priority events also can be taken even when high-priority events exists
   // This will allow to react on such events.
   //
   // If there are no events in the queue, one checks if thread should be cleaned up and
   // even destroyed


   for(int nq=0; nq<fNumQueues; nq++)
      if (fQueues[nq].Size()>0) {
         if (--(fQueues[nq].scaler)>0) {
            evnt = fQueues[nq].Pop();
            return true;
         }
         fQueues[nq].scaler = 8;
      }

   // in second loop check all queues according their priority

   for(int nq=0; nq<fNumQueues; nq++)
      if (fQueues[nq].Size()>0) {
         evnt = fQueues[nq].Pop();
         return true;
      }

   return false;
}


bool dabc::Thread::CompatibleClass(const std::string &clname) const
{
   if (clname.empty()) return true;

   return clname == typeThread;
}

void* dabc::Thread::MainLoop()
{
   EventId evid;
   double tmout;

   DOUT3("*** Thrd:%s Starting MainLoop PROCID = %u THRDID %u", GetName(), (unsigned) getpid(), (unsigned) syscall(SYS_gettid));

   while (fThrdWorking) {

      DOUT5("*** Thrd:%s Checking timeouts", GetName());

      tmout = CheckTimeouts();

      DOUT5("*** Thrd:%s Check timeouts %5.3f", GetName(), tmout);

      if (WaitEvent(evid, tmout)) {

         DOUT5("*** Thrd:%s GetEvent %s", GetName(), evid.asstring().c_str());

         ProcessEvent(evid);

         DOUT5("*** Thrd:%s DidEvent %s", GetName(), evid.asstring().c_str());
      } else
         ProcessNoneEvent();

      if (fExplicitLoop!=0) RunExplicitLoop();
   }

   DOUT3("*** Thrd:%s Leaving MainLoop", GetName());

   return nullptr;
}

bool dabc::Thread::SingleLoop(unsigned workerid, double tmout_user)
{
   DOUT5("*** Thrd:%s SingleLoop user_tmout %5.3f", GetName(), tmout_user);

   // check situation that worker is halted and should brake its execution
   // if necessary, worker should fire exception
   if ((workerid>0) && (workerid<fWorkers.size())) {
      if (fWorkers[workerid]->doinghalt) return false;
   }

   double tmout = CheckTimeouts();
   if (tmout_user>=0)
      if ((tmout<0) || (tmout_user<tmout)) tmout = tmout_user;

   EventId evid;

   if (WaitEvent(evid, tmout))
      ProcessEvent(evid);
   else
      ProcessNoneEvent();

   return true;
}

void dabc::Thread::RunEventLoop(double tm)
{
   if (!IsItself()) {
      EOUT("Cannot run thread %s event loop outer own thread", GetName());
      return;
   }

   if (tm<0) {
      EOUT("negative (endless) timeout specified - set default 0 sec (single event)");
      tm = 0;
   }

   TimeStamp finish = dabc::Now() + tm;

   while (fThrdWorking) {
      double remain = finish - dabc::Now();

      // we execute event loop at least once even when no time remains
      SingleLoop(0, remain>0 ? remain : 0.);

      if (remain <= 0) break;
   }
}


int dabc::Thread::RunCommandInTheThread(Worker *caller, Worker *dest, Command cmd)
{
   if (cmd.null() || !dest) {
      cmd.ReplyFalse();
      return dabc::cmd_false;
   }

   {
      dabc::LockGuard lock(ObjectMutex());

      // in principle, it is enough to check state of the thread
      // if destructor is started, we can reject submission of all commands
      if (!_IsNormalState()) return cmd_ignore;

      if (!caller) caller = fExec;
   }


   // we must be sure that call is done from thread itself - otherwise it is wrong
   if (!IsItself()) {
      EOUT("Cannot execute command in wrong thread context!!!");
      cmd.ReplyFalse();
      return cmd_false;
   }

   int res = cmd_false;

   { // this is begin of parenthesis for RecursionGuard

      // we indicate that processor involved in
      Thread::RecursionGuard iguard(this, caller->fWorkerId);

      bool exe_ready = false;

      cmd.AddCaller(caller, &exe_ready);

    try {

      DOUT3("********** Calling ExecteIn in thread %s %p", GetName(), this);

      // critical point - we want to submit command to other thread
      // if command receiver does not accept command means it either do not have thread or lost it
      // in this case command can be executed in current thread context ???
      // Once command is submitted it is guaranteed that it will be executed or command will be canceled

      if (dest->Submit(cmd)) {

         // we can access exe_ready directly, while this flag only access from caller thread
         // loop should be executed at least once to process do-nothing event produced by command reply

         do {

            // account timeout, whait 0.5 second longer as specified timeout
            double tmout = cmd.TimeTillTimeout(0.5);

            if (tmout == 0.) {
               res = cmd_timedout;
               break;
            }

            DOUT3("ExecuteIn - cmd:%s singleLoop proc %u time %4.1f", cmd.GetName(), caller->fWorkerId, ((tmout<=0) ? 0.1 : tmout));

            if (!SingleLoop(caller->fWorkerId, (tmout <= 0.) ? 0.1 : tmout)) {
               // FIXME: one should cancel command in normal way
               res = cmd_false;
               break;
            }
         } while (!exe_ready);
         DOUT3("------------ Proc %p Cmd %s ready = %s", caller, cmd.GetName(), DBOOL(exe_ready));

         if (exe_ready) res = cmd.GetResult();

      } else {

         // this is a case when command can be executed in current thread context

         // FIXME: should we do this - if destination does not accept command via Submit, should we execute it that way?

         DOUT0("Worker %s refuse to submit command - we do it as well", dest->GetName());
         res = cmd_false;
         cmd.SetResult(cmd_false);
      }

      // in any case remove caller from the command
      cmd.RemoveCaller(caller, &exe_ready);

    } catch (...) {

      // even in case of exception
      cmd.RemoveCaller(caller, &exe_ready);
    }

   } // this is end of parenthesis for RecursionGuard, should be closed before thread reference is released

   DOUT3("------------ Proc %p Cmd %s res = %d", caller, cmd.GetName(), res);

   return res;
}



bool dabc::Thread::Start(double timeout_sec, bool real_thread)
{
   // first, check if we should join thread,
   // when it was stopped before
   bool needkill = false;
   {
      LockGuard guard(ThreadMutex());
      switch (fState) {
         case stCreated:
         case stStopped:
            fRealThrd = real_thread;
            break;
         case stRunning:
            return true;
         case stError:
            EOUT("Restart from error state, may be dangerous");
            needkill = fRealThrd;
            fRealThrd = real_thread;
            break;
         case stChanging:
            EOUT("Status is changing from other thread. Not supported");
            return false;
         default:
            EOUT("Forgot something???");
            break;
      }

      fState = stChanging;
   }

   DOUT3("Thread %s starting kill:%s", GetName(), DBOOL(needkill));

   if (needkill) PosixThread::Kill();

   // from this moment on thread main loop must became functional

   fThrdWorking = true;
   bool res = true;

   if (fRealThrd) {
      PosixThread::Start(this);

      std::string tname = GetName();
      if (tname.length()>15) tname.resize(15);

      PosixThread::SetThreadName(tname.c_str());

      if (!fExec) {
         EOUT("Start thread without EXEC???");
         exit(765);
      }
      res = fExec->Execute("ConfirmStart", timeout_sec) == cmd_true;
   } else {
      PosixThread::UseCurrentAsSelf();
   }

   LockGuard guard(ThreadMutex());
   fState = res ? stRunning : stError;

   return res;
}

void dabc::Thread::RunnableCancelled()
{
   DOUT3("Thread cancelled %s", GetName());

   LockGuard guard(ThreadMutex());

   fState = stStopped;
}


bool dabc::Thread::Stop(double timeout_sec)
{
   bool needstop = false;

   {
      LockGuard guard(ThreadMutex());
      switch (fState) {
         case stCreated:
            return true;
         case stRunning:
            needstop = fRealThrd && !IsItself();
            break;
         case stStopped:
            return true;
         case stError:
            EOUT("Stop from error state, do nothing");
            return true;
         case stChanging:
            EOUT("State is changing from other thread. Not supported");
            return false;
         default:
            EOUT("Forgot something???");
            return false;
      }

      fState = stChanging;

      if (needstop)
         _Fire(EventId(evntStopThrd), priorityHighest);
   }

   bool res(false);

   DOUT3("Start doing thread %s stop with timeout %f s", GetName(),timeout_sec);

   if (!needstop) {

      fThrdWorking = false;
      res = true;
      LockGuard guard(ThreadMutex());
      fState = stStopped;

   } else {
      if (timeout_sec<=0.) timeout_sec = 1.;

      // FIXME: one should avoid any kind of timeouts and use normal condition here

      TimeStamp tm1 = dabc::Now();

      int cnt(0);
      double spent_time(0.);
      bool did_cancel(false);

      do {
         if (cnt++>1000) dabc::Sleep(0.001);

         if ((spent_time > timeout_sec * 0.7) && !did_cancel) {
            DOUT1("Cancel thread %s", GetName());
            PosixThread::Cancel();
            did_cancel = true;
            cnt = 0;
         }

         LockGuard guard(ThreadMutex());
         if (fState == stStopped) { res = true; break; }

      } while ( (spent_time = tm1.SpentTillNow()) < timeout_sec);

      if (!res) EOUT("Cannot wait for join while stop was not succeeded");
          else PosixThread::Join();
   }

   LockGuard guard(ThreadMutex());

   if (fState!=stStopped) {
      // not necessary, but to be sure that everything is done to stop thread
      fThrdWorking = false;
      fState = stError;
   } else
      fState = stStopped;

   return res;
}

bool dabc::Thread::Sync(double timeout_sec)
{
   return fExec->Execute("ConfirmSync", timeout_sec);
}

bool dabc::Thread::SetExplicitLoop(Worker* proc)
{
   if (!IsItself()) {
      EOUT("Call from other thread - absolutely wrong");
      exit(113);
   }

   if (fExplicitLoop!=0)
      EOUT("Explicit loop is already set");
   else
      fExplicitLoop = proc->fWorkerId;

   return fExplicitLoop == proc->fWorkerId;
}

void dabc::Thread::RunExplicitLoop()
{
   if ((fExplicitLoop==0) || (fExplicitLoop>=fWorkers.size())) return;

   DOUT4("Enter RunExplicitMainLoop");

   // first check that worker want to be halted, when do not start explicit loop at all
   if (fWorkers[fExplicitLoop]->doinghalt) return;

   RecursionGuard iguard(this, fExplicitLoop);

   try {

     fWorkers[fExplicitLoop]->work->DoWorkerMainLoop();

  } catch (dabc::Exception& e) {
     if (e.IsStop()) DOUT2("Worker %u stopped via exception", fExplicitLoop); else
     if (e.IsTimeout()) DOUT2("Worker %u stopped via timeout", fExplicitLoop); else
     EOUT("Exception %s in processor %u", e.what(), fExplicitLoop);
  } catch(...) {
     EOUT("Exception UNCKNOWN in processor %u", fExplicitLoop);
  }

  // we should call postloop in any case
  fWorkers[fExplicitLoop]->work->DoWorkerAfterMainLoop();

  DOUT5("Exit from RunExplicitMainLoop");

  fExplicitLoop = 0;
}


void dabc::Thread::FireDoNothingEvent()
{
   // used by timeout object to activate thread and leave WaitEvent function

   Fire(EventId(evntDoNothing), priorityLowest);
}

int dabc::Thread::ExecuteThreadCommand(Command cmd)
{
   DOUT2("Thread %s  Execute command %s", GetName(), cmd.GetName());

   if (cmd.IsName("ConfirmStart")) {

      DOUT3("THRD:%s item %s", GetName(), ItemName().c_str());

      // activate timeout at least once
      fExec->ActivateTimeout(0.01);

      return cmd_true;
   } else
   if (cmd.IsName("ConfirmSync")) {
      return cmd_true;
   } else
   if (cmd.IsName("AddWorker")) {

      Reference ref = cmd.GetRef("Worker");
      Worker* worker = (Worker*) ref();

      DOUT2("AddWorker %p in thrd %p", worker, this);

      if (worker==0) return cmd_false;

      if ((worker->fThread() != this) ||
          (worker->fThreadMutex != ThreadMutex())) {
         EOUT("Something went wrong - CRASH");
         ref.Destroy();
         exit(765);
      }

      {
         LockGuard guard(ThreadMutex());

         // we can use workers array outside mutex (as long as we are inside thread)
         // but we should lock mutex when we would like to change workers vector
         fWorkers.push_back(new WorkerRec(worker, worker->fAddon()));

         // from this moment on processor is fully functional
         worker->fWorkerId = fWorkers.size()-1;

         worker->fWorkerActive = true;

         DOUT3("----------------THRD %s WORKER %p %s  assigned ------------------ ", GetName(), worker, worker->GetName());

      }

      WorkersSetChanged();

      worker->InformThreadAssigned();

      //cmd->Print(1, "DIDjob");

      return cmd_true;

   } else

   if (cmd.IsName("InvokeWorkerDestroy")) {

      DOUT3("THRD:%s Request to destroy worker id %u", GetName(), cmd.GetUInt("WorkerId"));

      return CheckWorkerCanBeHalted(cmd.GetUInt("WorkerId"), actDestroy, cmd);
   } else

   if (cmd.IsName("HaltWorker")) {

      DOUT3("THRD:%s Request to halt worker id %u", GetName(), cmd.GetUInt("WorkerId"));

      return CheckWorkerCanBeHalted(cmd.GetUInt("WorkerId"), actHalt, cmd);
   }

   return cmd_ignore;
}


void dabc::Thread::ChangeRecursion(unsigned id, bool inc)
{
#ifdef DABC_EXTRA_CHECKS
   if (!IsItself()) {
      EOUT("ALARM, recursion changed not from thread itself");
   }
#endif

   if ((id>=fWorkers.size()) || (fWorkers[id]->work==0)) return;

   if (inc) {
      fWorkers[id]->recursion++;
   } else {
      fWorkers[id]->recursion--;

      if ((fWorkers[id]->recursion==0) && fWorkers[id]->doinghalt)
         CheckWorkerCanBeHalted(id);
   }
}

int dabc::Thread::CheckWorkerCanBeHalted(unsigned id, unsigned request, Command cmd)
{
   DOUT4("THRD:%s CheckWorkerCanBeHalted %u", GetName(), id);

   if ((id>=fWorkers.size()) || (fWorkers[id]->work==0)) {
      DOUT3("THRD:%s Worker %u no longer exists", GetName(), id);

      return cmd_false;
   }

   fWorkers[id]->doinghalt |= request;

   unsigned balance = 0;

   {
      LockGuard guard(ThreadMutex());
      // we indicate that worker should not produce more normal events
      // it will be able to supply commands with magic priority
      if (fWorkers[id]->doinghalt)
         fWorkers[id]->work->fWorkerActive = false;

      balance = fWorkers[id]->work->fWorkerFiredEvents - fWorkers[id]->processed;
   }

   DOUT4("THRD:%s CheckWorkerCanBeHalted %u doinghalt = %u", GetName(), id, fWorkers[id]->doinghalt);

   if (fWorkers[id]->doinghalt==0) return cmd_false;

   if ((fWorkers[id]->recursion > 0)  || (balance > 0)) {
      DOUT2("THRD:%s ++++++++++++++++++++++ worker %p %s %s event balance %u fired:%u processed:%u recursion %d",
            GetName(), fWorkers[id]->work, fWorkers[id]->work->GetName(), fWorkers[id]->work->ClassName(),
            balance, fWorkers[id]->work->fWorkerFiredEvents, fWorkers[id]->processed,
            fWorkers[id]->recursion);
      if (!cmd.null()) fWorkers[id]->cmds.Push(cmd);
      return cmd_postponed;
   }

   WorkerRec* rec(0);

   {
      LockGuard guard(ThreadMutex());

      // this excludes worker from any further event processing
      // do it under lock

      rec = fWorkers[id];

      DOUT5("Thrd:%s Remove record %u\n", GetName(), id);

      fWorkers[id] = new WorkerRec(0, 0);

      // reset id
      if (rec->work) rec->work->fWorkerId = 0;
   }

   DOUT4("THRD:%s CheckWorkerCanBeHalted %u rec = %p worker = %p", GetName(), id, rec, rec ? rec->work : 0);

   // FIXME: this must be legitime method to destroy any worker
   //        one can remove it from workers vector

   // before worker will be really destroyed indicate to the world that processor is disappear
   WorkersSetChanged();

   if (rec!=0) {

      if (rec->work && rec->work->IsLogging())
         DOUT0("Trying to destroy worker %p id %u via thread %s", rec->work, id, GetName());

      // release thread reference from here
      if (rec->work) rec->work->ClearThreadRef();

      // true indicates that object should be destroyed immediately
      if (rec->doinghalt & actDestroy) {
         if (rec->work && rec->work->DestroyCalledFromOwnThread())
            delete rec->work;
      }

      // inform all commands that everything goes well
      rec->cmds.ReplyAll(cmd_true);

      delete rec;
   }

   LockGuard guard(ThreadMutex());

   DOUT2("THRD:%s mark thread for cleanup check", GetName());

   // indicate for thread itself that it can be optimized
   fCheckThrdCleanup = true;

   return cmd_true;
}


void dabc::Thread::_Fire(const EventId& arg, int nq)
{
   DOUT3("Thrd: %p %s Fire event code:%u item:%u arg:%u nq:%d NumQueues:%u", this, GetName(), arg.GetCode(), arg.GetItem(), arg.GetArg(), nq, fNumQueues);

   _PushEvent(arg, nq);
   fWorkCond._DoFire();

#ifdef DABC_EXTRA_CHECKS

   long sum = 0;
   for (int n=0;n<fNumQueues;n++) sum+=fQueues[n].Size();
   if (sum!=fWorkCond._FiredCounter()) {
      dabc::SetDebugLevel(5);
      DOUT5("Thrd %s Error sum1 %ld cond %ld  event %s",
            GetName(), sum, fWorkCond._FiredCounter(),
            arg.asstring().c_str());
   }
#endif

}

bool dabc::Thread::WaitEvent(EventId& evid, double tmout)
{

   LockGuard lock(ThreadMutex());

   if (fWorkCond._DoWait(tmout))
      return _GetNextEvent(evid);

   return false;
}


void dabc::Thread::ProcessEvent(const EventId& evnt)
{
   uint16_t itemid = evnt.GetItem();

   DOUT5("*** Thrd:%s Event:%s  q0:%u q1:%u q2:%u",
         GetName(), evnt.asstring().c_str(),
         fQueues[0].Size(), fQueues[1].Size(), fQueues[2].Size());

   if (itemid>0) {
      if (itemid>=fWorkers.size()) {
         EOUT("Thrd:%p %s FALSE worker id:%u size:%lu evnt:%s - ignore", this, GetName(), itemid, (long unsigned) fWorkers.size(), evnt.asstring().c_str());
         return;
      }

      Worker* worker = fWorkers[itemid]->work;

      DOUT3("*** Thrd:%p proc:%p itemid:%u event:%u doinghalt:%u", this, worker, itemid, evnt.GetCode(), fWorkers[itemid]->doinghalt);

      if (worker==0) return;

      fWorkers[itemid]->processed++;

      if (worker==dabc::mgr())
         DOUT3("Process manager event %s fired:%u processed: %u", evnt.asstring().c_str(), worker->fWorkerFiredEvents, fWorkers[itemid]->processed);

      try {

         DOUT3("*** Thrd:%p proc:%s event:%u", this, worker->GetName(), evnt.GetCode());

         IntGuard iguard(fWorkers[itemid]->recursion);

         if (evnt.GetCode() < Worker::evntFirstSystem) {
            if (evnt.GetCode() < Worker::evntFirstAddOn)
               worker->ProcessCoreEvent(evnt);
            else {
               if (worker->fAddon.null())
                  EOUT("Get event for non-existing addon");
               else
                  worker->fAddon()->ProcessEvent(evnt);
            }
         } else
            worker->ProcessEvent(evnt);

      } catch (...) {

         if (fWorkers[itemid]->doinghalt)
            CheckWorkerCanBeHalted(itemid);

         throw;
      }

      // this block will be executed also if exception was produced by user
      if (fWorkers[itemid]->doinghalt)
         CheckWorkerCanBeHalted(itemid);

   } else

   switch (evnt.GetCode()) {
      case evntCheckTmoutWorker: {

         if (evnt.GetArg() >= fWorkers.size()) {
            DOUT3("evntCheckTmoutWorker - mismatch in processor id:%u sz:%u ", evnt.GetArg(), fWorkers.size());
            break;
         }

         WorkerRec* rec = fWorkers[evnt.GetArg()];
         if (rec->work==0) {
            DOUT3("Worker no longer exists", evnt.GetArg());
            break;
         }

         if (rec->tmout_worker.CheckEvent(ThreadMutex())) CheckTimeouts(true);

         break;
      }

      case evntCheckTmoutAddon: {

         if (evnt.GetArg() >= fWorkers.size()) {
            DOUT3("evntCheckTmoutWorker - mismatch in processor id:%u sz:%u ", evnt.GetArg(), fWorkers.size());
            break;
         }

         WorkerRec* rec = fWorkers[evnt.GetArg()];
         if (rec->work==0) {
            DOUT3("Worker no longer exists", evnt.GetArg());
            break;
         }

         if (rec->tmout_addon.CheckEvent(ThreadMutex())) CheckTimeouts(true);

         break;
      }


      case evntCleanupThrd: {

         unsigned totalsize(0);
         {
            LockGuard lock(ThreadMutex());
            totalsize = _TotalNumberOfEvents();

            // if cleanup was started due to no workers in thread,
            // one should stop it while thread will be deleted by other means
            if ((evnt.GetArg() < 100) && !_IsNormalState()) break;
         }

         if (fWorkers.size()!=2) {
            // this is situation when cleanup was started by DecReference while
            // there is no more references on the thread and one can destroy thread
            // one need to ensure that no more other events existing
            DOUT3("Thrd:%s Cleanup running when num workers %u != 2 - something strange", GetName(), fWorkers.size());
         }

         DOUT3("THRD:%s Num workers = %u totalsize %u", GetName(), fWorkers.size(), totalsize);

         if ((totalsize>0) && (evnt.GetArg() % 100 < 20)) {
            LockGuard lock(ThreadMutex());
            _Fire(EventId(evntCleanupThrd, 0, evnt.GetArg()+1), priorityLowest);
         } else {

            if (totalsize>0)
               EOUT("THRD %s %u events are not processed", GetName(), totalsize);

            if (dabc::mgr.null()) {
               printf("Cannot normally destroy thread %s while manager reference is already empty\n", GetName());
               fThrdWorking = false;
            } else
            if (!dabc::mgr()->DestroyObject(this)) {
               EOUT("Thread cannot be normally destroyed, just leave main loop");
               fThrdWorking = false;
            } else {
               DOUT3(" -------- THRD %s refcnt %u DESTROYMENT GOES TO MANAGER", GetName(), fObjectRefCnt);
            }
         }

         break;
      }

      case evntDoNothing:
         break;

      case evntStopThrd: {
         DOUT3("Thread %s get stop event", GetName());
         fThrdWorking = false;
         break;
      }

      default:
         ProcessExtraThreadEvent(evnt);
         break;
   }

   DOUT5("Thrd:%s Item:%u Event:%u arg:%u done", GetName(), itemid, evnt.GetCode(), evnt.GetArg());
}


bool dabc::Thread::AddWorker(Reference ref, bool sync)
{
   Command cmd("AddWorker");
   cmd.SetRef("Worker", ref);
   return sync ? fExec->Execute(cmd) : fExec->Submit(cmd);
}

bool dabc::Thread::Execute(dabc::Command cmd, double tmout)
{
   cmd.SetPriority(Worker::priorityMaximum);

   return fExec->Execute(cmd, tmout);
}

bool dabc::Thread::HaltWorker(Worker* work)
{
   if (work==0) return false;

   if (IsItself())
      return CheckWorkerCanBeHalted(work->fWorkerId, actHalt) == cmd_true;

   Command cmd("HaltWorker");
   cmd.SetUInt("WorkerId", work->fWorkerId);
   cmd.SetPriority(Worker::priorityMaximum);
   return fExec->Execute(cmd) == cmd_true;
}


void dabc::Thread::WorkerAddonChanged(Worker* work, bool assign)
{
   if (work==0) return;

   if (!IsItself()) {
      EOUT("Not allowed from other thread");
      exit(333);
   }

   if (work->WorkerId() >= fWorkers.size()) {
      EOUT("Mismatch of workers IDs");
      exit(333);
   }

   WorkerRec* rec = fWorkers[work->WorkerId()];

   if (rec->work != work) {
      EOUT("%s Mismatch of worker id %u rec: %p worker: %p ", GetName(), work->WorkerId(), rec->work, work);
      exit(444);
   }

   rec->addon = assign ? work->fAddon() : 0;

   WorkersSetChanged();
}


bool dabc::Thread::InvokeWorkerDestroy(Worker* work)
{
   // TODO: one must be sure that command is executed,
   // therefore state of the thread must be checked
   // This action can only work asynchron

   if (work==0) return false;

   Command cmd("InvokeWorkerDestroy");
   cmd.SetUInt("WorkerId", work->fWorkerId);

   DOUT4("Exec %p Invoke to destroy worker %p %s", fExec, work, work->GetName());

   return fExec->Submit(cmd);
}

double dabc::Thread::CheckTimeouts(bool forcerecheck)
{
   if (fProcessingTimeouts>0) return -1.;

   IntGuard guard(fProcessingTimeouts);

   TimeStamp now;

   if (!forcerecheck) {
      if (fNextTimeout.null()) return -1.;
      now.GetNow();
      double dist = fNextTimeout - now;
      if (dist>0.) return dist;
   } else
      now.GetNow();

   double min_tmout(-1.), last_diff(0.);

   for (unsigned n=1;n<fWorkers.size();n++) {
      WorkerRec* rec = fWorkers[n];
      if ((rec==0) || (rec->work==0)) continue;

      if (rec->tmout_worker.CheckNextProcess(now, min_tmout, last_diff)) {

         double dist = rec->work->ProcessTimeout(last_diff);

         rec->tmout_worker.SetNextFire(now, dist, min_tmout);
      }

      if (rec->tmout_addon.CheckNextProcess(now, min_tmout, last_diff)) {

         double dist = rec->work->ProcessAddonTimeout(last_diff);

         rec->tmout_addon.SetNextFire(now, dist, min_tmout);
      }
   }

   if (min_tmout>=0.)
      fNextTimeout = now + min_tmout;
   else
      fNextTimeout.Reset();

   return min_tmout;
}

void dabc::Thread::ObjectCleanup()
{
   DOUT3("---- THRD %s ObjectCleanup refcnt %u", GetName(), fObjectRefCnt);

   // FIXME: should we wait until all commands and all events are processed
   // FIXME: can we delete worker already here??

   // we keep exec worker for a while
   RemoveChild(fExec, false);
   if (fExec->GetParent()!=0) EOUT("PARENT IS STILL THERE");

   {
      LockGuard lock(ObjectMutex());
      // Workaround - Exec processor is keeping one reference
      // We ignore this reference and manager is allowed to destroy thread once any other references are cleared
      fObjectRefCnt--;
      fDidDecRefCnt = true;
   }

   DOUT3("---- THRD %s ObjectCleanup in the middle", GetName());

   dabc::Object::ObjectCleanup();

   DOUT3("---- THRD %s ObjectCleanup done refcnt = %u workerssize = %u", GetName(), NumReferences(), fWorkers.size());
}

bool dabc::Thread::_DoDeleteItself()
{
   // we are outside own thread - let other do dirty job for other, also if thread not working one cannot delete itself
   // FIXME :should we check that there is no events in the queues like
   //          if (!IsItself() && (_TotalNumberOfEvents()==0)) return false;

   if (!IsItself() || !fThrdWorking || !fRealThrd) return false;

   DOUT2("!!!!!!!!!!!! THRD %s DO DELETE ITSELF !!!!!!!!!!!!!!!", GetName());

   _Fire(EventId(evntCleanupThrd, 0, 100), priorityNormal);

   return true;
}


void dabc::Thread::Print(int lvl)
{
   dabc::Object::Print(lvl);

   LockGuard guard(ThreadMutex());

   dabc::lgr()->Debug(lvl, "file", 1, "func", dabc::format("   Workers vector size: %lu", (long unsigned) fWorkers.size()).c_str());

   for (unsigned n=1;n<fWorkers.size();n++) {
      Worker* work = fWorkers[n]->work;

      if (work==0) continue;
      dabc::lgr()->Debug(lvl, "file", 1, "func", dabc::format("   Worker: %u is %p %s %s", n, work, work->GetName(), work->ClassName()).c_str());
   }
}

unsigned dabc::Thread::NumWorkers()
{
   unsigned cnt = 0;
   for (unsigned n=1;n<fWorkers.size();n++)
      if (fWorkers[n]->work != 0) cnt++;

   return cnt;
}

// -----------------------------------------------------------------

bool dabc::ThreadRef::Execute(dabc::Command cmd, double tmout)
{
   if (!GetObject()) return false;

   return GetObject()->Execute(cmd, tmout);
}


void dabc::ThreadRef::RunEventLoop(double tmout)
{
   if (tmout < 0) tmout = 0.;

   if (!GetObject())
      dabc::Sleep(tmout);
   else
      GetObject()->RunEventLoop(tmout);
}

bool dabc::ThreadRef::_ActivateWorkerTimeout(unsigned workerid, int priority, double tmout)
{
   if (!GetObject()) return false;

   if (workerid >= GetObject()->fWorkers.size()) return false;

   Thread::WorkerRec* rec = GetObject()->fWorkers[workerid];

   // TODO: why worker priority is important here ????
   //       with default priority multinode applications (ib-test) not connecting correctly

   if (rec->tmout_worker.Activate(tmout))
      GetObject()->_Fire(EventId(Thread::evntCheckTmoutWorker, 0, workerid), priority);

   return true;
}

bool dabc::ThreadRef::_ActivateAddonTimeout(unsigned workerid, int priority, double tmout)
{
   if (!GetObject()) return false;

   if (workerid >= GetObject()->fWorkers.size()) return false;

   Thread::WorkerRec* rec = GetObject()->fWorkers[workerid];

   // TODO: why worker priority is important here ????
   //       with default priority multinode applications (ib-test) not connecting correctly

   if (rec->tmout_addon.Activate(tmout))
      GetObject()->_Fire(EventId(Thread::evntCheckTmoutAddon, 0, workerid), priority);

   return true;
}


bool dabc::ThreadRef::MakeWorkerFor(WorkerAddon* addon, const std::string &name)
{
   if (null()) return false;
   Worker* worker = new Worker(nullptr, name.empty() ? "dummy" : name.c_str());
   worker->AssignAddon(addon);
   return worker->AssignToThread(*this);
}
