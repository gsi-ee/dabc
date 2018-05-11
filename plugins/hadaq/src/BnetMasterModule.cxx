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

#include "hadaq/HadaqTypeDefs.h"

hadaq::BnetMasterModule::BnetMasterModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   fControl = Cfg("Controller", cmd).AsBool(false);
   fMaxRunSize = Cfg("MaxRunSize", cmd).AsUInt(2000000000LU);

   double period = Cfg("period", cmd).AsDouble(fControl ? 0.2 : 1);
   CreateTimer("update", period);

   fCmdCnt = 1;

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

   if (fControl) {
      CreateCmdDef("StartRun").AddArg("prefix", "string", true, "run");
      CreateCmdDef("StopRun");
   }

   // Publish(fWorkerHierarchy, "$CONTEXT$/BNET");
   PublishPars("$CONTEXT$/BNET");

   DOUT0("BNET MASTER Control %s period %3.1f ", DBOOL(fControl), period);
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
   } else if (cmd.GetInt("#bnet_cnt")==fCmdCnt) {
      if (!fCurrentCmd.null()) {
         int cnt = cmd.GetInt("#RetCnt");
         cmd.SetInt("#RetCnt", cnt--);
         if (cnt<=0) fCurrentCmd.Reply(dabc::cmd_true);
      }

      return true;
   }

   return dabc::Module::ReplyCommand(cmd);
}

void hadaq::BnetMasterModule::BeforeModuleStart()
{
}

void hadaq::BnetMasterModule::ProcessTimerEvent(unsigned timer)
{
   dabc::CmdGetNamesList cmd;
   dabc::PublisherRef(GetPublisher()).Submit(Assign(cmd));

   if (!fCurrentCmd.null() && fCurrentCmd.IsTimedout())
      fCurrentCmd.Reply(dabc::cmd_false);

}

int hadaq::BnetMasterModule::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("StartRun") || cmd.IsName("StopRun")) {
      if (!fCurrentCmd.null()) fCurrentCmd.Reply(dabc::cmd_false);

      std::vector<std::string> builders = fWorkerHierarchy.GetHChild("Builders").Field("value").AsStrVect();
      if (builders.size() == 0) return dabc::cmd_true;

      dabc::WorkerRef publ = GetPublisher();
      if (publ.null()) return dabc::cmd_false;

      fCurrentCmd = cmd;
      fCmdCnt++;

      if (!cmd.IsTimeoutSet()) cmd.SetTimeout(10.);

      bool isstart = cmd.IsName("StartRun");

      cmd.SetInt("#RetCnt", builders.size());

      for (int n=0;n<(int) builders.size();++n) {

         std::string query;

         if (isstart) {
            unsigned runid = cmd.GetUInt("runid");
            if (runid==0) runid = hadaq::CreateRunId();
            std::string prefix = cmd.GetStr("prefix", "test");
            query = dabc::format("mode=start&runid=%u&prefix=%s", runid, prefix.c_str());
         } else {
            query = "mode=stop";
         }

         dabc::CmdGetBinary subcmd(builders[n] + "/BnetFileControl", "execute", query);
         subcmd.SetInt("#bnet_cnt", fCmdCnt);

         publ.Submit(Assign(subcmd));
      }

      return dabc::cmd_postponed;
   }

   return dabc::cmd_true;

}
