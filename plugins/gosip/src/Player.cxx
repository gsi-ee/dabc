// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "gosip/Player.h"

#include "dabc/Publisher.h"


gosip::Player::Player(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   fWorkerHierarchy.Create("FESA", true);

   dabc::CommandDefinition cmddef = CreateCmdDef("CmdGosip");
   //cmddef.SetField(dabc::prop_auth, true); // require authentication
   cmddef.AddArg("cmd", "string", true, "-");
   
   dabc::Hierarchy ui = fWorkerHierarchy.CreateHChild("UI");
   ui.SetField(dabc::prop_kind, "DABC.HTML");
   ui.SetField("dabc:UserFilePath", "${DABCSYS}/plugins/gosip/htm/");
   ui.SetField("dabc:UserFileMain", "gosip.htm");

   CreateTimer("update", 1., false);
   
   PublishPars("GOSIP/Test");
}

gosip::Player::~Player()
{
}

void gosip::Player::ProcessTimerEvent(unsigned timer)
{
   //dabc::Hierarchy ui = fWorkerHierarchy.FindChild("UserInterface");
   //DOUT0("Process timer '%s'", ui.GetField("FilePath").AsStr().c_str());
}


int gosip::Player::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("CmdGosip")) {

      std::string gosipcmd = cmd.GetStr("cmd");
      DOUT0("****************** CmdGosip %s ****************", gosipcmd.c_str());
      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}
