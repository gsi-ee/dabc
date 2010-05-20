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
#include "dabc/WorkingThread.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "dabc/WorkingProcessor.h"
#include "dabc/Command.h"

#ifdef DO_INDEX_CHECK
unsigned dabc::WorkingThread::maxlimit = 1000;
#endif

class dabc::WorkingThread::ExecProcessor : public dabc::WorkingProcessor {
   protected:

      CommandsQueue fPostponed;
   public:
      ExecProcessor() :
         dabc::WorkingProcessor(),
         fPostponed(CommandsQueue::kindSubmit)
      {
         fProcessorPriority = 0;
      }

      virtual int ExecuteCommand(Command* cmd)
      {
         return fProcessorThread->ExecuteThreadCommand(cmd);
      }

      void AddPostponed(Command* cmd)
      {
         fPostponed.Push(cmd);
         ActivateTimeout(0.001);
      }

      virtual double ProcessTimeout(double last_diff)
      {
         if (fPostponed.Size()==0) return -1;
         dabc::Command* cmd = fPostponed.Pop();

         int res = fProcessorThread->ExecuteThreadCommand(cmd);

         if (res!=cmd_postponed) dabc::Command::Reply(cmd, res);

         return (fPostponed.Size()==0) ? -1. : 0.001;
      }

};


dabc::WorkingThread::WorkingThread(Basic* parent, const char* name, unsigned numqueus) :
   Basic(parent, name),
   Thread(),
   Runnable(),
   fState(stCreated),
   fThrdWorking(false),
   fNoLongerUsed(false),
   fRealThrd(true),
   fWorkMutex(),
   fWorkCond(&fWorkMutex),
   fQueues(0),
   fNumQueues(numqueus),
   fTime(),
   fNextTimeout(NullTimeStamp),
   fProcessingTimeouts(0),
   fProcessors(),
   fExplicitLoop(0),
   fExitExplicitLoop(false),
   fExec(0)
{
   if (fNumQueues>0) {
     fQueues = new EventsQueue[fNumQueues];
     for (int n=0;n<fNumQueues;n++)
        fQueues[n].Init(256, true);
   }

   DOUT3(("Thread %s created", GetName()));

   fProcessors.push_back(0); // exclude id==0

   fExec = new ExecProcessor;
   fExec->fProcessorId = fProcessors.size();
   fProcessors.push_back(fExec);
   fExec->fProcessorThread = this;
   fExec->fProcessorMainMutex = &fWorkMutex;

}

dabc::WorkingThread::~WorkingThread()
{
   // !!!!!!!! Do not forgot stopping thread in destructors of inherited classes too  !!!!!

   DOUT3(("Start thread %s destructor %s", GetName(), DBOOL(IsItself())));

   Stop(true);

   LockGuard guard(fWorkMutex);
   if (fState==stError) {
      EOUT(("Kill thread in error state, nothing better can be done"));
      Kill();
      fState = stJoined;
   }

   delete [] fQueues; fQueues = 0;

   fExec->fProcessorId = 0;
   fExec->fProcessorThread = 0;
   fExec->fProcessorMainMutex = 0;
   delete fExec; fExec = 0;

   fNumQueues = 0;

   DOUT3(("Thread %s %p destroyed", GetName(), this));
}

bool dabc::WorkingThread::CompatibleClass(const char* clname) const
{
   if ((clname==0) || (strlen(clname)==0)) return true;

   return strcmp(clname, typeWorkingThread) == 0;
}


void* dabc::WorkingThread::MainLoop()
{
   EventId evid;
   double tmout;

   DOUT2(("*** Thrd:%s Starting MainLoop", GetName()));

   while (IsThrdWorking()) {

      DOUT5(("*** Thrd:%s Checking timeouts", GetName()));

      tmout = CheckTimeouts();

      DOUT5(("*** Thrd:%s Check timeouts %5.3f", GetName(), tmout));

      evid = WaitEvent(tmout);

      if (evid!=NullEventId)
         ProcessEvent(evid);

      if (fExplicitLoop) RunExplicitLoop();
   }

   DOUT2(("*** Thrd:%s Leaving MainLoop", GetName()));

   return 0;
}

void dabc::WorkingThread::SingleLoop(WorkingProcessor* proc, double tmout_user) throw (StopException)
{

   DOUT5(("*** Thrd:%s SingleLoop user_tmout %5.3f", GetName(), tmout_user));

   if (fExitExplicitLoop)
      if (proc==fExplicitLoop)
         throw StopException();

   double tmout = CheckTimeouts();
   if (tmout_user>=0)
      if ((tmout<0) || (tmout_user<tmout)) tmout = tmout_user;

   EventId evid = WaitEvent(tmout);

   if (evid!=NullEventId) ProcessEvent(evid);
}

