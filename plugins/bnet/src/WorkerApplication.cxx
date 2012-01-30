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
#include "bnet/WorkerApplication.h"

#include "bnet/SenderModule.h"
#include "bnet/ReceiverModule.h"

#include "dabc/Parameter.h"
#include "dabc/Command.h"
#include "dabc/timing.h"
#include "dabc/CommandsSet.h"


bnet::WorkerApplication::WorkerApplication(const char* classname) :
   dabc::Application(classname)
{
   CreatePar(xmlIsGenerator).DfltBool(false);
   CreatePar(xmlIsSender).DfltBool(false);
   CreatePar(xmlIsReceiever).DfltBool(false);
   CreatePar(xmlIsFilter).DfltBool(false);
   CreatePar(xmlCombinerModus).DfltInt(0);
   CreatePar(xmlNumReadouts).DfltInt(0);
   for (int nr=0;nr<NumReadouts();nr++)
      CreatePar(ReadoutParName(nr).c_str());
   CreatePar(xmlStoragePar);

   CreatePar(xmlReadoutBuffer).DfltInt(2*1024);
   CreatePar(xmlReadoutPoolSize).DfltInt(4*0x100000);
   CreatePar(xmlTransportBuffer).DfltInt(8*1024);
   CreatePar(xmlTransportPoolSize).DfltInt(16*0x100000);
   CreatePar(xmlEventBuffer).DfltInt(32*1024);
   CreatePar(xmlEventPoolSize).DfltInt(4*0x100000);
   CreatePar(xmlCtrlBuffer).DfltInt(2*1024);
   CreatePar(xmlCtrlPoolSize).DfltInt(2*0x100000);

   CreatePar(parStatus).SetStr("Off");
   CreatePar(parSendStatus).SetStr("oooo");
   CreatePar(parRecvStatus).SetStr("oooo");

   CreatePar(CfgNodeId).SetInt(dabc::mgr()->NodeId());
   CreatePar(CfgNumNodes).SetInt(dabc::mgr()->NumNodes());
   CreatePar(CfgController).SetBool(false);
   CreatePar(CfgSendMask).SetStr("");
   CreatePar(CfgRecvMask).SetStr("");
   CreatePar(CfgClusterMgr).SetStr("");
   CreatePar(CfgNetDevice).SetStr("");
   CreatePar(CfgConnected).SetBool(false);
   CreatePar(CfgEventsCombine).SetInt(1);
   CreatePar(CfgReadoutPool).DfltStr(ReadoutPoolName);

   CreatePar("Info", "info");

   if (IsReceiver()) {
      CreateCmdDef("StartFile").AddArg("FileName", "string", true);

      CreateCmdDef("StopFile");
   }

   DOUT1(("!!!! Worker plugin created name = %s!!!!", GetName()));
}

std::string bnet::WorkerApplication::ReadoutParName(int nreadout)
{
   std::string name;
   dabc::formats(name, "Input%dCfg", nreadout);
   return name;
}

std::string bnet::WorkerApplication::ReadoutPar(int nreadout) const
{
   if ((nreadout<0) || (nreadout>=NumReadouts())) return "";
   return Par(ReadoutParName(nreadout)).AsStdStr();
}

bool bnet::WorkerApplication::CreateStorage(const char* portname)
{
   return CreateOutFile(portname, Par(xmlStoragePar).AsStdStr());
}

bool bnet::WorkerApplication::CreateOutFile(const char* portname, const std::string&)
{
   return dabc::mgr.CreateTransport(portname, "");
}

void bnet::WorkerApplication::DiscoverNodeConfig(dabc::Command cmd)
{

   DOUT1(("Process DiscoverNodeConfig sender:%s recv:%s", DBOOL(IsSender()), DBOOL(IsReceiver())));

   Par(CfgController).SetBool(cmd.GetBool(xmlWithController));
   Par(CfgEventsCombine).SetInt(cmd.GetInt(xmlNumEventsCombine, 1));
   Par(CfgNetDevice).SetStr(cmd.GetStr(xmlNetDevice,""));

   Par(xmlCtrlBuffer).SetInt(cmd.GetInt(xmlCtrlBuffer, 1024));
   Par(xmlTransportBuffer).SetInt(cmd.GetInt(xmlTransportBuffer, 1024));

   cmd.SetBool(xmlIsSender, IsSender());
   cmd.SetBool(xmlIsReceiever, IsReceiver());

   if (IsSender()) {

      dabc::ModuleRef m = dabc::mgr.FindModule("Sender");
      if (!m.null()) {
         dabc::Command cmd2("GetConfig");
         if (m.Execute(cmd2))
            cmd.SetStr(parRecvMask, cmd2.GetStr(parRecvMask));
      }
   }

   if (IsReceiver()) {
      dabc::ModuleRef m = dabc::mgr.FindModule("Receiver");
      if (!m.null()) {
         dabc::Command cmd2("GetConfig");
         if (m.Execute(cmd2))
            cmd.SetStr(parSendMask, cmd2.GetStr(parSendMask));
      }
   }
}

