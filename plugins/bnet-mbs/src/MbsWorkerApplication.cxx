 #include "bnet/MbsWorkerApplication.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"

#include "mbs/Factory.h"

#include "mbs/MbsTypeDefs.h"

#include "bnet/MbsFactory.h"

bnet::MbsWorkerApplication::MbsWorkerApplication(const char* classname) :
   WorkerApplication(classname ? classname : xmlMbsWorkerClass)
{
   CreatePar(mbs::xmlServerKind).DfltStr("");

   CreatePar(mbs::xmlSizeLimit).DfltInt(0);

   if (IsReceiver()) {
      CreateCmdDef("StartServer").AddArg("Kind", "string", false);
      CreateCmdDef("StopServer");
   }
}

int bnet::MbsWorkerApplication::ExecuteCommand(dabc::Command cmd)
{
   int cmd_res = dabc::cmd_true;

   if (cmd.IsName("StartServer")) {
      std::string kind = cmd.GetStr("Kind", "");
      if (kind.empty()) kind = mbs::ServerKindToStr(mbs::StreamServer);
      cmd_res = cmd_bool(CreateOutServer(kind));
      Par("Info").SetStr(dabc::format("Start MBS server Kind:%s res:%s", kind.c_str(), DBOOL(cmd_res)));
   } else
   if (cmd.IsName("StopServer")) {
      cmd_res = cmd_bool(CreateOutServer(""));
      Par("Info").SetStr(dabc::format("Stop MBS server res:%s", DBOOL(cmd_res)));
   } else
      cmd_res = bnet::WorkerApplication::ExecuteCommand(cmd);
   return cmd_res;
}


bool bnet::MbsWorkerApplication::CreateReadout(const char* portname, int portnumber)
{
   std::string cfg = ReadoutPar(portnumber);

   if (IsGenerator()) {

      std::string modulename = dabc::format("Readout%d", portnumber);

      dabc::mgr.CreateModule("mbs::GeneratorModule", modulename.c_str());

      dabc::mgr()->ConnectPorts((modulename + "/Output").c_str(), portname);

      cfg = "Generator";

   } else
   if ((cfg.find("lmd") != cfg.npos) || (cfg.find("LMD") != cfg.npos)) {
      dabc::CmdCreateTransport cmd(portname, mbs::typeLmdInput, "MbsInpThrd");
      cmd.SetStr(mbs::xmlFileName, cfg);
      if (!dabc::mgr.Execute(cmd, 10)) return false;
   } else {
      dabc::CmdCreateTransport cmd(portname, mbs::typeClientTransport, "MbsReadoutThrd");
      cmd.SetStr(mbs::xmlServerName, cfg);

      if (!dabc::mgr.Execute(cmd, 10)) return false;
   }

   DOUT1(("Create input for port:%s cfg:%s done", portname, cfg.c_str()));

   return true;
}

bool bnet::MbsWorkerApplication::CreateCombiner(const char* name)
{
   return dabc::mgr.CreateModule("bnet::MbsCombinerModule", name);
}

bool bnet::MbsWorkerApplication::CreateBuilder(const char* name)
{
   return dabc::mgr.CreateModule("bnet::MbsBuilderModule", name);
}

bool bnet::MbsWorkerApplication::CreateFilter(const char* name)
{
   return dabc::mgr.CreateModule("bnet::MbsFilterModule", name);
}

bool bnet::MbsWorkerApplication::CreateStorage(const char* portname)
{
   if (dabc::mgr.FindModule("Splitter").null()) {
      dabc::CmdCreateModule cmd("dabc::SplitterModule", "Splitter", "MbsOutThrd");
      cmd.SetStr(dabc::xmlPoolName, bnet::EventPoolName);
      cmd.SetInt(dabc::xmlNumOutputs, 2);
      if (!dabc::mgr.Execute(cmd)) return false;

      if (!dabc::mgr()->ConnectPorts(portname, "Splitter/Input")) return false;
   }

   return CreateOutFile(portname, Par(xmlStoragePar).AsStdStr()) &&
          CreateOutServer(Par(mbs::xmlServerKind).AsStdStr());
}

bool bnet::MbsWorkerApplication::CreateOutFile(const char* portname, const std::string& filename)
{
   if (dabc::mgr.FindModule("Splitter").null()) return false;

   if (filename.empty()) {
      if (!dabc::mgr.CreateTransport("Splitter/Output0", "")) return false;

      Par("Info").SetStr(dabc::format("Stop file writing"));

   } else {
      DOUT0(("Start Create output file = %s", filename.c_str()));

      dabc::CmdCreateTransport cmd("Splitter/Output0", mbs::typeLmdOutput, "MbsOutThrd");
      cmd.SetStr(mbs::xmlFileName, filename);
      if (!dabc::mgr.Execute(cmd, 5)) {
         EOUT(("Cannot create output file %s", filename.c_str()));
         return false;
      }
      DOUT0(("Create output file = %s", filename.c_str()));
      Par("Info").SetStr(dabc::format("Start file writing %s", filename.c_str()));
   }
   return true;
}

bool bnet::MbsWorkerApplication::CreateOutServer(const std::string& serverkind)
{
   if (dabc::mgr.FindModule("Splitter").null()) return false;

   if (serverkind.empty()) {
      if (!dabc::mgr.CreateTransport("Splitter/Output1", "")) return false;
   } else {
      DOUT0(("Start CreateOutServer"));

      dabc::CmdCreateTransport cmd("Splitter/Output1", mbs::typeServerTransport, "MbsOutServThrd");
      if ((serverkind != mbs::ServerKindToStr(mbs::TransportServer)) &&
          (serverkind != mbs::ServerKindToStr(mbs::StreamServer)))
            cmd.SetStr(mbs::xmlServerKind, mbs::ServerKindToStr(mbs::StreamServer));
         else
            cmd.SetStr(mbs::xmlServerKind, serverkind.c_str());
      cmd.Field(dabc::xmlBufferSize).SetInt(Par(xmlEventBuffer).AsInt(16834));
      if (!dabc::mgr.Execute(cmd, 5)) {
         EOUT(("Cannot create output server of kind %s", serverkind.c_str()));
         return false;
      }
      DOUT0(("Create output server of kind %s", serverkind.c_str()));
   }
   return true;
}

bool bnet::MbsWorkerApplication::BeforeAppModulesDestroyed()
{
//   DeleteCmdDef("StartServer");
//   DeleteCmdDef("StopServer");

   return bnet::WorkerApplication::BeforeAppModulesDestroyed();
}