void dabc::WorkingThread::RunEventLoop(WorkingProcessor* proc, double tm, bool dooutput)
{
   TimeStamp_t first_tm = ThrdTimeStamp();

   if (tm<0) {
      EOUT(("negative (endless) timeout specified - set default (0 sec)"));
      tm = 0;
   }

   long last_out = -1;

   while (IsThrdWorking()) {
      double dist = TimeDistance(first_tm, ThrdTimeStamp());

      double wait_tm = (dist < tm) ? tm - dist : 0;
      if (dooutput && (wait_tm>0.2)) wait_tm = 0.2;

      if (proc && (proc->IsProcessorDestroyment() || proc->IsProcessorHalted())) break;


      SingleLoop(proc, wait_tm);

      if (dooutput) {
         long curr_out = lrint(tm - dist);
         if (curr_out<0) curr_out = 0;
         if (curr_out!=last_out) {
            fprintf(stdout, "\b\b\b%3ld", curr_out);
            fflush(stdout);
            last_out = curr_out;
         }
      }

      if (dist>=tm) break;
   }

   if (dooutput) {
      fprintf(stdout, "\b\b\b");
      fflush(stdout);
   }
}


bool dabc::WorkingThread::Start(double timeout_sec, bool withoutthrd)
{
   // first, check if we should join thread,
   // when it was stopped before
   bool needjoin = false;
   bool needkill = false;
   {
      LockGuard guard(fWorkMutex);
      fRealThrd = !withoutthrd;
      switch (fState) {
         case stCreated:
            break;
         case stRunning:
            return true;
         case stStopped:
            needjoin = true;
            break;
         case stJoined:
            break;
         case stError:
            EOUT(("Restart from error state, may be dangerous"));
            needkill = true;
            break;
         case stChanging:
            EOUT(("Status is changing from other thread. Not supported"));
            return false;
         default:
            EOUT(("Forgot something???"));
      }

      fState = stChanging;
   }

   DOUT3(("Thread %s starting join:%s kill:%s", GetName(), DBOOL(needjoin), DBOOL(needkill)));

   if (fRealThrd) {
      if (needjoin) Thread::Join(); else
      if (needkill) Thread::Kill();
   }

   // from this moment on thread main loop must became functional

   fThrdWorking = true;

   if (fRealThrd)
      Thread::Start(this);
   else
      Thread::UseCurrentAsSelf();

   bool res = fExec->Execute("ConfirmStart", timeout_sec) == cmd_true;

   LockGuard guard(fWorkMutex);
   fState = res ? stRunning : stError;

   return res;
}

bool dabc::WorkingThread::Stop(bool waitjoin, double timeout_sec)
{
   bool needstop = false;

   {
      LockGuard guard(fWorkMutex);
      switch (fState) {
         case stCreated:
            return true;
         case stRunning:
            needstop = true;
            break;
         case stStopped:
            if (!waitjoin) return true;
            break;
         case stJoined:
            return true;
         case stError:
            EOUT(("Stop from error state, do nothing"));
            return true;
         case stChanging:
            EOUT(("State is changing from other thread. Not supported"));
            return false;
         default:
            EOUT(("Forgot something???"));
      }

      fState = stChanging;
   }

   bool res = true;

   DOUT3(("Start execute ConfirmStop"));

   if (needstop) res = fExec->Execute("ConfirmStop", timeout_sec);

   DOUT3(("Did execute ConfirmStop"));

   if (waitjoin && fRealThrd) {
      if (!res) EOUT(("Cannot wait for join when stop was not succeeded"));
           else Thread::Join();
   }

   LockGuard guard(fWorkMutex);

   if (!res) {
      // not necessary, but to be sure that everything is done to stop thread
      fThrdWorking = false;
      fState = stError;
   } else
      fState = waitjoin ? stJoined : stStopped;

   return res;
}

void dabc::WorkingThread::SetWorkingFlag(bool on)
{
   LockGuard guard(fWorkMutex);
   fThrdWorking = on;
}

bool dabc::WorkingThread::Sync(double timeout_sec)
{
   return fExec->Execute("ConfirmSync", timeout_sec);
}

bool dabc::WorkingThread::SetExplicitLoop(WorkingProcessor* proc)
{
   if (!IsItself()) {
      EOUT(("Call from other thread - absolutely wrong"));
      exit(113);
   }

   if (fExplicitLoop!=0)
      EOUT(("Explicit loop is already set"));
   else
      fExplicitLoop = proc;

   return fExplicitLoop == proc;
}

