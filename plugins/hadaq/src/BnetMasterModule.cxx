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

   fSameBuildersCnt = 0;

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

   item = fWorkerHierarchy.CreateHChild("LastPrefix");
   item.SetField(dabc::prop_kind, "Text");
   item.SetField("value", "");
   item.SetField("_hidden", "true");

   CreatePar("State").SetFld(dabc::prop_kind, "Text").SetValue("Init");
   CreatePar("Quality").SetFld(dabc::prop_kind, "Text").SetValue("0.5");

   CreatePar("RunId").SetFld(dabc::prop_kind, "Text").SetValue("--");
   CreatePar("RunIdStr").SetFld(dabc::prop_kind, "Text").SetValue("--");
   CreatePar("RunPrefix").SetFld(dabc::prop_kind, "Text").SetValue("--");

   CreatePar("DataRate").SetUnits("MB").SetFld(dabc::prop_kind,"rate").SetFld("#record", true);
   CreatePar("EventsRate").SetUnits("Ev").SetFld(dabc::prop_kind,"rate").SetFld("#record", true);

   if (fControl) {
      CreateCmdDef("StartRun").AddArg("prefix", "string", true, "run")
                              .AddArg("oninit", "int", false, "0");
      CreateCmdDef("StopRun");
      CreateCmdDef("ResetDAQ");
   }

   // Publish(fWorkerHierarchy, "$CONTEXT$/BNET");
   PublishPars("$CONTEXT$/BNET");

   DOUT0("BNET MASTER Control %s period %3.1f ", DBOOL(fControl), period);
}

