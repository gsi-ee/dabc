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
#include "dabc/CommandsSet.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/WorkingThread.h"

dabc::CommandsSet::CommandsSet(Command* cmd, bool parallel_exe) :
   CommandClientBase(),
   WorkingProcessor(),
   fReceiver(dabc::mgr()),
   fMain(cmd),
   fParallelExe(parallel_exe),
   fCmdsSet(false, false),
   fMainRes(true),
   fCompleted(false),
   fNeedDestroy(true)
{
   DOUT5(("%p Command set for cmd %s", this, DNAME(fMain)));
}

dabc::CommandsSet::~CommandsSet()
{
   DOUT5(("%p Command set destroyed", this));
}

void dabc::CommandsSet::SetReceiver(CommandReceiver* rcv)
{
   LockGuard lock(fCmdsMutex);

   fReceiver = rcv;
}

void dabc::CommandsSet::Add(Command* cmd)
{
   LockGuard lock(fCmdsMutex);

   if (fCompleted) {
      EOUT(("Set marked as completed, do not add more commands to it !!!!"));
   }

   fCmdsSet.Push(cmd);
}

bool dabc::CommandsSet::_ProcessReply(Command* cmd)
{
   if (!cmd->GetResult()) {
      fMainRes = false;
      std::string v;
      cmd->SaveToString(v);
      EOUT(("Main Cmd %s fails because of subcmd: %s", (fMain ? fMain->GetName() : "---"), v.c_str()));
   }

   DOUT5(("CommandsSet::_ProcessReply this:%p main:%p %s cmd:%s  numsubm = %d numwait = %d res = %s",
      this, fMain, (fMain ? fMain->GetName() : "---"), cmd->GetName(),
      _NumSubmCmds(), fCmdsSet.Size(), DBOOL(fMainRes)));

   // call user-defined method, which can redirect or even delete command object
   // false means that client will finalize command itself
   bool keep_cmd = ProcessCommandReply(cmd);

   // if set is not completed, do not react on the replies
   if (!fCompleted) return keep_cmd;

   if (((_NumSubmCmds()==0) && (fCmdsSet.Size()==0)) || !fMainRes) {

      DOUT3(("CommandsSet::MainReply %s res %s", (fMain ? fMain->GetName() : "---"), DBOOL(fMainRes)));

      dabc::Command::Reply(fMain, fMainRes);
      fMain = 0;

      if (fNeedDestroy) {
         fNeedDestroy = false;
         dabc::mgr()->DeleteAnyObject(this);
      }
   } else
   if ((_NumSubmCmds()==0) || fParallelExe)
      _SubmitNextCommands();

   return keep_cmd;
}

void dabc::CommandsSet::Reset(bool mainres)
{
   fCmdsSet.Cleanup();

   CancelCommands();

   // here we may avoid locking while no commands is remaining in client list
   // but there is probability for timeout, therefore anyhow lock

   dabc::Command* cmd = 0;

   {
      LockGuard lock(fCmdsMutex);
      cmd = fMain;
      mainres = fMainRes && mainres;
      fMainRes = true;
      fMain = 0;
      fNeedDestroy = false;
   }

   dabc::Command::Reply(cmd, mainres);

   // command must be destroyed by the user
}

bool dabc::CommandsSet::_SubmitNextCommands()
{
   dabc::Command* cmd = 0;

   bool isany = false;

   do {

      cmd = 0;

      if (fCmdsSet.Size() > 0)
         cmd = fCmdsSet.Pop();

      if (_Assign(cmd)) {

         DOUT5(("CommandsSet::Submit command %s cnt:%d", cmd->GetName(), _NumSubmCmds()));

         fReceiver->Submit(cmd);

         isany = true;

         if (!fParallelExe) return true;
      }

   } while (cmd!=0);

   return isany;
}

