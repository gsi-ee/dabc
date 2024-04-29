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

#include "dogma/CommandModule.h"

#include "dabc/Manager.h"

dogma::CommandModule::CommandModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
}

int dogma::CommandModule::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("GenericRead")) {
      DOUT5("Read at %x", cmd.GetInt("Addr"));
      cmd.SetInt("Value", 5678);
      return dabc::cmd_true;
   }

   if (cmd.IsName("GenericWrite")) {
      return dabc::cmd_true;
   }

   return dabc::cmd_false;
}

void dogma::CommandModule::BeforeModuleStart()
{
   DOUT0("Starting DOGMA module");
}

void dogma::CommandModule::ProcessTimerEvent(unsigned)
{
}
