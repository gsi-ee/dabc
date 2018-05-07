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
#include "dabc/Publisher.h"
#include "dabc/Iterator.h"

hadaq::BnetMasterModule::BnetMasterModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   double period = Cfg("period", cmd).AsDouble(1);

   CreateTimer("update", period);

   fWorkerHierarchy.Create("Bnet");

   fWorkerHierarchy.SetField("_player","DABC.BnetControl");

   dabc::Hierarchy item = fWorkerHierarchy.CreateHChild("State");
   item.SetField(dabc::prop_kind, "Text");
   item.SetField("value", "Init");

   item = fWorkerHierarchy.CreateHChild("Inputs");
   item.SetField(dabc::prop_kind, "Text");
   item.SetField("value", "");
   item.SetField("_hidden", "true");

   item = fWorkerHierarchy.CreateHChild("Builders");
   item.SetField(dabc::prop_kind, "Text");
   item.SetField("value", "");
   item.SetField("_hidden", "true");

   Publish(fWorkerHierarchy, "$CONTEXT$/BNET");

   DOUT0("BNET MASTER period %3.1f", period);
}

bool hadaq::BnetMasterModule::ReplyCommand(dabc::Command cmd)
{
   if (cmd.IsName(dabc::CmdGetNamesList::CmdName())) {
      //DOUT0("Get hierarchy");
      dabc::Hierarchy h = dabc::CmdGetNamesList::GetResNamesList(cmd);
      dabc::Iterator iter(h);
      std::vector<std::string> binp, bbuild, nodes_inp, nodes_build;
      while (iter.next()) {
         dabc::Hierarchy item = iter.ref();
         if (item.HasField("_bnet")) {
            std::string kind = item.GetField("_bnet").AsStr(),
                        producer = item.GetField("_producer").AsStr();

            std::size_t pos = producer.find_last_of("/");
            if (pos != std::string::npos) producer.resize(pos);

            if (kind == "sender") { binp.push_back(item.ItemName()); nodes_inp.push_back(producer); }
            if (kind == "receiver") { bbuild.push_back(item.ItemName()); nodes_build.push_back(producer); }
            //DOUT0("Get BNET %s", item.ItemName().c_str());
         }
      }

      fWorkerHierarchy.GetHChild("Inputs").SetField("value", binp);
      fWorkerHierarchy.GetHChild("Inputs").SetField("nodes", nodes_inp);
      fWorkerHierarchy.GetHChild("Builders").SetField("value", bbuild);
      fWorkerHierarchy.GetHChild("Builders").SetField("nodes", nodes_build);

      return true;
   }

   return true;
}

void hadaq::BnetMasterModule::BeforeModuleStart()
{
}

void hadaq::BnetMasterModule::ProcessTimerEvent(unsigned timer)
{
   dabc::CmdGetNamesList cmd;
   dabc::PublisherRef(GetPublisher()).Submit(Assign(cmd));
}
