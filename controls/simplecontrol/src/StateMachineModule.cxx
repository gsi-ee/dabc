#include "dabc/StateMachineModule.h"

#include "dabc/Command.h"
#include "dabc/Manager.h"

dabc::StateMachineModule::StateMachineModule(Manager* mgr) : 
   ModuleAsync(mgr,"SMmodule")
{
}

int dabc::StateMachineModule::ExecuteCommand(Command* cmd)
{
   if (cmd->IsName("StateTransition")) {
       
      const char* stcmd = cmd->GetStr("Cmd");
      
      dabc::String prev_state = GetManager()->CurrentState();
      
      bool res = GetManager()->DoStateTransition(stcmd);
      
      DOUT1(( "StateTransition %s -> %s  res:%s", prev_state.c_str(), GetManager()->CurrentState(), DBOOL(res)));
      
      return cmd_bool(res);
   }
      
   return dabc::ModuleAsync::ExecuteCommand(cmd);
}