void hadaq::BnetMasterModule::AddItem(std::vector<std::string> &items, std::vector<std::string> &nodes, const std::string &item, const std::string &node)
{
   auto iter1 = items.begin();
   auto iter2 = nodes.begin();
   while (iter1 != items.end()) {
      if (*iter1 > item) {
         items.insert(iter1, item);
         nodes.insert(iter2, node);
         return;
      }
      ++iter1;
      ++iter2;
   }

   items.emplace_back(item);
   nodes.emplace_back(node);
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

            if (kind == "sender") AddItem(binp, nodes_inp, item.ItemName(), producer);
            if (kind == "receiver") AddItem(bbuild, nodes_build, item.ItemName(), producer);
            //DOUT0("Get BNET %s", item.ItemName().c_str());
         }
      }

      if ((fLastBuilders.size()>0) && (fLastBuilders == bbuild)) {
         fSameBuildersCnt++;
         if (!fInitRunCmd.null() && (fSameBuildersCnt>cmd.GetInt("oninit"))) {
            DOUT0("DETECTED SAME BUILDERS %d", fSameBuildersCnt);

            fInitRunCmd.SetBool("#verified", true);
            int res = ExecuteCommand(fInitRunCmd);
            if (res != dabc::cmd_postponed)
               fInitRunCmd.Reply(res);
            else
               fInitRunCmd.Release();
         }

      } else {
         fSameBuildersCnt = 0;
      }

      fLastBuilders = bbuild;

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
      fCtrlStateQuality = 1;
      fCtrlStateName = "";
      fCtrlData = 0.;
      fCtrlEvents = 0.;
      fCtrlInpNodesCnt = 0;
      fCtrlInpNodesExpect = 0;
      fCtrlBldNodesCnt = 0;
      fCtrlBldNodesExpect = 0;

      fCtrlRunId = 0;
      fCtrlRunPrefix = "";

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

         if (is_builder) fCtrlBldNodesCnt++;
                    else fCtrlInpNodesCnt++;

         std::string state = item.GetField("state").AsStr();
         double quality = item.GetField("quality").AsDouble();

         if (fCtrlStateName.empty() || (quality < fCtrlStateQuality)) {
            fCtrlStateQuality = quality;
            fCtrlStateName = state;
         }

         if (is_builder) {
            // check maximal size
            if ((fMaxRunSize > 0) && (item.GetField("runsize").AsUInt() > fMaxRunSize*1e6))
               fCtrlSzLimit = true;

            // check current runid
            unsigned runid = item.GetField("runid").AsUInt();
            std::string runprefix = item.GetField("runprefix").AsStr();

            if (runid && !runprefix.empty()) {
               if (!fCtrlRunId) {
                  fCtrlRunId = runid;
                  fCtrlRunPrefix = runprefix;
               } else if ((fCtrlRunId != runid) || (fCtrlRunPrefix != runprefix)) {
                  if (fCtrlStateQuality>0) {
                     fCtrlStateName = "RunMismatch";
                     fCtrlStateQuality = 0;
                  }
               }
            }

            int ninputs = item.GetField("ninputs").AsInt();
            if (fCtrlInpNodesExpect == 0) fCtrlInpNodesExpect = ninputs;

            if ((fCtrlInpNodesExpect != ninputs) && (fCtrlStateQuality > 0)) {
               fCtrlStateName = "InputsMismatch";
               fCtrlStateQuality = 0;
            }

         } else {
            int nbuilders = item.GetField("nbuilders").AsInt();
            if (fCtrlBldNodesExpect==0) fCtrlBldNodesExpect = nbuilders;
            if ((fCtrlBldNodesExpect != nbuilders) && (fCtrlStateQuality > 0)) {
               fCtrlStateName = "BuildersMismatch";
               fCtrlStateQuality = 0;
            }
         }

         // DOUT0("BNET reply from %s state %s sz %u", item.GetField("_bnet").AsStr().c_str(), item.GetField("state").AsStr().c_str(), item.GetField("runsize").AsUInt());
      }

      if (fCtrlCnt==0) {
         if (fCtrlStateName.empty()) {
            fCtrlStateName = "Ready";
            fCtrlStateQuality = 1.;
         }
         if ((fCtrlInpNodesCnt == 0) && (fCtrlStateQuality > 0)) {
            fCtrlStateName = "NoInputs";
            fCtrlStateQuality = 0;
         }

         if ((fCtrlInpNodesExpect != fCtrlInpNodesCnt) && (fCtrlStateQuality > 0)) {
            fCtrlStateName = "InputsMismatch";
            fCtrlStateQuality = 0;
         }

         if ((fCtrlBldNodesCnt == 0)  && (fCtrlStateQuality > 0)) {
            fCtrlStateName = "NoBuilders";
            fCtrlStateQuality = 0;
         }
         if ((fCtrlBldNodesExpect != fCtrlBldNodesCnt) && (fCtrlStateQuality > 0)) {
            fCtrlStateName = "BuildersMismatch";
            fCtrlStateQuality = 0;
         }

         SetParValue("State", fCtrlStateName);
         SetParValue("Quality", fCtrlStateQuality);
         SetParValue("RunId", fCtrlRunId);
         SetParValue("RunIdStr", fCtrlRunId ? hadaq::FormatFilename(fCtrlRunId,0) : std::string("0"));
         SetParValue("RunPrefix", fCtrlRunPrefix);

         fWorkerHierarchy.GetHChild("LastPrefix").SetField("value", fCtrlRunPrefix);

         DOUT3("BNET control sequence ready state %s limit %s", fCtrlStateName.c_str(), DBOOL(fCtrlSzLimit));

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

   if (!fInitRunCmd.null() && fInitRunCmd.IsTimedout())
      fInitRunCmd.Reply(dabc::cmd_false);
}

