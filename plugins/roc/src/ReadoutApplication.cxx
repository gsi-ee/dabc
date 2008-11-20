#include "roc/ReadoutApplication.h"

#include "dabc/Parameter.h"
#include "dabc/Command.h"
#include "dabc/timing.h"
#include "dabc/CommandsSet.h"

#include "mbs/MbsTypeDefs.h"
#include "mbs/Factory.h"

#include "roc/RocCalibrModule.h"

#include "roc/RocCommands.h"

#include "SysCoreDefines.h"

const char* roc::xmlDoCalibr          = "DoCalibr";
const char* roc::xmlRocIp             = "RocIp";
const char* roc::xmlMbsServerKind     = "MbsServerKind";
const char* roc::xmlRawFile           = "RawFile";
const char* roc::xmlRawFileLimit      = "RawFileLimit";
const char* roc::xmlCalibrFile        = "CalibrFile" ;
const char* roc::xmlCalibrFileLimit   = "CalibrFileLimit";


roc::ReadoutApplication::ReadoutApplication(const char* name) :
   dabc::Application(name ? name : "RocPlugin")
{
   CreateParInt(roc::xmlDoCalibr, 1);

   CreateParInt(roc::xmlNumRocs, 3);

   for (int nr=0; nr<NumRocs(); nr++)
      CreateParStr(FORMAT(("%s%d", xmlRocIp, nr)));
   CreateParInt(dabc::xmlBufferSize, 8192);
   CreateParInt(dabc::xmlNumBuffers, 100);

   CreateParStr(xmlMbsServerKind, "Stream");

   CreateParStr(xmlRawFile, "");
   CreateParInt(xmlRawFileLimit, 0);
   CreateParStr(xmlCalibrFile, "");
   CreateParInt(xmlCalibrFileLimit, 0);

   DOUT1(("!!!! Data server plugin created %s !!!!", GetName()));
}

int roc::ReadoutApplication::DataServerKind() const
{
   int kind = mbs::NoServer;
   std::string servertype = GetParStr(xmlMbsServerKind);
   if(servertype.find("Stream")!=std::string::npos)
      kind=mbs::StreamServer;
   else
   if(servertype.find("Transport")!=std::string::npos)
      kind=mbs::TransportServer;
   return kind;
}

std::string roc::ReadoutApplication::RocIp(int nreadout) const
{
   if ((nreadout<0) || (nreadout>=NumRocs())) return "";
   return GetParStr(FORMAT(("%s%d", xmlRocIp, nreadout)));
}

