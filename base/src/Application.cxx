#include "dabc/Application.h"

#include "dabc/Manager.h"
#include "dabc/logging.h"

dabc::Application::Application(const char* name) :
   Folder(dabc::mgr(), name ? name : "App", true),
   WorkingProcessor(this),
   fConnCmd(0),
   fConnTmout(0)
{
   DOUT3(("Plugin %s created", GetName()));
}


dabc::Application::~Application()
{
   DOUT3(("Start Application %s destructor", GetName()));

   dabc::Command::Reply(fConnCmd, false);
   fConnCmd = 0;

   // delete childs (parameters) before destructor is finished that one
   // can correcly use GetParsHolder() in Manager::ParameterEvent()
   DeleteChilds();

   DOUT3(("Did Application %s destructor", GetName()));
}

int dabc::Application::ExecuteCommand(dabc::Command* cmd)
{
   int cmd_res = cmd_false;

   if (cmd->IsName("CreateAppModules")) {
      cmd_res = cmd_bool(CreateAppModules());
   } else
   if (cmd->IsName("ConnectAppModules")) {
      if (fConnCmd!=0) {
         EOUT(("There is active connect command !!!"));
      } else {
         int res = IsAppModulesConnected();
         if (res==1) cmd_res = cmd_true; else
         if (res==0) cmd_res = cmd_false; else {
            fConnCmd = cmd;
            fConnTmout = SMCommandTimeout();
            ActivateTimeout(0.2);
            cmd_res = cmd_postponed;
         }
      }
   } else
   if (cmd->IsName("BeforeAppModulesStarted")) {
      cmd_res = cmd_bool(BeforeAppModulesStarted());
   } else
   if (cmd->IsName("AfterAppModulesStopped")) {
      cmd_res = cmd_bool(AfterAppModulesStopped());
   } else
   if (cmd->IsName("BeforeAppModulesDestroyed")) {
      cmd_res = cmd_bool(BeforeAppModulesDestroyed());
   } else
   if (cmd->IsName("CheckModulesRunning")) {
      if (!IsModulesRunning()) {
         if (strcmp(dabc::mgr()->CurrentState(), dabc::Manager::stReady) != 0) {
            DOUT1(("!!!!! ******** !!!!!!!!  All main modules are stopped - we can switch to Stop state"));
            dabc::mgr()->InvokeStateTransition(dabc::Manager::stcmdDoStop);
         }
      }
   } else
      cmd_res = dabc::WorkingProcessor::ExecuteCommand(cmd);

   return cmd_res;
}

double dabc::Application::ProcessTimeout(double last_diff)
{
   // we using timeout events to check if connection is established

   fConnTmout -= last_diff;

   int res = IsAppModulesConnected();

   if ((res==2) && (fConnTmout<0)) res = 0;

   if ((res==1) || (res==0)) {
      dabc::Command::Reply(fConnCmd, (res==1));
      fConnCmd = 0;
      return -1;
   }

   return 0.2;
}


bool dabc::Application::DoStateTransition(const char* state_trans_name)
{
   // method called from SM thread, one should use Execute at this place

   bool res = true;

   if (strcmp(state_trans_name, dabc::Manager::stcmdDoConfigure)==0) {

      DOUT1(("Doing CreateAppModules %d", SMCommandTimeout()));

      res = Execute("CreateAppModules", SMCommandTimeout());

      DOUT1(("Did CreateAppModules"));

      res = dabc::mgr()->CreateMemoryPools() && res;

      DOUT1(("Did CreateMemoryPools"));

   } else
   if (strcmp(state_trans_name, dabc::Manager::stcmdDoEnable)==0) {
      res = Execute("ConnectAppModules", SMCommandTimeout());
   } else
   if (strcmp(state_trans_name, dabc::Manager::stcmdDoStart)==0) {
      res = Execute("BeforeAppModulesStarted", SMCommandTimeout());
      res = dabc::mgr()->StartAllModules() && res;
   } else
   if (strcmp(state_trans_name, dabc::Manager::stcmdDoStop)==0) {
      res = dabc::mgr()->StopAllModules();
      res = Execute("AfterAppModulesStopped", SMCommandTimeout()) && res;
   } else
   if (strcmp(state_trans_name, dabc::Manager::stcmdDoHalt)==0) {
      res = Execute("BeforeAppModulesDestroyed", SMCommandTimeout());
      res = dabc::mgr()->CleanupManager() && res;
   } else
   if (strcmp(state_trans_name, dabc::Manager::stcmdDoError)==0) {
      res = dabc::mgr()->StopAllModules();
   } else
      res = false;

   return res;
}

bool dabc::Application::IsModulesRunning()
{
   return dabc::mgr()->IsAnyModuleRunning();
}

void dabc::Application::InvokeCheckModulesCmd()
{
   Submit(new Command("CheckModulesRunning"));
}