void bnet::WorkerApplication::ApplyNodeConfig(dabc::Command mastercmd)
{
   Par(CfgController).SetBool(mastercmd.GetBool(CfgController, 0));
   Par(CfgSendMask).SetStr(mastercmd.GetStr(parSendMask, "xxxx"));
   Par(CfgRecvMask).SetStr(mastercmd.GetStr(parRecvMask, "xxxx"));
   Par(CfgClusterMgr).SetStr(mastercmd.GetStr(CfgClusterMgr,""));

   dabc::CommandsSet* set = new dabc::CommandsSet(thread(), false);

   if (IsSender()) {
      dabc::Command cmd("Configure");
      cmd.Field("Standalone").SetBool(!Par(CfgController).AsBool());
      cmd.Field(parRecvMask).SetStr(Par(CfgRecvMask).AsStdStr());

      set->Add(cmd.SetReceiver("Sender"));
   }

   if (IsReceiver()) {
      dabc::Command cmd("Configure");
      cmd.Field(parSendMask).SetStr(Par(CfgSendMask).AsStdStr());
      set->Add(cmd.SetReceiver("Receiver"));

      dabc::CmdSetParameter cmd2;
      cmd2.ParName().SetStr(parSendMask);
      cmd2.ParValue().SetStr(Par(CfgSendMask).AsStr());
      set->Add(cmd2.SetReceiver("Builder"));
   }


   dabc::CmdSetParameter cmd3;
   cmd3.ParName().SetStr(CfgConnected);
   cmd3.ParValue().SetBool(true);
   set->Add(cmd3.SetReceiver(this));

   set->SubmitSet(mastercmd, SMCommandTimeout());

   DOUT3(("ApplyNodeConfig invoked"));
}

int bnet::WorkerApplication::ExecuteCommand(dabc::Command cmd)
{
   int cmd_res = dabc::cmd_true;

   if (cmd.IsName("DiscoverWorkerConfig")) {
      DiscoverNodeConfig(cmd);
   } else
   if (cmd.IsName("ApplyConfigNode")) {
      DOUT3(( "Get reconfigure recvmask = %s", cmd.GetStr(parRecvMask)));
      ApplyNodeConfig(cmd);
      cmd_res = dabc::cmd_postponed;
   } else
   if (cmd.IsName("StartFile") || cmd.IsName("StopFile")) {

      std::string filename = cmd.GetStr("FileName","");

      if (cmd.IsName("StartFile"))
         Par("Info").SetStr(dabc::format("Starting file writing: %s", filename.c_str()));
      else
         Par("Info").SetStr("Stopping file writing");

      if (IsReceiver()) {
         std::string outportname = IsFilter() ? GetFilterOutputName("Filter") : "Builder/Output";

         DOUT0(("Create file output for port %s", outportname.c_str()));

         cmd_res = cmd_bool(CreateOutFile(outportname.c_str(), filename));
         DOUT0(("Create file output done"));
      }
   } else

      cmd_res = dabc::Application::ExecuteCommand(cmd);

   return cmd_res;
}

bool bnet::WorkerApplication::IsModulesRunning()
{
   if (IsSender())
      if (dabc::mgr.FindModule("Sender").IsRunning()) return true;

   if (IsReceiver())
      if (dabc::mgr.FindModule("Receiver").IsRunning()) return true;

   return false;
}

bool bnet::WorkerApplication::CheckWorkerModules()
{
   /*
   if (!IsModulesRunning()) {
      if (dabc::mgr()->CurrentState() != dabc::Manager::stReady) {
         DOUT1(("!!!!! ******** !!!!!!!!  All main modules are stopped - we can switch to Stop state"));
         dabc::mgr()->InvokeStateTransition(dabc::Manager::stcmdDoStop);
      }
   }
   */

   return true;
}

