#include "dabc/CommandClient.h"

#include <list>

#include "dabc/Command.h"
#include "dabc/threads.h"
#include "dabc/logging.h"

class dabc::CommandClientBase::stl_commands_list : public std::list<dabc::Command*> {};


dabc::CommandClientBase::CommandClientBase() :
   fCmdsMutex(new Mutex(true)),
   fSubmCmds(new stl_commands_list)
{
}

dabc::CommandClientBase::~CommandClientBase()
{
   CancelCommands();

   delete fSubmCmds; fSubmCmds = 0;

   delete fCmdsMutex; fCmdsMutex = 0;
}

dabc::Command* dabc::CommandClientBase::Assign(Command* cmd)
{
   LockGuard lock(fCmdsMutex);

   return _Assign(cmd);
}

dabc::Command* dabc::CommandClientBase::_Assign(Command* cmd)
{
   if ((cmd==0) || (cmd->fClient == this)) return 0;

   if (cmd->fClient!=0) {
      EOUT(("Commnd is assigned to other client - fatal error"));
      return 0;
   }

   cmd->fClient = this;
   cmd->fClientMutex = fCmdsMutex;
   fSubmCmds->push_back(cmd);

   return cmd;
}

void dabc::CommandClientBase::_Forget(Command* cmd)
{
   // call is done from command destructor, just forget it

   if (cmd==0) {
      EOUT(("~~~~~~~~~~~~~~~~ Problem ~~~~~~~~~~~"));
      exit(1);
   }

   DOUT5(("_Forget cmd %s size now:%p", cmd->GetName(), fSubmCmds));

   fSubmCmds->remove(cmd);
   cmd->fClient = 0;
   cmd->fClientMutex = 0;
}

bool dabc::CommandClientBase::_CommandReplyed(Command* cmd, bool res)
{
   // this method is called when command is replyed
   // if returned true means we get control over command object
   // if returned false, object will be finalised


   // first, we "forget" command - client has no reply job with it
   _Forget(cmd);

   return _ProcessReply(cmd);
}


int dabc::CommandClientBase::CancelCommands()
{
   LockGuard lock(fCmdsMutex);

   DOUT5(("CleanupReplyCmdsList list = %u", (fSubmCmds ? fSubmCmds->size() : 11111111)));

   if (fSubmCmds==0) {
      EOUT(("!!!!!!!!!!!! CANNOT BE !!!!!!!!!!!!"));
      return 0;
   }

   int sz = (int) fSubmCmds->size();

   while (fSubmCmds->size()>0) {
      Command* cmd = fSubmCmds->front();
      String v;
      cmd->SaveToString(v);
      EOUT(("Need to forget command %s",  v.c_str()));
      cmd->SetCanceled();
      _Forget(cmd);
   }

   return sz;
}

unsigned dabc::CommandClientBase::_NumSubmCmds()
{
   return fSubmCmds->size();
}

// ______________________________________________________________________

int dabc::CommandReceiver::ProcessCommand(dabc::Command* cmd)
{
   /// this method perfrom command processing
   // return true if command processed and result is true

   if (cmd==0) return cmd_false;

   DOUT5(("ProcessCommand command %p cli:%p", cmd, this));

   int cmd_res = cmd_ignore;

   if (cmd->IsCanceled())
      cmd_res = cmd_false;
   else
      cmd_res = PreviewCommand(cmd);

   if (cmd_res == cmd_ignore)
      cmd_res = ExecuteCommand(cmd);

   if (cmd_res == cmd_ignore) {
      EOUT(("Command ignored %s", cmd->GetName()));
      cmd_res = cmd_false;
   }

   bool completed = ((cmd_res == cmd_false) || (cmd_res == cmd_true));

   DOUT5(("Execute command %p %s res = %d", cmd, (completed ? cmd->GetName() : "-----"), cmd_res));

   if (completed)
      dabc::Command::Reply(cmd, cmd_res != cmd_false);

   DOUT5(("ProcessCommand command %p done res = %d", cmd, cmd_res));

   return cmd_res;
}

int dabc::CommandReceiver::ExecuteCommand(dabc::Command* cmd)
{
   // this is normal place where command should be executed
   // Function returns result of command execution
   // No any dabc::Command::Reply(cmd, res) should be used here
   // Function can return cmd_postponed indicating, that execution
   // will be done later and object will take responsibility for reply

   return cmd_false;
}