int hadaq::BnetMasterModule::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("StartRun") || cmd.IsName("StopRun")) {

      bool isstart = cmd.IsName("StartRun");

      DOUT0("Command %s oninit %s", cmd.GetName(), cmd.GetStr("oninit").c_str());

      if (isstart && (cmd.GetInt("oninit")>0) && !cmd.GetBool("#verified")) {
         DOUT0("Detect START RUN with oninit flag!!!!");

         // this is entry point for StartRun command during initialization
         // we remember it and checks that at least two time same list of input nodes are collected
         if (!fInitRunCmd.null()) fInitRunCmd.Reply(dabc::cmd_false);
         fInitRunCmd = cmd;
         fSameBuildersCnt = 0; // reset counter
         if (!cmd.IsTimeoutSet()) cmd.SetTimeout(30. + cmd.GetInt("oninit")*2.);
         return dabc::cmd_postponed;
      }

      if (!fCurrentFileCmd.null()) fCurrentFileCmd.Reply(dabc::cmd_false);

      std::vector<std::string> builders = fWorkerHierarchy.GetHChild("Builders").GetField("value").AsStrVect();
      if (builders.size() == 0) return dabc::cmd_true;

      dabc::WorkerRef publ = GetPublisher();
      if (publ.null()) return dabc::cmd_false;

      fCurrentFileCmd = cmd;
      fCmdCnt++;

      if (!cmd.IsTimeoutSet()) cmd.SetTimeout(10.);

      cmd.SetInt("#RetCnt", builders.size());

      std::string query;
      std::string prefix;
      unsigned runid = 0;
      if (isstart) {
         prefix = cmd.GetStr("prefix");
         if (prefix == "NO_FILE" || prefix == "--")
            isstart = false;
      }
      if (isstart) {
         runid = cmd.GetUInt("runid");
         if (runid == 0)
            runid = hadaq::CreateRunId();
         query = dabc::format("mode=start&runid=%u", runid);
         if (!prefix.empty()) {
            query.append("&prefix=");
            query.append(prefix);
         }
         DOUT0("Starting new run %s", query.c_str());
      } else {
         query = "mode=stop";
      }

      std::string lastprefix = fWorkerHierarchy.GetHChild("LastPrefix").GetField("value").AsStr();
      fWorkerHierarchy.GetHChild("LastPrefix").SetField("value", prefix);

      for (unsigned n=0; n<builders.size(); ++n) {
         dabc::CmdGetBinary subcmd(builders[n] + "/BnetFileControl", "execute", query);
         subcmd.SetInt("#bnet_cnt", fCmdCnt);
         publ.Submit(Assign(subcmd));
      }

      query = "";

      if (isstart && (prefix == "tc") && lastprefix.empty()) {
         query = dabc::format("mode=start&runid=%u", runid);
      } else if (!isstart && (lastprefix == "tc")) {
         query = "mode=stop";
      }

      if (!query.empty()) {

         DOUT0("Mster CLIBRATION query %s", query.c_str());

         // trigger calibration start for all TDCs
         std::vector<std::string> inputs = fWorkerHierarchy.GetHChild("Inputs").GetField("value").AsStrVect();

         for (unsigned n=0; n<inputs.size(); ++n) {
            dabc::CmdGetBinary subcmd(inputs[n] + "/BnetCalibrControl", "execute", query);
            publ.Submit(subcmd);
         }
      }

      return dabc::cmd_postponed;
   } else if (cmd.IsName("ResetDAQ")) {
      std::vector<std::string> builders = fWorkerHierarchy.GetHChild("Builders").GetField("value").AsStrVect();
      std::vector<std::string> inputs = fWorkerHierarchy.GetHChild("Inputs").GetField("value").AsStrVect();

      dabc::WorkerRef publ = GetPublisher();
      if (publ.null()) return dabc::cmd_false;

      for (unsigned n=0; n<inputs.size(); ++n) {
         dabc::CmdGetBinary subcmd(inputs[n], "cmd.json", "command=DropAllBuffers");
         publ.Submit(subcmd);
      }

      for (unsigned n=0; n<builders.size(); ++n) {
         dabc::CmdGetBinary subcmd(builders[n], "cmd.json", "command=DropAllBuffers");
         publ.Submit(subcmd);
      }

      return dabc::cmd_true;
   }

   return dabc::cmd_true;

}
