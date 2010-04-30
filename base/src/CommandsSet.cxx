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

dabc::CommandsSet::CommandsSet(WorkingThread* thrd, bool parallel) :
   WorkingProcessor(),
   fReceiver(dabc::mgr()),
   fCmds(10, true),
   fParallelExe(parallel),
   fConfirm(0),
   fSyncMode(false),
   fCompleted(false)
{
   if (thrd) AssignProcessorToThread(thrd, true);
}

dabc::CommandsSet::~CommandsSet()
{
   CleanupCmds();
}

void dabc::CommandsSet::Cleanup()
{
   CleanupCmds();
   fCompleted = false;
}

void dabc::CommandsSet::SetReceiver(WorkingProcessor* proc)
{
   fReceiver = proc;
}

void dabc::CommandsSet::CleanupCmds()
{
   for (unsigned n=0; n<fCmds.Size();n++) {
      if (fCmds.Item(n).CanFree())
         dabc::Command::Finalise(fCmds.Item(n).cmd);
      else
         dabc::Command::Cancel(fCmds.Item(n).cmd);
   }
}


void dabc::CommandsSet::Add(Command* cmd, WorkingProcessor* recv, bool do_submit)
{
   if (cmd==0) return;

   if ((ProcessorThread()==0) && !do_submit) {
      EOUT(("Command %s cannot be submitted by the user when set not assigned to the thread!", cmd->GetName()));
   }

   CmdRec rec;

   rec.cmd = cmd;
   rec.recv = recv ? recv : fReceiver;
   rec.state = do_submit ? 0 : 1;

   fCmds.Push(rec);
}

unsigned dabc::CommandsSet::GetNumber() const
{
   return fCmds.Size();
}

dabc::Command* dabc::CommandsSet::GetCommand(unsigned n)
{
   if (n>=fCmds.Size()) return 0;

   if (fCmds.Item(n).state != 2) return 0;

   return fCmds.Item(n).cmd;
}

int dabc::CommandsSet::GetCmdResult(unsigned n)
{
   dabc::Command* cmd = GetCommand(n);
   return cmd ? cmd->GetResult() : cmd_timedout;
}

int dabc::CommandsSet::ExecuteCommand(Command* cmd)
{
   if (fConfirm!=0) {
      EOUT(("Something wrong - confirmation command already exists"));
      return WorkingProcessor::ExecuteCommand(cmd);
   }

   DOUT0(("CommandsSet get command %s", cmd->GetName()));

   // first submit commands which should be submitted
   while (SubmitNextCommand())
     if (!fParallelExe) break;

   int res = CheckExecutionResult();

   if (res==cmd_postponed)
      fConfirm = cmd;
   else {
      if (!fCompleted) SetCompleted(res);
      fCompleted = true;
   }

   DOUT0(("Set execution res = %d", res));

   return res;
}

bool dabc::CommandsSet::ReplyCommand(Command* cmd)
{
   for (unsigned n=0; n<fCmds.Size();n++) {
      if (fCmds.Item(n).cmd == cmd) {
         if (fCmds.Item(n).state != 1)
            EOUT(("Wrong state %d for command %s at reply", fCmds.Item(n).state, cmd->GetName()));
         fCmds.ItemPtr(n)->state = 2;
         break;
      }
   }

   SetCommandCompleted(cmd);

   SubmitNextCommand();

   int res = CheckExecutionResult();

   if (res!=cmd_postponed) {
      DOUT0(("SET COMPLETED res = %d !!!", res));

      if (!fCompleted) SetCompleted(res);
      fCompleted = true;

      if (fConfirm!=0) {
         dabc::Command::Reply(fConfirm, res);
         fConfirm = 0;
      }

      if (!fSyncMode) DestroyProcessor();
   }

   // keep commands stored to have access to them later
   return false;
}

bool dabc::CommandsSet::SubmitNextCommand()
{
   for (unsigned n=0; n<fCmds.Size();n++)
      if (fCmds.Item(n).state==0)
         if (fCmds.Item(n).recv) {

            DOUT0(("CommandsSet distributes cmd %s  item %u size %u", fCmds.Item(n).cmd->GetName(), n, fCmds.Size()));

            fCmds.ItemPtr(n)->state = 1;

            Assign(fCmds.Item(n).cmd);

            fCmds.Item(n).recv->Submit(fCmds.Item(n).cmd);

            return true;

         } else {
            EOUT(("Not able submit next command %s", fCmds.Item(n).cmd->GetName()));
            fCmds.ItemPtr(n)->state = 2;
         }

   return false;
}

int dabc::CommandsSet::CheckExecutionResult()
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

int dabc::CommandsSet::ExecuteSet(double tmout)
{
   WorkingThread curr(0, "Current");

   bool didassign = false;

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

      didassign = true;
   }

   DOUT0(("Calling AnyCommand"));

   fSyncMode = true;

   int res = ExecuteIn(this, "AnyCommand", tmout);

   DOUT0(("Calling AnyCommand res = %d", res));

   if (didassign)
      RemoveProcessorFromThread(true);

   return res;
}

bool dabc::CommandsSet::SubmitSet(Command* rescmd, double tmout)
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

   return Submit(rescmd);
}

double dabc::CommandsSet::ProcessTimeout(double last_diff)
{
   EOUT(("CommandsSet Timeout !!!!!!!!!!!!!"));

   if (!fCompleted) SetCompleted(cmd_timedout);
   fCompleted = true;

   if (fConfirm!=0) {
      dabc::Command::Reply(fConfirm, cmd_timedout);
      fConfirm = 0;
   }

   if (!fSyncMode) DestroyProcessor();

   return -1;
}