void dabc::WorkingThread::RunExplicitLoop()
{
   if (fExplicitLoop==0) return;

   DOUT4(("Enter RunExplicitMainLoop"));

   fExitExplicitLoop = false;

   WorkingThread::IntGuard iguard(fExplicitLoop->fProcessorRecursion);

   try {

     fExplicitLoop->DoProcessorMainLoop();

  } catch (dabc::StopException e) {

     DOUT2(("WorkingProcessor %u stopped via exception", fExplicitLoop->fProcessorId));

  } catch (dabc::TimeoutException e) {

     DOUT2(("WorkingProcessor %u stopped via timeout", fExplicitLoop->fProcessorId));

  } catch(dabc::Exception e) {
     EOUT(("Exception %s in processor %u", e.what(), fExplicitLoop->fProcessorId));
  } catch(...) {
     EOUT(("Exception UNCKNOWN in processor %u", fExplicitLoop->fProcessorId));
  }

  fExplicitLoop->DoProcessorAfterMainLoop();

  fExplicitLoop->CheckHaltCmds();

  DOUT5(("Exit from RunExplicitMainLoop"));

  fExplicitLoop = 0;
  fExitExplicitLoop = false;
}


void dabc::WorkingThread::FireDoNothingEvent()
{
   // used by timeout object to activate thread and leave WaitEvent function

   Fire(CodeEvent(evntDoNothing), -1);
}

int dabc::WorkingThread::ExecuteThreadCommand(Command* cmd)
{
   DOUT3(("WorkingThread::Execute command %s", cmd->GetName()));

   int res = cmd_true;

   if (cmd->IsName("ConfirmStart")) {
   } else
   if (cmd->IsName("ConfirmStop")) {
      fThrdWorking = false;
   } else
   if (cmd->IsName("ConfirmSync")) {
   } else
   if (cmd->IsName("AddProcessor")) {
      DOUT4(("AddProcessor in thrd %p", this));

      WorkingProcessor* proc = (WorkingProcessor*) cmd->GetPtr("WorkingProcessor");
      if (proc==0) return cmd_false;

      fNoLongerUsed = false;

      proc->fProcessorThread = this;
      proc->fProcessorMainMutex = &fWorkMutex;

      proc->fProcessorId = fProcessors.size();
      fProcessors.push_back(proc);

      ProcessorNumberChanged();

      proc->OnThreadAssigned();
   } else
   if (cmd->IsName("RemoveProcessor")) {
      uint32_t id = (uint32_t) cmd->GetInt("ProcessorId");

      if (id>0) {

         if (id<fProcessors.size()) {
            if (fProcessors[id]) fProcessors[id]->fProcessorId = 0;

            DOUT3(("Thrd:%s Remove processor %u %p", GetName(), id, fProcessors[id]));

            fProcessors[id] = 0;
            // rebuild processors vector after we process all other events now in the queue
            Fire(CodeEvent(evntRebuildProc, 0, fProcessors.size()), -1);
         } else {
            EOUT(("No processor with id = %u", id));
            res = cmd_false;
         }

         fNoLongerUsed = !CheckThreadUsage();

         ProcessorNumberChanged();

         DOUT3(("Thrd:%s Remove processor %u done", GetName(), id));
      }
   } else
   if (cmd->IsName("DestroyProcessor")) {
      uint32_t id = (uint32_t) cmd->GetInt("ProcessorId");
      WorkingProcessor* proc = fProcessors[id];

      DOUT3(("Destroy processor %u %p", id, proc));

      if (proc==0)
         res = cmd_false;
      else {
         if (!proc->fProcessorDestroyment) {
            EOUT(("Destroyment flag not specified why !!!!!"));
         }

         if (proc->fProcessorRecursion>0) {
            EOUT(("Processor %p Recursion = %d", proc, proc->fProcessorRecursion));
            res = cmd_postponed;
            fExec->AddPostponed(cmd);
         } else {
            fProcessors[id] = 0;
            delete proc;
         }
      }

      fNoLongerUsed = !CheckThreadUsage();

   } else

   if (cmd->IsName("ExitMainLoop")) {
      uint32_t id = (uint32_t) cmd->GetInt("ProcessorId");
      WorkingProcessor* proc = fProcessors[id];

      DOUT4(("ExitMainLoop for processor %u %p", id, proc));

      // do extra job only when explicit loop is running
      if (fExplicitLoop!=0) {
         if (proc==0) {
            EOUT(("Intern error, RemoveMainLoop for not ModuleSync"));
            res = cmd_false;
         } else
         if (proc!=fExplicitLoop) {
            EOUT(("Explicit loop running by other processor %u, ignore request from processor %u", fExplicitLoop->fProcessorId, id));
            res = cmd_false;
         } else
            fExitExplicitLoop = true;
      }
   } else

      res = cmd_false;

   return res;
}

