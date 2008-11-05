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
   if (IsGenerator()) {

      dabc::String modulename;

      dabc::formats(modulename,"Readout%d", portnumber);

      dabc::Module* m = new bnet::MbsGeneratorModule(modulename.c_str(), this);
      dabc::mgr()->MakeThreadForModule(m, modulename.c_str());

      dabc::mgr()->CreateMemoryPools();

      modulename += "/Ports/Output";

      dabc::mgr()->ConnectPorts(modulename.c_str(), portname);
   } else {

      dabc::Command* cmd = new dabc::CmdCreateDataTransport;

      DOUT1(("!!!!! Create readout %d with par %s", portnumber, ReadoutPar(portnumber)));

      if (!cmd->ReadFromString(ReadoutPar(portnumber), true)) {
         EOUT(("Cannot decode command parameters for input %d::: %s", portnumber, ReadoutPar(portnumber)));
         dabc::Command::Finalise(cmd);
         return false;
      }

      if (cmd->GetPar("InpName")==0) {
         EOUT(("Important parameter 'InpName' missed in config for input %d", portnumber));
         dabc::Command::Finalise(cmd);
         return false;
      }

      if (cmd->GetPar("InpType")==0) {
         EOUT(("Important parameter 'InpType' missed in config for input %d", portnumber));
         dabc::Command::Finalise(cmd);
         return false;
      }

      dabc::CmdCreateDataTransport::SetArgs(cmd, portname, "MbsIOThrd");

      bool res = dabc::mgr()->Execute(cmd, 10);

      DOUT3(("Create input for port %s res = %s", portname, DBOOL(res)));

      if (!res) return false;
   }

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
   const char* outfile = StoragePar();

   DOUT3(("!!! CreateStorage port:%s par:%s", portname, outfile));

   if ((outfile==0) || (strlen(outfile)==0))
      return bnet::WorkerApplication::CreateStorage(portname);

   dabc::Command* cmd = new dabc::CmdCreateDataTransport;

   if (!cmd->ReadFromString(outfile, true)) {
      EOUT(("Cannot decode command parameters for output %s", outfile));
      dabc::Command::Finalise(cmd);
      return false;
   }

   if (cmd->GetPar("OutName")==0) {
      EOUT(("Important parameter 'OutName' missed in config for output"));
      dabc::Command::Finalise(cmd);
      return false;
   }

   if (cmd->GetPar("OutType")==0) {
      EOUT(("Important parameter 'OutType' missed in config for output"));
      dabc::Command::Finalise(cmd);
      return false;
   }

   dabc::CmdCreateDataTransport::SetArgs(cmd, portname, "MbsIOThrd");

   bool res = dabc::mgr()->Execute(cmd, 5);

   DOUT1(("Create output for port %s res = %s", portname, DBOOL(res)));

   return res;
}

void bnet::MbsWorkerApplication::SetMbsFilePars(const char* filebase)
{
   SetParValue("IsGenerator", 0);

   SetParValue("IsFilter", 0);

   int nodeid = dabc::mgr()->NodeId();

   if (IsReceiver()) {
      int recvid = 0;

      if (bnet::NetBidirectional)
         recvid = nodeid - 1;
      else
         recvid = (nodeid-1) / 2;

      SetParValue("StoragePar", FORMAT(("OutType:%s; OutName:%s_out_%d.lmd;", mbs::Factory::NewFileType(), filebase, recvid)));
//      SetParValue("StoragePar", FORMAT(("OutType:%s; OutName:%s_out_%d; SizeLimit:10000000; NumMulti:-1;", mbs::Factory::NewFileType(), filebase, recvid)));
   }

   if (IsSender()) {
      int senderid = 0;
      if (bnet::NetBidirectional)
         senderid = nodeid - 1;
      else
         senderid = nodeid;

      for (int nr=0;nr<NumReadouts();nr++) {
         dabc::String cfgstr, parname;

         dabc::formats(cfgstr, "InpType:%s; InpName:%s_inp_%d_%d*.lmd;", mbs::Factory::NewFileType(), filebase, senderid, nr);

         dabc::formats(parname, "Input%dCfg", nr);

         SetParValue(parname.c_str(), cfgstr.c_str());

         DOUT3(("Set filepar value %s %s : res:%s", parname.c_str(), cfgstr.c_str(), ReadoutPar(nr)));
      }
   }
}

void bnet::MbsWorkerApplication::SetMbsTransportPars()
{
   SetParValue("IsGenerator", 0);

   SetParValue("IsFilter", 0);

   SetParValue("NumReadouts", 1);

   if ((dabc::mgr()->NodeId()==1) || (dabc::mgr()->NodeId()==2)) {

      SetParValue("IsSender", 1);
      SetParValue("IsReceiver", 0);

      const char* server_name = (dabc::mgr()->NodeId()==1) ? "x86g-4" : "x86-7";

      dabc::String cfgstr;

      dabc::formats(cfgstr, "InpType:%s; InpName:%s.gsi.de; Port:6000;", mbs::Factory::NewTransportType(), server_name);

      SetParValue("Input0Cfg", cfgstr.c_str());

      SetParValue("IsGenerator", 1);

   } else {
      SetParValue("IsSender", 0);
      SetParValue("IsReceiver", 1);

      SetParValue("StoragePar", FORMAT(("InpType:%s; OutName:/tmp/test_out.lmd;", mbs::Factory::NewFileType())));
   }
}

void bnet::MbsWorkerApplication::SetMbsGeneratorsPars()
{
   SetParValue("IsGenerator", 0);

   SetParValue("IsFilter", 0);

   SetParValue("NumReadouts", 1);

   if ((dabc::mgr()->NodeId()==1) || (dabc::mgr()->NodeId()==2)) {

      SetParValue("IsSender", 1);
      SetParValue("IsReceiver", 0);

      const char* server_name = (dabc::mgr()->NodeId()==1) ? "master" : "node01";

      dabc::String cfgstr;

      dabc::formats(cfgstr, "InpType:%s; InpName:%s; Port:8000;", mbs::Factory::NewTransportType(), server_name);

      SetParValue("Input0Cfg", cfgstr.c_str());
   } else {
      SetParValue("IsSender", 0);
      SetParValue("IsReceiver", 1);

//      SetParValue("StoragePar", FORMAT(("InpType:%s; OutName:/tmp/test_out.lmd;", mbs::Factory::NewFileType())));
   }
}
