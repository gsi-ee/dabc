 #include "MbsWorkerApplication.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"

#include "mbs/Factory.h"

#include "MbsGeneratorModule.h"
#include "MbsCombinerModule.h"
#include "MbsBuilderModule.h"
#include "MbsFilterModule.h"
#include "MbsFactory.h"

bnet::MbsWorkerApplication::MbsWorkerApplication() :
   WorkerApplication(xmlMbsWorkerClass)
{
}


bool bnet::MbsWorkerApplication::CreateReadout(const char* portname, int portnumber)
{
   std::string cfg = ReadoutPar(portnumber);

   if (IsGenerator()) {

      std::string modulename;

      dabc::formats(modulename,"Readout%d", portnumber);

      dabc::Module* m = new bnet::MbsGeneratorModule(modulename.c_str());
      dabc::mgr()->MakeThreadForModule(m, modulename.c_str());

      modulename += "/Output";

      dabc::mgr()->ConnectPorts(modulename.c_str(), portname);

      cfg = "Generator";

   } else
   if ((cfg.find("lmd") != cfg.npos) || (cfg.find("LMD") != cfg.npos)) {
      dabc::Command* cmd = new dabc::CmdCreateTransport(portname, mbs::typeLmdInput, "MbsInpThrd");
      cmd->SetStr(mbs::xmlFileName, cfg);
      if (!dabc::mgr()->Execute(cmd, 10)) return false;
   } else {

      dabc::Command* cmd = new dabc::CmdCreateTransport(portname, mbs::typeClientTransport, "MbsReadoutThrd");
      cmd->SetStr(mbs::xmlServerName, cfg);

      if (!dabc::mgr()->Execute(cmd, 10)) return false;
   }


   DOUT1(("Create input for port:%s cfg:%s done", portname, cfg.c_str()));

   return true;
}

bool bnet::MbsWorkerApplication::CreateCombiner(const char* name)
{
   return dabc::mgr()->CreateModule("bnet::MbsCombinerModule", name);
}

bool bnet::MbsWorkerApplication::CreateBuilder(const char* name)
{
   return dabc::mgr()->CreateModule("bnet::MbsBuilderModule", name);
}

bool bnet::MbsWorkerApplication::CreateFilter(const char* name)
{
   return dabc::mgr()->CreateModule("bnet::MbsFilterModule", name);
}

bool bnet::MbsWorkerApplication::CreateStorage(const char* portname)
{
   std::string outfile = GetParStr(xmlStoragePar);

   DOUT3(("!!! CreateStorage port:%s par:%s", portname, outfile.c_str()));

   if (outfile.length()==0)
      return bnet::WorkerApplication::CreateStorage(portname);

   dabc::Command* cmd = new dabc::CmdCreateTransport(portname, mbs::typeLmdOutput, "MbsIOThrd");
   cmd->SetStr(mbs::xmlFileName, outfile);

   bool res = dabc::mgr()->Execute(cmd, 5);

   DOUT1(("Create output for port %s res = %s", portname, DBOOL(res)));

   return res;
}
