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

#include "hadaq/BnetMasterModule.h"

#include "dabc/Manager.h"

hadaq::BnetMasterModule::BnetMasterModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   double period = Cfg("period", cmd).AsDouble(1);

   CreateTimer("update", period, false);

   fWorkerHierarchy.Create("Bnet");

   dabc::Hierarchy item = fWorkerHierarchy.CreateHChild("State");
   item.SetField(dabc::prop_kind, "Text");
   item.SetField("value", "Init");

   item = fWorkerHierarchy.CreateHChild("Inputs");
   item.SetField(dabc::prop_kind, "Text");
   item.SetField("value", "Init");
   item.SetField("_hidden", "true");

   item = fWorkerHierarchy.CreateHChild("Outputs");
   item.SetField(dabc::prop_kind, "Text");
   item.SetField("value", "Init");
   item.SetField("_hidden", "true");

   Publish(fWorkerHierarchy, "$CONTEXT$/BNET");

   DOUT0("BNET MASTER period %3.1f", period);
}

bool hadaq::BnetMasterModule::ReplyCommand(dabc::Command cmd)
{
   if (cmd.IsName("GetTransportStatistic")) {
      return true;
   }

   return true;
}

void hadaq::BnetMasterModule::BeforeModuleStart()
{
}

void hadaq::BnetMasterModule::ProcessTimerEvent(unsigned timer)
{
}