void dabc::CommandsSet::Completed(dabc::CommandsSet* set, double timeout_sec)
{
   if (set==0) return;

   bool is_done = true;

   // first, mark set as completed - no more commands is recommended to add
   {
      LockGuard lock(set->fCmdsMutex);

      if (set->fCompleted) {
         EOUT(("Set is already marked as completed - do not repeat again"));
         return;
      }

      set->fCompleted = true;

      set->_SubmitNextCommands();

      // if set await submitted commands, keep it
      if (set->_NumSubmCmds()>0)
         is_done = false;
      else
         set->fNeedDestroy = false;
   }

   if (is_done) {
      // no need locking here - nobody can access fMain at this point
      DOUT3(("Main command %s completed with res %s", set->fMain->GetName(), DBOOL(set->fMainRes)));

      dabc::Command::Reply(set->fMain, set->fMainRes);
      set->fMain = 0;

      delete set;
   } else {
      if (timeout_sec>0) {
         set->AssignProcessorToThread(dabc::mgr()->ProcessorThread());
         set->ActivateTimeout(timeout_sec);
      }
   }
}

double dabc::CommandsSet::ProcessTimeout(double)
{
   EOUT(("%p Command %s timedout not-replyed %d", this, DNAME(fMain), _NumSubmCmds()));

   bool dodestroy = false;
   dabc::Command* cmd = 0;

   {
      LockGuard lock(fCmdsMutex);
      cmd = fMain;
      fMain = 0;
      dodestroy = fNeedDestroy;
      fNeedDestroy = false;
   }

   dabc::Command::Reply(cmd, false);
   if (dodestroy) DestroyProcessor();

   return -1;
}

// ============================================================================================


dabc::NewCommandsSet::NewCommandsSet() :
   WorkingProcessor(),
   fReceiver(dabc::mgr()),
   fCmds(10, true),
   fParallelExe(true),
   fConfirm(0),
   fSyncMode(false)
{
}

dabc::NewCommandsSet::~NewCommandsSet()
{
   CleanupCmds();
}

void dabc::NewCommandsSet::SetReceiver(WorkingProcessor* proc)
{
   fReceiver = proc;
}

void dabc::NewCommandsSet::CleanupCmds()
{
   for (unsigned n=0; n<fCmds.Size();n++) {
      if (fCmds.Item(n).CanFree())
         dabc::Command::Finalise(fCmds.Item(n).cmd);
      else
         dabc::Command::Cancel(fCmds.Item(n).cmd);
   }
}


void dabc::NewCommandsSet::Add(Command* cmd, WorkingProcessor* recv)
{
   CmdRec rec;

   rec.cmd = cmd;
   rec.recv = recv ? recv : fReceiver;
   rec.state = 0;

   fCmds.Push(rec);
}

unsigned dabc::NewCommandsSet::GetNumber() const
{
   return fCmds.Size();
}

dabc::Command* dabc::NewCommandsSet::GetCommand(unsigned n)
{
   if (n>=fCmds.Size()) return 0;

   if (fCmds.Item(n).state != 2) return 0;

   return fCmds.Item(n).cmd;
}

int dabc::NewCommandsSet::GetCmdResult(unsigned n)
{
   dabc::Command* cmd = GetCommand(n);
   return cmd ? cmd->GetResult() : cmd_timedout;
}

int dabc::NewCommandsSet::ExecuteCommand(Command* cmd)
{
   if (fConfirm!=0) {
      EOUT(("Something wrong - confirmation command already exists"));
      return WorkingProcessor::ExecuteCommand(cmd);
   }

   DOUT0(("CommandsSet get command %s", cmd->GetName()));

   // first submit commands which should be submitted
   while (SubmitNextCommand())
     if (!fParallelExe) break;

   fConfirm = cmd;

   int res = CheckExecutionResult();

   DOUT0(("Set execution res = %d", res));

   return res;
}

