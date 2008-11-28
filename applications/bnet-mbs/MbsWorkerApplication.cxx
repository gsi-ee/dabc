 #include "MbsWorkerApplication.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"

#include "mbs/Factory.h"

#include "MbsGeneratorModule.h"
#include "MbsCombinerModule.h"
#include "MbsBuilderModule.h"
#include "MbsFilterModule.h"

bnet::MbsWorkerApplication::MbsWorkerApplication(const char* name) :
   WorkerApplication(name)
{
   bool is_all_to_all = bnet::NetBidirectional;
   int CombinerModus = 0;
   int NumReadouts = 4;

   SetPars(is_all_to_all, NumReadouts, CombinerModus);

   if (bnet::UseFileSource)
//      SetMbsTransportPars();
//      SetMbsGeneratorsPars();
      SetMbsFilePars("/tmp/genmbs");
}


bool bnet::MbsWorkerApplication::CreateReadout(const char* portname, int portnumber)
{
   std::string cfg = ReadoutPar(portnumber);

   if (IsGenerator()) {

      std::string modulename;

      dabc::formats(modulename,"Readout%d", portnumber);

      dabc::Module* m = new bnet::MbsGeneratorModule(modulename.c_str(), this);
      dabc::mgr()->MakeThreadForModule(m, modulename.c_str());

      dabc::mgr()->CreateMemoryPools();

      modulename += "/Ports/Output";

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

dabc::Module* bnet::MbsWorkerApplication::CreateCombiner(const char* name)
{
   dabc::Module* m = new bnet::MbsCombinerModule(name, this);
   dabc::mgr()->MakeThreadForModule(m, name);
   return m;
}

dabc::Module* bnet::MbsWorkerApplication::CreateBuilder(const char* name)
{
   dabc::Module* m = new bnet::MbsBuilderModule(name, this);
   dabc::mgr()->MakeThreadForModule(m, name);
   return m;
}

dabc::Module* bnet::MbsWorkerApplication::CreateFilter(const char* name)
{
   dabc::Module* m = new bnet::MbsFilterModule(name, this);
   dabc::mgr()->MakeThreadForModule(m, name);
   return m;
}

bool bnet::MbsWorkerApplication::CreateStorage(const char* portname)
{
   std::string outfile = StoragePar();

   DOUT3(("!!! CreateStorage port:%s par:%s", portname, outfile.c_str()));

   if (outfile.length()==0)
      return bnet::WorkerApplication::CreateStorage(portname);

   dabc::Command* cmd = new dabc::CmdCreateTransport(portname, mbs::typeLmdOutput, "MbsIOThrd");
   cmd->SetStr(mbs::xmlFileName, outfile);

   bool res = dabc::mgr()->Execute(cmd, 5);

   DOUT1(("Create output for port %s res = %s", portname, DBOOL(res)));

   return res;
}

void bnet::MbsWorkerApplication::SetMbsFilePars(const char* filebase)
{
   SetParInt("IsGenerator", 0);

   SetParInt("IsFilter", 0);

   int nodeid = dabc::mgr()->NodeId();

   if (IsReceiver()) {
      int recvid = 0;

      if (bnet::NetBidirectional)
         recvid = nodeid - 1;
      else
         recvid = (nodeid-1) / 2;

      SetParStr("StoragePar", FORMAT(("%s_out_%d.lmd;", filebase, recvid)));
   }

   if (IsSender()) {
      int senderid = 0;
      if (bnet::NetBidirectional)
         senderid = nodeid - 1;
      else
         senderid = nodeid;

      for (int nr=0;nr<NumReadouts();nr++) {
         std::string cfgstr, parname;

         dabc::formats(cfgstr, "%s_inp_%d_%d*.lmd;", filebase, senderid, nr);

         dabc::formats(parname, "Input%dCfg", nr);

         SetParStr(parname.c_str(), cfgstr);

         DOUT3(("Set filepar value %s %s : res:%s", parname.c_str(), cfgstr.c_str(), ReadoutPar(nr).c_str()));
      }
   }
}

void bnet::MbsWorkerApplication::SetMbsTransportPars()
{
   SetParInt("IsGenerator", 0);

   SetParInt("IsFilter", 0);

   SetParInt("NumReadouts", 1);

   if ((dabc::mgr()->NodeId()==1) || (dabc::mgr()->NodeId()==2)) {

      SetParInt("IsSender", 1);
      SetParInt("IsReceiver", 0);

      const char* server_name = (dabc::mgr()->NodeId()==1) ? "x86g-4" : "x86-7";

      SetParStr("Input0Cfg", server_name);

      SetParInt("IsGenerator", 1);

   } else {
      SetParInt("IsSender", 0);
      SetParInt("IsReceiver", 1);

      SetParStr("StoragePar", dabc::format("/tmp/test_out.lmd;"));
   }
}

void bnet::MbsWorkerApplication::SetMbsGeneratorsPars()
{
   SetParInt("IsGenerator", 0);

   SetParInt("IsFilter", 0);

   SetParInt("NumReadouts", 1);

   if ((dabc::mgr()->NodeId()==1) || (dabc::mgr()->NodeId()==2)) {

      SetParInt("IsSender", 1);
      SetParInt("IsReceiver", 0);

      const char* server_name = (dabc::mgr()->NodeId()==1) ? "master" : "node01";

      SetParStr("Input0Cfg", server_name);
   } else {
      SetParInt("IsSender", 0);
      SetParInt("IsReceiver", 1);

      SetParStr("StoragePar", dabc::format("/tmp/test_out.lmd;"));
   }
}
