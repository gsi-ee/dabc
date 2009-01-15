#include "dabc/WorkingThread.h"

#include "dabc/WorkingProcessor.h"

#include "dabc/Command.h"

#ifdef DO_INDEX_CHECK
unsigned dabc::WorkingThread::maxlimit = 1000;
#endif

dabc::WorkingThread::WorkingThread(Basic* parent, const char* name, unsigned numqueus) :
   Basic(parent, name),
   Thread(),
   Runnable(),
   CommandReceiver(),
   fState(stCreated),
   fThrdWorking(false),
   fNoLongerUsed(false),
   fRealThrd(true),
   fWorkMutex(),
   fWorkCond(&fWorkMutex),
   fQueues(0),
   fNumQueues(numqueus),
   fSysCommands(false, false),
   fTime(),
   fNextTimeout(NullTimeStamp),
   fProcessors(),
   fExplicitLoop(0),
   fExitExplicitLoop(false)
{
   if (fNumQueues>0) {
     fQueues = new EventsQueue[fNumQueues];
     for (int n=0;n<fNumQueues;n++)
        fQueues[n].Init(256, true);
   }

   DOUT3(("Thread %s created", GetName()));

   fProcessors.push_back(0); // exclude id==0
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

      tmout = CheckTimeouts();

      DOUT5(("*** Thrd:%s Wait Event %5.1f", GetName(), tmout));

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
   if (fExitExplicitLoop)
      if (proc==fExplicitLoop)
         throw StopException();

   double tmout = CheckTimeouts();
   if (tmout_user>=0)
      if ((tmout<0) || (tmout_user<tmout)) tmout = tmout_user;

   EventId evid = WaitEvent(tmout);

   if (evid!=NullEventId) ProcessEvent(evid);
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
            EOUT(("Status is chaning from other thread. Not supported"));
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

   bool res = Execute("ConfirmStart", timeout_sec);

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

   if (needstop) res = Execute("ConfirmStop", timeout_sec);

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
   return Execute("ConfirmSync", timeout_sec);
}

bool dabc::WorkingThread::SetExplicitLoop(WorkingProcessor* proc)
{
   if (!IsItself()) {
      EOUT(("Call from other thread - absolutely wrong"));
      exit(1);
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

  DOUT5(("Exit from RunExplicitMainLoop"));

  fExplicitLoop = 0;
  fExitExplicitLoop = false;
}


bool dabc::WorkingThread::Submit(Command* cmd)
{
   {
      LockGuard guard(fWorkMutex);

      if (IsThrdWorking()) {
        fSysCommands.Push(cmd);
        _Fire(evntSysCmd, 0);
         return true;
      }
   }

   return CommandReceiver::Submit(cmd);
}

void dabc::WorkingThread::FireDoNothingEvent()
{
   // used by timeout object to activate thread and leave WaitEvent function

   Fire(CodeEvent(evntDoNothing), -1);
}

int dabc::WorkingThread::ExecuteCommand(Command* cmd)
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

      proc->fProcessorId = fProcessors.size();
      fProcessors.push_back(proc);
      proc->fProcessorThread = this;

      ProcessorNumberChanged();

      proc->OnThreadAssigned();
   } else
   if (cmd->IsName("RemoveProcessor")) {
      uint32_t id = (uint32_t) cmd->GetInt("ProcessorId");

      if (id>0) {

         DOUT5(("Thrd:%s Remove processor %u", GetName(), id));

         if (id<fProcessors.size()) {
            if (fProcessors[id]) fProcessors[id]->fProcessorId = 0;

            fProcessors[id] = 0;
            // rebuild processors vector after we process all other events now in the queue
            Fire(CodeEvent(evntRebuildProc, 0, fProcessors.size()), -1);
         } else {
            EOUT(("No processor with id = %u", id));
            res = cmd_false;
         }

         fNoLongerUsed = !CheckThreadUsage();

         ProcessorNumberChanged();

         DOUT5(("Thrd:%s Remove processor %u done", GetName(), id));
      }
   } else
   if (cmd->IsName("DestroyProcessor")) {
      uint32_t id = (uint32_t) cmd->GetInt("ProcessorId");
      WorkingProcessor* proc = fProcessors[id];
      fProcessors[id] = 0;

      DOUT3(("Destroy processor %u %p", id, proc));
      if (proc) delete proc;
           else res = cmd_false;
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
   // check if we have any processors

   for (unsigned id = 1; id<fProcessors.size(); id++)
      if (fProcessors[id]) return true;

   return false;
}

