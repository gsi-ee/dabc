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


      std::string tgtstate = dabc::mgr()->TargetStateName(stcmd);

      std::string prev_state = dabc::mgr()->CurrentState();

      if (prev_state == tgtstate) {
         DOUT1(("SM Command %s leads to current state, do nothing", stcmd));
         return cmd_true;
      }

      if (!dabc::mgr()->IsStateTransitionAllowed(stcmd, true)) return cmd_false;

      bool res = dabc::mgr()->DoStateTransition(stcmd);

      DOUT1(( "StateTransition %s -> %s  res:%s", prev_state.c_str(), dabc::mgr()->CurrentState().c_str(), DBOOL(res)));

      return cmd_bool(res);
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}