bool dabc::WorkingThread::CheckThreadUsage()
{
   // check if we have any processors, exclude exec processor with id=1

   for (unsigned id = 2; id<fProcessors.size(); id++)
      if (fProcessors[id]) return true;

   return false;
}

void dabc::WorkingThread::_Fire(EventId arg, int nq)
{
//   DOUT1(("Fire event code:%u item:%u arg:%u", GetEventCode(arg), GetEventItem(arg), GetEventArg(arg)));

   _PushEvent(arg, nq);
   fWorkCond._DoFire();

#ifdef DO_INDEX_CHECK

   long sum = 0;
   for (int n=0;n<fNumQueues;n++) sum+=fQueues[n].Size();
   if (sum!=fWorkCond._FiredCounter()) {
      dabc::SetDebugLevel(5);
      DOUT5(("Thrd %s Error sum1 %ld cond %ld  event code:%u item:%u arg:%u",
            GetName(), sum, fWorkCond._FiredCounter(),
            GetEventCode(arg), GetEventItem(arg), GetEventArg(arg)));
   }
#endif

}

dabc::EventId dabc::WorkingThread::WaitEvent(double tmout)
{
   LockGuard lock(fWorkMutex);

   DOUT5(("*** Thrd:%s Wait Event %5.1f cond_cnt %ld q0:%u q1:%u q2:%u",
         GetName(), tmout, fWorkCond._FiredCounter(),
         fQueues[0].Size(), fQueues[1].Size(), fQueues[2].Size()));

   if (fWorkCond._DoWait(tmout)) {
      DOUT5(("*** Thrd:%s Wait done", GetName()));
      return _GetNextEvent();
   }

   DOUT5(("*** Thrd:%s Wait timedout", GetName()));

   return NullEventId;
}


void dabc::WorkingThread::ProcessEvent(EventId evnt)
{
   uint16_t itemid = GetEventItem(evnt);

   DOUT5(("*** Thrd:%s Item:%u Event:%u arg:%u  q0:%u q1:%u q2:%u",
         GetName(), itemid, GetEventCode(evnt), GetEventArg(evnt),
         fQueues[0].Size(), fQueues[1].Size(), fQueues[2].Size()));

   if (itemid>0) {
      WorkingProcessor* proc = fProcessors[itemid];

      DOUT5(("*** Thrd:%s proc:%p destr %s halted %s",
            GetName(), proc,
            DBOOL((proc ? proc->fProcessorDestroyment : false)),
            DBOOL((proc ? proc->fProcessorHalted : false)) ));

      if (proc && !proc->fProcessorDestroyment && !proc->fProcessorHalted) {
         WorkingThread::IntGuard iguard(proc->fProcessorRecursion);

         if (GetEventCode(evnt) < WorkingProcessor::evntFirstSystem)
            proc->ProcessCoreEvent(evnt);
         else
            proc->ProcessEvent(evnt);

         proc->CheckHaltCmds();
      }
   } else

   switch (GetEventCode(evnt)) {
      case evntCheckTmout: {
         uint32_t tmid = GetEventArg(evnt);

         if (tmid >= fProcessors.size()) {
            DOUT3(("evntCheckTmout - mismatch in processor id:%u sz:%u ", tmid, fProcessors.size()));
            break;
         }

         WorkingProcessor* src = fProcessors[tmid];
         if (src==0) {
            DOUT3(("WorkingProcessor %u no longer exists", tmid));
            break;
         }

         TimeStamp_t mark(NullTimeStamp);
         double interv(0);

         if (src->TakeActivateData(mark, interv)) {
            if (interv<0) {
               src->fProcessorNextFire = NullTimeStamp;
               src->fProcessorPrevFire = NullTimeStamp;
            } else {
               // if one activate timeout with positive interval, emulate
               // that one already has previous call to ProcessTimeout
               if (IsNullTime(src->fProcessorPrevFire) && (interv>0))
                  src->fProcessorPrevFire = mark;

               mark = TimeShift(mark, interv);

               // set activation time only in the case if no other active timeout was used
               if (IsNullTime(src->fProcessorNextFire) ||
                     TimeDistance(mark, src->fProcessorNextFire) > 0.) {
                  src->fProcessorNextFire = mark;
                  CheckTimeouts(true);
               }
            }

         }

         break;
      }

      case evntRebuildProc: {
         unsigned old_size = GetEventArg(evnt);
         if (old_size==fProcessors.size()) {
            unsigned new_size = fProcessors.size();

            while ((new_size>1) && (fProcessors[new_size-1]==0)) new_size--;

            fProcessors.resize(new_size);

            DOUT3(("Thrd:%s Shrink processors size to %u", GetName(), new_size));

         } else
            DOUT3(("Processors sizes changed during evntRebuildProc event"));
         break;
      }

      case evntDoNothing:
         break;

      default:
         EOUT(("Unknown event %u arg %u", GetEventCode(evnt), GetEventArg(evnt)));
         break;
   }

   DOUT5(("Thrd:%s Item:%u Event:%u arg:%u done", GetName(), itemid, GetEventCode(evnt), GetEventArg(evnt)));
}

