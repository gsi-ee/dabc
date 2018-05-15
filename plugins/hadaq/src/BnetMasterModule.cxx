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
   fMaxRunSize = Cfg("MaxRunSize", cmd).AsUInt(2000);

   double period = Cfg("period", cmd).AsDouble(fControl ? 0.2 : 1);
   CreateTimer("update", period);

   fCmdCnt = 1;

   fCtrlId = 1;
   fCtrlTm.GetNow();
   fCtrlCnt = 0;

   fWorkerHierarchy.Create("Bnet");

   fWorkerHierarchy.SetField("_player","DABC.BnetControl");

   dabc::Hierarchy item = fWorkerHierarchy.CreateHChild("Inputs");
   item.SetField(dabc::prop_kind, "Text");
   item.SetField("value", "");
   item.SetField("_hidden", "true");

   item = fWorkerHierarchy.CreateHChild("Builders");
   item.SetField(dabc::prop_kind, "Text");
   item.SetField("value", "");
   item.SetField("_hidden", "true");

   CreatePar("State").SetFld(dabc::prop_kind, "Text").SetValue("Init");

   CreatePar("DataRate").SetUnits("MB").SetFld(dabc::prop_kind,"rate").SetFld("#record", true);
   CreatePar("EventsRate").SetUnits("Ev").SetFld(dabc::prop_kind,"rate").SetFld("#record", true);

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

      if (fCtrlCnt != 0) {
         if (!fCtrlTm.Expired()) return true;
         if (fCtrlCnt > 0) EOUT("Fail to get %d control records", fCtrlCnt);
      }

      fCtrlCnt = 0;
      fCtrlId++;
      fCtrlTm.GetNow(3.);

      fCtrlSzLimit = false;
      fCtrlState = 0;
      fCtrlStateName = "";
      fCtrlData = 0.;
      fCtrlEvents = 0.;

      dabc::WorkerRef publ = GetPublisher();

      for (unsigned n=0;n<bbuild.size();++n) {
         dabc::CmdGetBinary subcmd(bbuild[n], "hierarchy", "childs");
         subcmd.SetInt("#bnet_ctrl_id", fCtrlId);
         publ.Submit(Assign(subcmd));
         fCtrlCnt++;
      }

      for (unsigned n=0;n<binp.size();++n) {
         dabc::CmdGetBinary subcmd(binp[n], "hierarchy", "childs");
         subcmd.SetInt("#bnet_ctrl_id", fCtrlId);
         publ.Submit(Assign(subcmd));
         fCtrlCnt++;
      }

      if (fCtrlCnt == 0)
         SetParValue("State", "NoNodes");

      return true;

   } else if (cmd.GetInt("#bnet_cnt") == fCmdCnt) {
      // this commands used to send file requests

      if (!fCurrentFileCmd.null()) {
         int cnt = cmd.GetInt("#RetCnt");
         cmd.SetInt("#RetCnt", cnt--);
         if (cnt<=0) fCurrentFileCmd.Reply(dabc::cmd_true);
      }

      return true;

   } if (cmd.GetInt("#bnet_ctrl_id") == fCtrlId) {
      // this commands used to send control requests

      fCtrlCnt--;

      dabc::Hierarchy h = dabc::CmdGetNamesList::GetResNamesList(cmd);
      dabc::Iterator iter(h);

      bool is_builder = false;


      while (iter.next()) {
         dabc::Hierarchy item = iter.ref();
         if (!item.HasField("_bnet")) {
            if (item.IsName("HadaqData") && is_builder) fCtrlData += item.GetField("value").AsDouble(); else
            if (item.IsName("HadaqEvents") && is_builder) fCtrlEvents += item.GetField("value").AsDouble();
            continue;
         }
         // normally only that item should be used

         if (item.GetField("_bnet").AsStr() == "receiver") is_builder = true;

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
         SetParValue("State", fCtrlStateName);

         DOUT0("BNET control sequence ready state %s limit %s", fCtrlStateName.c_str(), DBOOL(fCtrlSzLimit));

         Par("DataRate").SetValue(fCtrlData);
         Par("EventsRate").SetValue(fCtrlEvents);

         if (fControl && fCtrlSzLimit && fCurrentFileCmd.null()) {
            // this is a place, where new run automatically started
            dabc::Command newrun("StartRun");
            newrun.SetTimeout(20);
            fCtrlCnt = -1;
            fCtrlId++;
            fCtrlTm.GetNow(7.); // prevent to make next request very fast
            Submit(newrun);
         }
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

   if (!fCurrentFileCmd.null() && fCurrentFileCmd.IsTimedout())
      fCurrentFileCmd.Reply(dabc::cmd_false);
}

int hadaq::BnetMasterModule::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("StartRun") || cmd.IsName("StopRun")) {
      if (!fCurrentFileCmd.null()) fCurrentFileCmd.Reply(dabc::cmd_false);

      std::vector<std::string> builders = fWorkerHierarchy.GetHChild("Builders").GetField("value").AsStrVect();
      if (builders.size() == 0) return dabc::cmd_true;

      dabc::WorkerRef publ = GetPublisher();
      if (publ.null()) return dabc::cmd_false;

      fCurrentFileCmd = cmd;
      fCmdCnt++;

      if (!cmd.IsTimeoutSet()) cmd.SetTimeout(10.);

      bool isstart = cmd.IsName("StartRun");

      cmd.SetInt("#RetCnt", builders.size());

      std::string query;

      if (isstart) {
         unsigned runid = cmd.GetUInt("runid");
         if (runid==0) runid = hadaq::CreateRunId();
         query = dabc::format("mode=start&runid=%u", runid);

         std::string prefix = cmd.GetStr("prefix");
         if (!prefix.empty()) {
            query.append("&prefix=");
            query.append(prefix);
         }
         DOUT0("Starting new run %s", query.c_str());
      } else {
         query = "mode=stop";
      }

      for (unsigned n=0; n<builders.size(); ++n) {
         dabc::CmdGetBinary subcmd(builders[n] + "/BnetFileControl", "execute", query);
         subcmd.SetInt("#bnet_cnt", fCmdCnt);
         publ.Submit(Assign(subcmd));
      }

      return dabc::cmd_postponed;
   }

   return dabc::cmd_true;

}
