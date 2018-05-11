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

hadaq::BnetMasterModule::BnetMasterModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   fControl = Cfg("Controller", cmd).AsBool(false);
   fRunPrefix = Cfg("RunPrefix", cmd).AsStr();
   fMaxRunSize = Cfg("MaxRunSize", cmd).AsUInt(2000);

   double period = Cfg("period", cmd).AsDouble(fControl ? 0.2 : 1);
   CreateTimer("update", period);

   fCmdCnt = 1;

   fCtrlId = 1;
   fCtrlTm.GetNow();
   fCtrlCnt = 0;

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
   } else if (cmd.GetInt("#bnet_cnt") == fCmdCnt) {
      if (!fCurrentCmd.null()) {
         int cnt = cmd.GetInt("#RetCnt");
         cmd.SetInt("#RetCnt", cnt--);
         if (cnt<=0) fCurrentCmd.Reply(dabc::cmd_true);
      }

      return true;
   } if (cmd.GetInt("#bnet_ctrl_id") == fCtrlId) {
      fCtrlCnt--;

      dabc::Hierarchy h = dabc::CmdGetNamesList::GetResNamesList(cmd);
      dabc::Iterator iter(h);

      while (iter.next()) {
         dabc::Hierarchy item = iter.ref();
         if (!item.HasField("_bnet")) continue;
         // normally only that item should be used

         std::string state = item.GetField("state").AsStr();

         if (state=="Ready") {
            // ok
         } else if (((state=="NoCalibr") || (state=="NoFile")) && (fCtrlState<2)) {
            fCtrlState = 1;
            if (fCtrlStateName.empty()) fCtrlStateName = state;
         } else {
            fCtrlState = 2;
            fCtrlStateName = "Error";
         }

         if ((fMaxRunSize > 0) && (item.GetField("runsize").AsUInt() > fMaxRunSize*1e6))
            fCtrlSzLimit = true;

         // DOUT0("BNET reply from %s state %s sz %u", item.GetField("_bnet").AsStr().c_str(), item.GetField("state").AsStr().c_str(), item.GetField("runsize").AsUInt());
      }

      if (fCtrlCnt==0) {
         if (fCtrlStateName.empty()) fCtrlStateName = "Ready";
         fWorkerHierarchy.GetHChild("State").SetField("value", fCtrlStateName);
         DOUT0("BNET control sequence ready state %s limit %s", fCtrlStateName.c_str(), DBOOL(fCtrlSzLimit));
      }

   }

   return dabc::Module::ReplyCommand(cmd);
}

void hadaq::BnetMasterModule::BeforeModuleStart()
{
}

void hadaq::BnetMasterModule::ProcessTimerEvent(unsigned timer)
{
   dabc::CmdGetNamesList cmd;
   dabc::WorkerRef publ = GetPublisher();

   publ.Submit(Assign(cmd));

   if (!fCurrentCmd.null() && fCurrentCmd.IsTimedout())
      fCurrentCmd.Reply(dabc::cmd_false);

   if (!fControl) return;

   if (fCtrlCnt>0) {
      // wait at least 2 seconds before sending new request
      if (!fCtrlTm.Expired(2.)) return;

      EOUT("Fail to get %d control records", fCtrlCnt);

   }

   fCtrlCnt = 0;
   fCtrlId++;
   fCtrlTm.GetNow();

   fCtrlSzLimit = false;
   fCtrlState = 0;
   fCtrlStateName = "";

   std::vector<std::string> builders = fWorkerHierarchy.GetHChild("Builders").Field("value").AsStrVect();
   std::vector<std::string> inputs = fWorkerHierarchy.GetHChild("Inputs").Field("value").AsStrVect();

   for (unsigned n=0;n<builders.size();++n) {
      dabc::CmdGetBinary subcmd(builders[n], "hierarchy","");
      subcmd.SetInt("#bnet_ctrl_id", fCtrlId);
      publ.Submit(Assign(subcmd));
      fCtrlCnt++;
   }

   for (unsigned n=0;n<inputs.size();++n) {
      dabc::CmdGetBinary subcmd(inputs[n], "hierarchy", "");
      subcmd.SetInt("#bnet_ctrl_id", fCtrlId);
      publ.Submit(Assign(subcmd));
      fCtrlCnt++;
   }

   if (fCtrlCnt == 0)
      fWorkerHierarchy.GetHChild("State").SetField("value", "NoNodes");
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
            fRunPrefix = cmd.GetStr("prefix", "test");
            query = dabc::format("mode=start&runid=%u&prefix=%s", runid, fRunPrefix.c_str());
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