std::string bnet::WorkerApplication::GetCombinerInputName(const char* name, int n)
{
   return dabc::format("%s/Input%d", name, n);
}

std::string bnet::WorkerApplication::GetCombinerOutputName(const char* name)
{
   return dabc::format("%s/Output", name);
}

std::string bnet::WorkerApplication::GetFilterOutputName(const char* name)
{
   return dabc::format("%s/Output", name);
}



bool bnet::WorkerApplication::CreateAppModules()
{
   Par("Info").SetStr("Create worker modules");

   DOUT1(("CreateAppModules starts dev = %s", Par(CfgNetDevice).AsStr()));

   Par(bnet::CfgReadoutPool).SetStr(CombinerModus()==0 ? bnet::ReadoutPoolName : bnet::TransportPoolName);

   dabc::mgr.CreateDevice(Par(CfgNetDevice).AsStr(), "BnetDev");

   dabc::BufferSize_t size = 0;
   dabc::unsigned num = 0;

   if (IsSender() && (CombinerModus()==0)) {
      size = Par(xmlReadoutBuffer).AsInt(1024);
      num = Par(xmlReadoutPoolSize).AsInt(0x100000) / size;
      dabc::mgr.CreateMemoryPool(Par(CfgReadoutPool).AsStr(ReadoutPoolName), size, num);
   }

   size = Par(xmlTransportBuffer).AsInt(1024);
   num = Par(xmlTransportPoolSize).AsInt(16*0x100000)/ size;

   dabc::mgr.CreateMemoryPool(bnet::TransportPoolName, size, num, 0, sizeof(bnet::SubEventNetHeader));

   if (IsReceiver()) {
      size = Par(xmlEventBuffer).AsInt(2048*16);
      num = Par(xmlEventPoolSize).AsInt(4*0x100000) / size;
      dabc::mgr.CreateMemoryPool(bnet::EventPoolName, size, num);
   }

   if (IsSender()) {
      size = Par(xmlCtrlBuffer).AsInt(2*1024);
      num = Par(xmlCtrlPoolSize).AsInt(4*0x100000) / size;

      dabc::mgr.CreateMemoryPool(bnet::ControlPoolName, size, num);
   }

   if (IsSender()) {

      if (!CreateCombiner("Combiner")) {
         EOUT(("Combiner module not created"));
         return false;
      }

      if (!dabc::mgr.CreateModule("bnet::SenderModule", "Sender", SenderThreadName)) {
         EOUT(("Sender module not created"));
         return false;
      }
   }

   if (IsReceiver()) {

      if (!dabc::mgr.CreateModule("bnet::ReceiverModule", "Receiver", ReceiverThreadName)) {
         EOUT(("Receiver module not created"));
         return false;
      }

      if (!CreateBuilder("Builder")) {
         EOUT(("EventBuilder is not created"));
         return false;
      }

      if (IsFilter() && !CreateFilter("Filter")) {
         EOUT(("EventFilter is not created"));
         return false;
      }
   }

   if (IsSender()) {

      for (int nr=0;nr<NumReadouts();nr++)
        if (!CreateReadout(GetCombinerInputName("Combiner", nr).c_str(), nr)) {
           EOUT(("Cannot create readout channel %d", nr));
           return false;
        }

      dabc::mgr()->ConnectPorts(GetCombinerOutputName("Combiner").c_str(), "Sender/Input");
   }

   if (IsReceiver()) {
      dabc::mgr()->ConnectPorts("Receiver/Output", "Builder/Input");

      std::string outportname;

      if (IsFilter()) {
         dabc::mgr()->ConnectPorts("Builder/Output", "Filter/Input");
         outportname = GetFilterOutputName("Filter");
      } else
         outportname = "Builder/Output";

      if (!CreateStorage(outportname.c_str())) {
         EOUT(("Not able to create storage for port %s", outportname.c_str()));
      }
   }

   Par(parStatus).SetStr("Ready");

   Par(CfgConnected).SetBool(false);

   return true;
}

bool bnet::WorkerApplication::BeforeAppModulesDestroyed()
{
//   DeleteCmdDef("StartFile");
//   DeleteCmdDef("StopFile");

   Par("Info").SetStr("Delete worker modules");

   return true;
}
