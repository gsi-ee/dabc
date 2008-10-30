#include "dabc/CommandsSet.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"

dabc::CommandsSet::CommandsSet(Manager* mgr, Command* cmd, bool parallel_exe) :
   CommandClientBase(),
   WorkingProcessor(),
   fMgr(mgr),
   fReceiver(mgr),
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
      EOUT(("Subcommand %s failed for command %s", cmd->GetName(), (fMain ? fMain->GetName() : "---")));
   }

   DOUT5(("CommandsSet::_ProcessReply this:%p main:%p %s cmd:%s  numsubm = %d numwait = %d res = %s", 
      this, fMain, (fMain ? fMain->GetName() : "---"), cmd->GetName(),
      _NumSubmCmds(), fCmdsSet.Size(), DBOOL(fMainRes)));

   // call user-defined method, which can redirect or even delete command object
   // false means that client will finalize command itself
   bool keep_cmd = ProcessCommandReply(cmd);
   
   // if set is not completed, do not react on the replyes
   if (!fCompleted) return keep_cmd;

   if (((_NumSubmCmds()==0) && (fCmdsSet.Size()==0)) || !fMainRes) {
      DOUT5(("CommandsSet::MainReply %s", (fMain ? fMain->GetName() : "---")));

      dabc::Command::Reply(fMain, fMainRes);
      fMain = 0;

      if (fMgr) {
//         DestroyProcessor();  
//         RemoveProcessorFromThread(false);
         if (fNeedDestroy) {
            fNeedDestroy = false;
            fMgr->DeleteAnyObject(this); 
         }
      } else {
         EOUT(("Manager was not specified. Memory leak"));
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
   
   // command must be detroyed by the user
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
      DOUT5(("Finish main %p while it is completed", set->fMain));
      dabc::Command::Reply(set->fMain, set->fMainRes);
      set->fMain = 0;
   
      delete set;
   } else {
      if (timeout_sec>0) {
         set->AssignProcessorToThread(set->fMgr->ProcessorThread());
         set->ActivateTimeout(timeout_sec);
      }
      DOUT5(("Main command %p keeped alive", set->fMain));
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
