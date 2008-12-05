#include "dabc/StateMachineModule.h"

#include "dabc/Command.h"
#include "dabc/Manager.h"

dabc::StateMachineModule::StateMachineModule() :
   ModuleAsync("SMmodule")
{
}

int dabc::StateMachineModule::ExecuteCommand(Command* cmd)
{
   if (cmd->IsName(CmdStateTransition::CmdName())) {

      const char* stcmd = cmd->GetStr("Cmd");

      if (!dabc::mgr()->IsStateTransitionAllowed(stcmd, true)) return cmd_false;

      std::string prev_state = dabc::mgr()->CurrentState();

      bool res = dabc::mgr()->DoStateTransition(stcmd);

      DOUT1(( "StateTransition %s -> %s  res:%s", prev_state.c_str(), dabc::mgr()->CurrentState().c_str(), DBOOL(res)));

      return cmd_bool(res);
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}