bool roc::ReadoutApplication::CreateAppModules()
{
   DOUT1(("CreateAppModules starts..·"));
   bool res=false;
   dabc::Command* cmd;

   std::string devname = "ROC"; // parametrize this later?
   fFullDeviceName = "Devices/"+devname;

   dabc::mgr()->CreateMemoryPool(roc::xmlRocPool,
                                 GetParInt(dabc::xmlBufferSize, 8192),
                                 GetParInt(dabc::xmlNumBuffers, 100));

   if (DoTaking()) {
      res = dabc::mgr()->CreateDevice("RocDevice", devname.c_str());
      DOUT1(("Create Roc Device = %s", DBOOL(res)));
      if(!res) return false;

      cmd = new dabc::CommandCreateModule("RocCombinerModule","RocComb", "RocCombThrd");
      cmd->SetInt(dabc::xmlNumOutputs, 2);
      res = dabc::mgr()->Execute(cmd);
      DOUT1(("Create ROC combiner module = %s", DBOOL(res)));
      if(!res) return false;
   }

   if (DoCalibr()) {
      cmd = new dabc::CommandCreateModule("RocCalibrModule","RocCalibr", "RocCalibrThrd");
      cmd->SetInt(dabc::xmlNumOutputs, 2);
      res = dabc::mgr()->Execute(cmd);
      DOUT1(("Create ROC calibration module = %s", DBOOL(res)));
      if(!res) return false;

      if (DoTaking()) {
         res = dabc::mgr()->ConnectPorts("RocComb/Ports/Output0", "RocCalibr/Ports/Input");
         DOUT1(("Connect readout and calibr modules = %s", DBOOL(res)));
         if(!res) return false;
      }
   }

   res = dabc::mgr()->CreateMemoryPools();
   DOUT1(("Create memory pools result=%s", DBOOL(res)));
   if(!res) return false;

   if (DoTaking()) {

   //// connect module to ROC device, one transport for each board:

      for(int t=0; t<NumRocs(); t++) {
         cmd = new dabc::CmdCreateTransport(devname.c_str(), FORMAT(("RocComb/Ports/Input%d", t))); // container for additional board parameters
         cmd->SetStr(roc::xmlBoardIP, RocIp(t));
         res = dabc::mgr()->Execute(cmd);
         DOUT1(("Connected readout module input %d  to ROC board %s, result %s",t, RocIp(t).c_str(), DBOOL(res)));
         if(!res) return false;
      }

      // configure loop over all connected rocs:
       for(int t=0; t<NumRocs();++t) {
          res = ConfigureRoc(t);
          DOUT1(("Configure ROC %d returns %s", t, DBOOL(res)));
          if(!res) return false;
      }
   }

   if (OutputFileName().length()>0) {
      cmd = 0;
      if (DoTaking()) {
         cmd = new dabc::CmdCreateDataTransport("RocComb/Ports/Output1", "OutputThrd");
         dabc::CmdCreateDataTransport::SetArgsOut(cmd, mbs::Factory::LmdFileType(), OutputFileName().c_str());
      } else {
         cmd = new dabc::CmdCreateDataTransport("RocCalibr/Ports/Input", "InputThrd");
         dabc::CmdCreateDataTransport::SetArgsInp(cmd, mbs::Factory::LmdFileType(), OutputFileName().c_str());
      }

      if (OutFileLimit()>0) {
         cmd->SetInt("SizeLimit", OutFileLimit());
         cmd->SetInt("NumMulti", -1); // allow to create multiple files
      }

      res = dabc::mgr()->Execute(cmd);

      DOUT1(("Create raw lmd %s file res = %s", (DoTaking() ? "output" : "input"), DBOOL(res)));
      if(!res) return false;
   }

   if ((CalibrFileName().length()>0) && DoCalibr()) {
      const char* outtype = mbs::Factory::LmdFileType();

      if ((CalibrFileName().rfind(".root")!=std::string::npos) ||
          (CalibrFileName().rfind(".ROOT")!=std::string::npos))
             outtype = "RocTreeOutput";

      cmd = new dabc::CmdCreateDataTransport("RocCalibr/Ports/Output1", "OutputThrd");
      dabc::CmdCreateDataTransport::SetArgsOut(cmd, outtype, CalibrFileName().c_str());

      if (CalibrFileLimit()>0) {
         cmd->SetInt("SizeLimit", CalibrFileLimit());
         cmd->SetInt("NumMulti", -1); // allow to create multiple files
      }

      res = dabc::mgr()->Execute(cmd);
      DOUT1(("Create calibr output file res = %s", DBOOL(res)));
      if(!res) return false;
   }


   if (DataServerKind() != mbs::NoServer) {

     // need mbs device for event server:
      if (!dabc::mgr()->FindDevice("MBS")) {
         res=dabc::mgr()->CreateDevice("MbsDevice", "MBS");
         DOUT1(("Create Mbs Device = %s", DBOOL(res)));
      }

   ///// connect module to mbs server:
      const char* portname = DoCalibr() ? "RocCalibr/Ports/Output0" : "RocComb/Ports/Output0";
      cmd = new dabc::CmdCreateTransport("MBS", portname);
      cmd->SetInt("ServerKind", DataServerKind()); //mbs::StreamServer ,mbs::TransportServer
      //      cmd->SetInt("PortMin", 6002);
      //      cmd->SetInt("PortMax", 7000);
      cmd->SetUInt("BufferSize", GetParInt(dabc::xmlBufferSize, 8192));

      res = dabc::mgr()->Execute(cmd);
      DOUT1(("Connected readout module output to Mbs server = %s", DBOOL(res)));
      if(!res) return false;
   }

   return true;
}

bool roc::ReadoutApplication::WriteRocRegister(int rocid, int registr, int value)
{
   dabc::Command* cmd = new roc::CommandWriteRegister(rocid, registr, value);
   return dabc::mgr()->Execute(dabc::mgr()->LocalCmd(cmd, fFullDeviceName.c_str()), 1);
}


bool roc::ReadoutApplication::ConfigureRoc(int index)
{
   bool res = true;


   res = res && WriteRocRegister(index, ROC_NX_SELECT, 0);
   res = res && WriteRocRegister(index, ROC_NX_ACTIVE, 0);
//   if (index==0)
      res = res && WriteRocRegister(index, ROC_SYNC_M_SCALEDOWN, 1);
   res = res && WriteRocRegister(index, ROC_AUX_ACTIVE, 3);


/*
   res = res && WriteRocRegister(index, ROC_ACTIVATE_LOW_LEVEL, 1);
   res = res && WriteRocRegister(index, ROC_DO_TESTSETUP,1);
   res = res && WriteRocRegister(index, ROC_ACTIVATE_LOW_LEVEL,0);

   res = res && WriteRocRegister(index, ROC_NX_REGISTER_BASE + 0,255);
   res = res && WriteRocRegister(index, ROC_NX_REGISTER_BASE + 1,255);
   res = res && WriteRocRegister(index, ROC_NX_REGISTER_BASE + 2,0);
   res = res && WriteRocRegister(index, ROC_NX_REGISTER_BASE + 18,35);
   res = res && WriteRocRegister(index, ROC_NX_REGISTER_BASE + 32,1);

   res = res && WriteRocRegister(index, ROC_FIFO_RESET,1);
   res = res && WriteRocRegister(index, ROC_BUFFER_FLUSH_TIMER,1000);
*/

   return res;
}

bool roc::ReadoutApplication::IsModulesRunning()
{
   if (DoTaking())
      if (!dabc::mgr()->IsModuleRunning("RocComb")) return false;

   if (DoCalibr())
      if (!dabc::mgr()->IsModuleRunning("RocCalibr")) return false;

   return true;
}