bool dabc::WorkingThread::AddProcessor(WorkingProcessor* proc, bool sync)
{
   Command* cmd = new Command("AddProcessor");
   cmd->SetPtr("WorkingProcessor", proc);
   return sync ? fExec->Execute(cmd) : fExec->Submit(cmd);
}


int dabc::WorkingThread::Execute(dabc::Command* cmd, double tmout)
{
   return fExec->Execute(cmd, -1., WorkingProcessor::priorityMaximum);
}


int dabc::WorkingThread::SysCommand(const char* cmdname, WorkingProcessor* proc, int priority)
{
   Command* cmd = new Command(cmdname);
   cmd->SetInt("ProcessorId", proc->fProcessorId);
   return fExec->Execute(cmd, -1., priority);
}

void dabc::WorkingThread::RemoveProcessor(WorkingProcessor* proc)
{
   // no need to remove processor, which is al
   SysCommand("RemoveProcessor", proc);
}

void dabc::WorkingThread::ExitMainLoop(WorkingProcessor* proc)
{
   if (IsItself()) {
      if (fExplicitLoop == proc) {
         DOUT0(("Cannot leave main loop like this - did something stupid"));
         fExitExplicitLoop = true;
      }

      return;
   }

   SysCommand("ExitMainLoop", proc);
   fExec->Execute("ConfirmSync");
}

void dabc::WorkingThread::DestroyProcessor(WorkingProcessor* proc)
{
   Command* cmd = new Command("DestroyProcessor");
   cmd->SetInt("ProcessorId", proc->fProcessorId);
   fExec->Submit(cmd, fNumQueues-1); // submit with minimum priority - allow to complete all other actions
}

double dabc::WorkingThread::CheckTimeouts(bool forcerecheck)
{
   if (fProcessingTimeouts>0) return -1.;

   IntGuard guard(fProcessingTimeouts);

   TimeStamp_t now;

   if (!forcerecheck) {
      if (IsNullTime(fNextTimeout)) return -1.;
      now = ThrdTimeStamp();
      double dist = TimeDistance(now, fNextTimeout);
      if (dist>0.) return dist;
   } else
      now = ThrdTimeStamp();

   double min_tmout = -1.;

   for (unsigned n=1;n<fProcessors.size();n++) {
      WorkingProcessor* tmout = fProcessors.at(n);
      if (tmout==0) continue;

      if (IsNullTime(tmout->fProcessorNextFire)) continue;

      double dist = TimeDistance(now, tmout->fProcessorNextFire);

      if (dist<0.) {
         double last_diff = 0;
         if (!IsNullTime(tmout->fProcessorPrevFire))
            last_diff = TimeDistance(tmout->fProcessorPrevFire, now);

         DOUT5(("*** Thrd:%s Process timeout of processor %p", GetName(), tmout));

         dist = tmout->ProcessTimeout(last_diff);

         DOUT5(("*** Thrd:%s Process timeout of processor %p done", GetName(), tmout));

         if (dist>=0.) {
            tmout->fProcessorPrevFire = now;
            tmout->fProcessorNextFire = TimeShift(now, dist);
         } else {
            tmout->fProcessorPrevFire = NullTimeStamp;
            tmout->fProcessorNextFire = NullTimeStamp;
         }
      }

      if (dist>=0.)
         if ((min_tmout<0.) || (dist<min_tmout))  min_tmout = dist;
   }

   if (min_tmout>=0.)
      fNextTimeout = TimeShift(now, min_tmout);
   else
      fNextTimeout = NullTimeStamp;

   return min_tmout;
}