void dabc::WorkingThread::_Fire(EventId arg, int nq)
{
//   DOUT1(("Fire event code:%u item:%u arg:%u", GetEventCode(arg), GetEventItem(arg), GetEventArg(arg)));

   _PushEvent(arg, nq);
   fWorkCond._DoFire();
}

dabc::EventId dabc::WorkingThread::WaitEvent(double tmout)
{
   LockGuard lock(fWorkMutex);
   if (fWorkCond._DoWait(tmout)) return _GetNextEvent();

   return NullEventId;
}


void dabc::WorkingThread::ProcessEvent(EventId evnt)
{
   uint16_t itemid = GetEventItem(evnt);

   DOUT5(("*** Thrd:%s Item:%u Event:%u arg:%u", GetName(), itemid, GetEventCode(evnt), GetEventArg(evnt)));

   if (itemid>0) {
      WorkingProcessor* proc = fProcessors[itemid];
      if (proc) proc->ProcessEvent(evnt);
   } else
    switch (GetEventCode(evnt)) {
       case evntSysCmd: {

           dabc::Command* cmd = 0;
           {
              LockGuard guard(fWorkMutex);
              cmd = fSysCommands.Pop();
           }

           ProcessCommand(cmd);
           break;
       }

       case evntProcCmd: {
          uint32_t procid = GetEventArg(evnt);

          if (procid >= fProcessors.size()) {
             DOUT3(("evntProcCmd - missmatch in processor id:%u sz:%u ",procid, fProcessors.size()));
             break;
          }

          WorkingProcessor* proc = fProcessors[procid];

          if (proc) {
             Command* cmd = proc->fProcessorCommands.Pop();
             if (cmd) proc->ProcessCommand(cmd);
          } else {
             DOUT3(("WorkingProcessor with such id no longer exists"));
          }

          break;
       }

       case evntCheckTmout: {
          uint32_t tmid = GetEventArg(evnt);

          if (tmid >= fProcessors.size()) {
             DOUT3(("evntCheckTmout - missmatch in processor id:%u sz:%u ", tmid, fProcessors.size()));
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
                // if one activate timeout with positive interval, immulate
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
          EOUT(("Uncknown event %u arg %u", GetEventCode(evnt), GetEventArg(evnt)));
          break;
    }

   DOUT5(("Thrd:%s Item:%u Event:%u arg:%u done", GetName(), itemid, GetEventCode(evnt), GetEventArg(evnt)));

}

bool dabc::WorkingThread::AddProcessor(WorkingProcessor* proc, bool sync)
{
   Command* cmd = new Command("AddProcessor");
   cmd->SetPtr("WorkingProcessor", proc);
   return sync ? Execute(cmd) : Submit(cmd);
}

bool dabc::WorkingThread::SubmitProcessorCmd(WorkingProcessor* proc, Command* cmd)
{
   if (!IsThrdWorking()) {
      EOUT(("Fails for command %s", cmd->GetName()));
      dabc::Command::Reply(cmd, false);
      return false;
   }

   DOUT5(("Get processor command %s numq %d arg %d", cmd->GetName(), fNumQueues,  (fNumQueues>1) ? 1 : 0));

   proc->fProcessorCommands.Push(cmd);

   LockGuard guard(fWorkMutex);

   _Fire(CodeEvent(evntProcCmd, 0, proc->fProcessorId), 0);

   return true;
}

void dabc::WorkingThread::SysCommand(const char* cmdname, WorkingProcessor* proc)
{
   Command* cmd = new Command(cmdname);
   cmd->SetInt("ProcessorId", proc->fProcessorId);
   Execute(cmd);
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
         EOUT(("Cannot leave main loop like this - did something stuiped"));
         fExitExplicitLoop = true;
      }

      return;
   }

   SysCommand("ExitMainLoop", proc);
   Execute("ConfirmSync");
}

void dabc::WorkingThread::DestroyProcessor(WorkingProcessor* proc)
{
   Command* cmd = new Command("DestroyProcessor");
   cmd->SetInt("ProcessorId", proc->fProcessorId);
   Submit(cmd);
}

double dabc::WorkingThread::CheckTimeouts(bool forcerecheck)
{
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

         dist = tmout->ProcessTimeout(last_diff);

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

