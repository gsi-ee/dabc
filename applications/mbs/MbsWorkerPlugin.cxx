 #include "MbsWorkerPlugin.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"

#include "mbs/MbsFactory.h"

#include "MbsGeneratorModule.h"
#include "MbsCombinerModule.h"
#include "MbsBuilderModule.h"
#include "MbsFilterModule.h"

bool bnet::MbsWorkerPlugin::CreateReadout(const char* portname, int portnumber)
{
   if (IsGenerator()) {

      dabc::String modulename; 
      
      dabc::formats(modulename,"Readout%d", portnumber);
       
      dabc::Module* m = new bnet::MbsGeneratorModule(GetManager(), modulename.c_str(), this);
      GetManager()->MakeThreadForModule(m, modulename.c_str()); 
      
      GetManager()->CreateMemoryPools();
      
      modulename += "/Ports/Output";
      
      GetManager()->ConnectPorts(modulename.c_str(), portname);
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
      
      bool res = GetManager()->Execute(cmd, 10);
      
      DOUT3(("Create input for port %s res = %s", portname, DBOOL(res)));
      
      if (!res) return false;
   }

   return true;
}

dabc::Module* bnet::MbsWorkerPlugin::CreateCombiner(const char* name) 
{
   dabc::Module* m = new bnet::MbsCombinerModule(GetManager(), name, this);
   GetManager()->MakeThreadForModule(m, name); 
   return m;
}

dabc::Module* bnet::MbsWorkerPlugin::CreateBuilder(const char* name)
{
   dabc::Module* m = new bnet::MbsBuilderModule(GetManager(), name, this);
   GetManager()->MakeThreadForModule(m, name); 
   return m;
}

dabc::Module* bnet::MbsWorkerPlugin::CreateFilter(const char* name)
{
   dabc::Module* m = new bnet::MbsFilterModule(GetManager(), name, this);
   GetManager()->MakeThreadForModule(m, name);
   return m;
}

bool bnet::MbsWorkerPlugin::CreateStorage(const char* portname)
{
   const char* outfile = StoragePar();
   
   DOUT3(("!!! CreateStorage port:%s par:%s", portname, outfile));
   
   if ((outfile==0) || (strlen(outfile)==0))
      return bnet::WorkerPlugin::CreateStorage(portname);

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

   bool res = GetManager()->Execute(cmd, 5);
      
   DOUT1(("Create output for port %s res = %s", portname, DBOOL(res)));
      
   return res;
}

void bnet::MbsWorkerPlugin::SetMbsFilePars(const char* filebase)
{
   SetParValue("IsGenerator", 0);

   SetParValue("IsFilter", 0);
   
   int nodeid = GetManager()->NodeId();
   
   if (IsReceiver()) {
      int recvid = 0;
       
      if (bnet::NetBidirectional) 
         recvid = nodeid - 1;
      else
         recvid = (nodeid-1) / 2;

      SetParValue("StoragePar", FORMAT(("OutType:%s; OutName:%s_out_%d.lmd;", mbs::MbsFactory::NewFileType(), filebase, recvid)));
//      SetParValue("StoragePar", FORMAT(("OutType:%s; OutName:%s_out_%d; SizeLimit:10000000; NumMulti:-1;", mbs::MbsFactory::NewFileType(), filebase, recvid)));
   }

   if (IsSender()) {
      int senderid = 0;
      if (bnet::NetBidirectional) 
         senderid = nodeid - 1;
      else
         senderid = nodeid;
       
      for (int nr=0;nr<NumReadouts();nr++) {
         dabc::String cfgstr, parname;
      
         dabc::formats(cfgstr, "InpType:%s; InpName:%s_inp_%d_%d*.lmd;", mbs::MbsFactory::NewFileType(), filebase, senderid, nr);
         
         dabc::formats(parname, "Input%dCfg", nr);
         
         SetParValue(parname.c_str(), cfgstr.c_str());
         
         DOUT3(("Set filepar value %s %s : res:%s", parname.c_str(), cfgstr.c_str(), ReadoutPar(nr))); 
      }
   }
}

void bnet::MbsWorkerPlugin::SetMbsTransportPars()
{
   SetParValue("IsGenerator", 0);
   
   SetParValue("IsFilter", 0);

   SetParValue("NumReadouts", 1);
   
   if ((GetManager()->NodeId()==1) || (GetManager()->NodeId()==2)) {

      SetParValue("IsSender", 1);
      SetParValue("IsReceiver", 0);

      const char* server_name = (GetManager()->NodeId()==1) ? "x86g-4" : "x86-7"; 

      dabc::String cfgstr;
      
      dabc::formats(cfgstr, "InpType:%s; InpName:%s.gsi.de; Port:6000;", mbs::MbsFactory::NewTransportType(), server_name);

      SetParValue("Input0Cfg", cfgstr.c_str());

      SetParValue("IsGenerator", 1);
       
   } else {
      SetParValue("IsSender", 0);
      SetParValue("IsReceiver", 1);

      SetParValue("StoragePar", FORMAT(("InpType:%s; OutName:/tmp/test_out.lmd;", mbs::MbsFactory::NewFileType())));
   }
}

void bnet::MbsWorkerPlugin::SetMbsGeneratorsPars()
{
   SetParValue("IsGenerator", 0);
   
   SetParValue("IsFilter", 0);

   SetParValue("NumReadouts", 1);
   
   if ((GetManager()->NodeId()==1) || (GetManager()->NodeId()==2)) {

      SetParValue("IsSender", 1);
      SetParValue("IsReceiver", 0);

      const char* server_name = (GetManager()->NodeId()==1) ? "master" : "node01";
      
      dabc::String cfgstr;
      
      dabc::formats(cfgstr, "InpType:%s; InpName:%s; Port:8000;", mbs::MbsFactory::NewTransportType(), server_name);

      SetParValue("Input0Cfg", cfgstr.c_str());
   } else {
      SetParValue("IsSender", 0);
      SetParValue("IsReceiver", 1);

//      SetParValue("StoragePar", FORMAT(("InpType:%s; OutName:/tmp/test_out.lmd;", mbs::MbsFactory::NewFileType())));
   }
}


// _____________________________________________________________________

void InitUserPlugin(dabc::Manager* mgr)
{
   // add MBS basic functionality to Manager 
//   new mbs::MbsFactory(mgr);

   // create user worker plugin
   bool is_all_to_all = bnet::NetBidirectional;
   int CombinerModus = 0;
   int NumReadouts = 4;

   bnet::MbsWorkerPlugin* worker = new bnet::MbsWorkerPlugin(mgr);

   worker->SetPars(is_all_to_all, NumReadouts, CombinerModus);
   
   if (bnet::UseFileSource)
//      worker->SetMbsTransportPars();
//      worker->SetMbsGeneratorsPars();
      worker->SetMbsFilePars("/tmp/genmbs");
}