bool dabc::CommandReceiver::Submit(Command* cmd)
{
   // this method is called from outside to provide command for execution
   // it should be always thread-safe!
   // If object has no own thread, processing of command must be done immideately
   // If special thread is exists, one should put command in the queue and
   // ProcessCommand should be called from the thread

   // Here is default implementation - command is processed immidiately

   // Method returns true when it accept command for execution

   if (cmd==0) return false;

   return ProcessCommand(cmd) != cmd_false;
}

bool dabc::CommandReceiver::Execute(Command* cmd, double timeout_sec)
{
   if (cmd==0) return false;

   bool res = true;

   if (cmd->IsClient()) {
      // this is a case when client is assigned and probably waits for command to be executed
      // therefore we should not wait for execution, just do command and exit
      if (IsExecutionThread())
         res = (ProcessCommand(cmd) != cmd_false);
      else
         if (!Submit(cmd)) res = false;
   } else {

      CommandClient cli;

      if (IsExecutionThread()) {

         cli.Assign(cmd);

         int cmd_res = ProcessCommand(cmd);

         if (cmd_res!=cmd_postponed)
            return (cmd_res == cmd_true);
      } else

      if (!Submit(cli.Assign(cmd))) return false;

      res = cli.WaitCommands(timeout_sec);
   }

   return res;
}

bool dabc::CommandReceiver::Execute(const char* cmdname, double timeout_sec)
{
   return Execute(new Command(cmdname), timeout_sec);
}

int dabc::CommandReceiver::ExecuteInt(const char* cmdname, const char* intresname, double timeout_sec)
{
   dabc::Command* cmd = new dabc::Command(cmdname);
   cmd->SetKeepAlive(true);

   int res = -1;

   if (Execute(cmd, timeout_sec))
      res = cmd->GetInt(intresname, -1);

   dabc::Command::Finalise(cmd);

   return res;
}

dabc::String dabc::CommandReceiver::ExecuteStr(const char* cmdname, const char* strresname, double timeout_sec)
{
   dabc::Command* cmd = new dabc::Command(cmdname);
   cmd->SetKeepAlive(true);

   dabc::String res;

   if (Execute(cmd, timeout_sec))
      res = cmd->GetStr(strresname, "");

   dabc::Command::Finalise(cmd);

   return res;
}

// _______________________________________________________________

dabc::CommandClient::CommandClient(bool keepcmds) :
   CommandClientBase(),
   fKeepCmds(keepcmds),
   fReplyCond(0),
   fReplyedCmds(true, true),
   fReplyedRes(true)
{
   fReplyCond = new Condition(fCmdsMutex);
}

dabc::CommandClient::~CommandClient()
{
   fReplyedCmds.Cleanup();

   LockGuard guard(fCmdsMutex);
   delete fReplyCond; fReplyCond = 0;
}

void dabc::CommandClient::Reset()
{
   fReplyedRes = true;

   fReplyedCmds.Cleanup();

   fReplyCond->Reset();
}


bool dabc::CommandClient::_ProcessReply(Command* cmd)
{
   if (cmd==0) return false;

   if (!cmd->GetResult()) fReplyedRes = false;

   fReplyedCmds.Push(cmd);

   fReplyCond->_DoFire();

//   DOUT1(("CommandClient %p _ProcessReply %s", this, cmd->GetName()));

   return true;
}

bool dabc::CommandClient::WaitCommands(double timeout_sec)
{
   TimeStamp_t stoptm = TimeShift(TimeStamp(),  timeout_sec);

   double waittm = timeout_sec;

   DOUT5(("WaitCommands %3.1f size %d locked %s", timeout_sec, _NumSubmCmds(), DBOOL(fCmdsMutex->IsLocked())));

   do {
      {
        LockGuard guard(fCmdsMutex);
        // no need to wait if no commands is waiting
        if (_NumSubmCmds()==0) break;

        if (!fReplyCond->_DoWait(waittm)) break;

        DOUT5(("Leave %p _DoWait, tmout = %5.3f number = %d", this, waittm,  _NumSubmCmds()));
      }

      if (timeout_sec>0)
         waittm = TimeDistance(TimeStamp(), stoptm);

   } while ((timeout_sec<0.) || (waittm>0));

   int sz = CancelCommands();

   if (sz>0)
      EOUT(("Some (%d) commands was not processed tmout %5.3f", sz, timeout_sec));

   bool result = fReplyedRes && (sz==0);

   fReplyedRes = true;

   fReplyCond->Reset();

   if (!fKeepCmds) fReplyedCmds.Cleanup();

   return result;
}

// ______________________________________________