bool dabc::NewCommandsSet::NewCmd_ReplyCommand(Command* cmd)
{
   for (unsigned n=0; n<fCmds.Size();n++) {
      if (fCmds.Item(n).cmd == cmd) {
         if (fCmds.Item(n).state != 1)
            EOUT(("Wrong state %d for command %s at reply", fCmds.Item(n).state, cmd->GetName()));
         fCmds.ItemPtr(n)->state = 2;
         break;
      }
   }

   SubmitNextCommand();

   int res = CheckExecutionResult();

   if (res!=cmd_postponed) {
      DOUT0(("SET COMPLETED res = %d !!!", res));

      if (fConfirm!=0) {
         dabc::Command::Reply(fConfirm, res);
         fConfirm = 0;
      }

      if (!fSyncMode) DestroyProcessor();
   }

   // keep commands stored to have access to them later
   return false;
}

bool dabc::NewCommandsSet::SubmitNextCommand()
{
   for (unsigned n=0; n<fCmds.Size();n++)
      if (fCmds.Item(n).state==0)
         if (fCmds.Item(n).recv) {

            DOUT0(("CommandsSet distributes cmd %s  item %u size %u", fCmds.Item(n).cmd->GetName(), n, fCmds.Size()));

            fCmds.ItemPtr(n)->state = 1;

            NewCmd_Assign(fCmds.Item(n).cmd);

            fCmds.Item(n).recv->Submit(fCmds.Item(n).cmd);

            return true;

         } else {
            EOUT(("Not able submit next command %s", fCmds.Item(n).cmd->GetName()));
            fCmds.ItemPtr(n)->state = 2;
         }

   return false;
}

int dabc::NewCommandsSet::CheckExecutionResult()
{
   int res = cmd_true;

   for (unsigned n=0; n<fCmds.Size();n++) {
      if (fCmds.Item(n).state!=2)
         return cmd_postponed;

      if (fCmds.Item(n).cmd->GetResult()==cmd_false)
         res = cmd_false;
   }

   return res;
}

int dabc::NewCommandsSet::ExecuteSet(double tmout)
{
   WorkingThread curr(0, "Current");

   if (ProcessorThread()==0) {
      if (dabc::mgr()==0) {
         EOUT(("Cannot use CommandsSet without running manager !!!"));
         return cmd_false;
      }

      WorkingThread* thrd = dabc::mgr()->CurrentThread();
      if (thrd==0) {
         DOUT0(("Cannot use commands set outside DABC threads, create dummy !!!"));
         curr.Start(0, true);
         thrd = &curr;
      }

      DOUT0(("Assign SET to thread %s", thrd->GetName()));

      AssignProcessorToThread(thrd);
   }

   DOUT0(("Calling AnyCommand"));

   fSyncMode = true;

   int res = NewCmd_ExecuteIn(this, "AnyCommand", tmout);

   DOUT0(("Calling AnyCommand res = %d", res));

   RemoveProcessorFromThread(true);

   return res;
}

bool dabc::NewCommandsSet::SubmitSet(Command* rescmd, double tmout)
{
   if (ProcessorThread()==0) {
      if (dabc::mgr()==0) {
         EOUT(("Cannot use CommandsSet without running manager !!!"));
         return false;
      }

      WorkingThread* thrd = dabc::mgr()->CurrentThread();
      if (thrd==0) thrd = dabc::mgr()->ProcessorThread();
      if (thrd==0) {
         EOUT(("Cannot use commands set outside DABC threads !!!"));
         return false;
      }

      DOUT0(("Assign SET to thread %s", thrd->GetName()));

      AssignProcessorToThread(thrd);
   }

   if (rescmd==0) rescmd = new dabc::Command("AnyCommand");

   fSyncMode = false;

   if (tmout>0) ActivateTimeout(tmout);

   return NewCmd_Submit(rescmd);
}

double dabc::NewCommandsSet::ProcessTimeout(double last_diff)
{
   EOUT(("CommandsSet Timeout !!!!!!!!!!!!!"));

   if (fConfirm!=0) {
      dabc::Command::Reply(fConfirm, cmd_timedout);
      fConfirm = 0;
   }

   if (!fSyncMode) DestroyProcessor();

   return -1;
}

