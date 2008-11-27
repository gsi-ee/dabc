#include "roc/ReadoutApplication.h"

#include "dabc/Parameter.h"
#include "dabc/Command.h"
#include "dabc/timing.h"
#include "dabc/CommandsSet.h"

#include "mbs/MbsTypeDefs.h"
#include "mbs/Factory.h"

#include "roc/Commands.h"

#include "SysCoreDefines.h"

const char* roc::xmlDoCalibr          = "DoCalibr";
const char* roc::xmlRocIp             = "RocIp";
const char* roc::xmlRawFile           = "RawFile";
const char* roc::xmlCalibrFile        = "CalibrFile" ;


roc::ReadoutApplication::ReadoutApplication(const char* name) :
   dabc::Application(name ? name : "RocPlugin")
{
   CreateParInt(roc::xmlDoCalibr, 1);

   CreateParInt(roc::xmlNumRocs, 3);

   for (int nr=0; nr<NumRocs(); nr++)
      CreateParStr(FORMAT(("%s%d", xmlRocIp, nr)));
   CreateParInt(dabc::xmlBufferSize, 8192);
   CreateParInt(dabc::xmlNumBuffers, 100);

   CreateParStr(mbs::xmlServerKind, mbs::ServerKindToStr(mbs::StreamServer));

   CreateParStr(xmlRawFile, "");
   CreateParStr(xmlCalibrFile, "");
   CreateParInt(mbs::xmlSizeLimit, 0);

   DOUT1(("!!!! Data server plugin created %s !!!!", GetName()));
}

int roc::ReadoutApplication::DataServerKind() const
{
   return mbs::StrToServerKind(GetParStr(mbs::xmlServerKind).c_str());
}

std::string roc::ReadoutApplication::RocIp(int nreadout) const
{
   if ((nreadout<0) || (nreadout>=NumRocs())) return "";
   return GetParStr(FORMAT(("%s%d", xmlRocIp, nreadout)));
}

bool roc::ReadoutApplication::CreateAppModules()
{
   DOUT1(("CreateAppModules starts..·"));
   bool res = false;
   dabc::Command* cmd;

   std::string devname = "ROC";

   dabc::mgr()->CreateMemoryPool(roc::xmlRocPool,
                                 GetParInt(dabc::xmlBufferSize, 8192),
                                 GetParInt(dabc::xmlNumBuffers, 100));

   if (DoTaking()) {
      res = dabc::mgr()->CreateDevice("roc::Device", devname.c_str());
      DOUT1(("Create Roc Device = %s", DBOOL(res)));
      if(!res) return false;

      cmd = new dabc::CommandCreateModule("roc::CombinerModule", "RocComb", "RocCombThrd");
      cmd->SetInt(dabc::xmlNumOutputs, 2);
      res = dabc::mgr()->Execute(cmd);
      DOUT1(("Create ROC combiner module = %s", DBOOL(res)));
      if(!res) return false;
   }

   if (DoCalibr()) {
      cmd = new dabc::CommandCreateModule("roc::CalibrationModule", "RocCalibr", "RocCalibrThrd");
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
         cmd = new dabc::CmdCreateOutputTransport("RocComb/Ports/Output1", mbs::typeLmdOutput);
         // no need to set extra size parameters - it will be taken from application
         // if (FileSizeLimit()>0) cmd->SetInt(mbs::xmlSizeLimit, FileSizeLimit());
      } else {
         cmd = new dabc::CmdCreateInputTransport("RocCalibr/Ports/Input", mbs::typeLmdInput);
         // no need to extra set of buffer size - it will be taken from module itself
         //  cmd->SetInt(dabc::xmlBufferSize, GetParInt(dabc::xmlBufferSize, 8192));
      }

      cmd->SetStr(mbs::xmlFileName, OutputFileName().c_str());

      res = dabc::mgr()->Execute(cmd);

      DOUT1(("Create raw lmd %s file res = %s", (DoTaking() ? "output" : "input"), DBOOL(res)));
      if(!res) return false;
   }

   if ((CalibrFileName().length()>0) && DoCalibr()) {

      const char* outtype = mbs::typeLmdOutput;

      if ((CalibrFileName().rfind(".root")!=std::string::npos) ||
          (CalibrFileName().rfind(".ROOT")!=std::string::npos))
              outtype = "roc::TreeOutput";

      cmd = new dabc::CmdCreateOutputTransport("RocCalibr/Ports/Output1", outtype);

      cmd->SetStr(mbs::xmlFileName, CalibrFileName().c_str());
      // no need to set extra size parameters - it will be taken from application
      // if (FileSizeLimit()>0) cmd->SetInt(mbs::xmlSizeLimit, FileSizeLimit());

      res = dabc::mgr()->Execute(cmd);
      DOUT1(("Create calibr output file res = %s", DBOOL(res)));
      if(!res) return false;
   }


   if (DataServerKind() != mbs::NoServer) {

     // need mbs device for event server:
     res = dabc::mgr()->CreateDevice("mbs::Device", "MBS");
     DOUT1(("Create Mbs Device = %s", DBOOL(res)));
     if (!res) return false;

   ///// connect module to mbs server:
      const char* portname = DoCalibr() ? "RocCalibr/Ports/Output0" : "RocComb/Ports/Output0";
      cmd = new dabc::CmdCreateTransport("MBS", portname);

      // no need to set extra parameters - they will be taken from application !!!
//      cmd->SetStr(mbs::xmlServerKind, mbs::ServerKindToStr(DataServerKind())); //mbs::StreamServer ,mbs::TransportServer
//      cmd->SetInt(dabc::xmlBufferSize, GetParInt(dabc::xmlBufferSize, 8192));

      res = dabc::mgr()->Execute(cmd);
      DOUT1(("Connected readout module output to Mbs server = %s", DBOOL(res)));
      if(!res) return false;
   }

   return true;
}

bool roc::ReadoutApplication::WriteRocRegister(int rocid, int registr, int value)
{
   dabc::Device* dev = dabc::mgr()->FindDevice("ROC");
   if (dev==0) return false;

   return dev->Execute(new roc::CommandWriteRegister(rocid, registr, value));
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

