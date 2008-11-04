#include "roc/ReadoutApplication.h"

#include "dabc/Parameter.h"
#include "dabc/Command.h"
#include "dabc/timing.h"
#include "dabc/CommandsSet.h"

#include "mbs/MbsTypeDefs.h"
#include "mbs/Factory.h"

#include "roc/RocCalibrModule.h"

#include "SysCoreDefines.h"

roc::ReadoutApplication::ReadoutApplication(dabc::Basic* parent, const char* name) :
   dabc::Application(parent, name ? name : "RocPlugin")
{
   //new dabc::StrParameter(this,DABC_ROC_COMPAR_BOARDIP, "140.181.66.173"); // lxi010.gsi.de
   // todo: later provide more than one roc board as input

   new dabc::IntParameter(this, DABC_ROC_COMPAR_ROCSNUMBER, 1);
   for (int nr=0;nr<15;nr++)
      new dabc::StrParameter(this, FORMAT(("%s%d", DABC_ROC_PAR_ROCIP, nr)), "");

   SetParValue(FORMAT(("%s%d", DABC_ROC_PAR_ROCIP, 0)),"140.181.66.173"); // lxi010.gsi.de

   new dabc::IntParameter(this, DABC_ROC_COMPAR_BUFSIZE, 8192);
   new dabc::IntParameter(this, DABC_ROC_COMPAR_TRANSWINDOW, 30);
   new dabc::IntParameter(this, DABC_ROC_COMPAR_POOL_SIZE, 100);
   new dabc::IntParameter(this, DABC_ROC_COMPAR_QLENGTH, 10);
   new dabc::StrParameter(this, DABC_ROC_PAR_OUTFILE, "");
   new dabc::IntParameter(this, DABC_ROC_PAR_OUTFILELIMIT, 0);
   new dabc::StrParameter(this, DABC_ROC_PAR_DATASERVER, "Stream");
   new dabc::IntParameter(this, DABC_ROC_PAR_DOCALIBR, 1);
   new dabc::StrParameter(this, DABC_ROC_PAR_CALIBRFILE, "");
   new dabc::IntParameter(this, DABC_ROC_PAR_CALIBRFILELIMIT, 0);

   DOUT1(("!!!! Data server plugin created %s !!!!", GetName()));
}

int roc::ReadoutApplication::DataServerKind() const
{
   int kind = mbs::NoServer;
   std::string servertype=GetParCharStar(DABC_ROC_PAR_DATASERVER);
   if(servertype.find("Stream")!=std::string::npos)
      kind=mbs::StreamServer;
   else
   if(servertype.find("Transport")!=std::string::npos)
      kind=mbs::TransportServer;
   return kind;
}

const char* roc::ReadoutApplication::RocIp(int nreadout) const
{
   if ((nreadout<0) || (nreadout>=NumRocs())) return 0;
   return GetParCharStar(FORMAT(("%s%d", DABC_ROC_PAR_ROCIP, nreadout)));
}



bool roc::ReadoutApplication::CreateAppModules()
{
   DOUT1(("CreateAppModules starts..·"));
   bool res=false;
   dabc::Command* cmd;

   std::string devname = "ROC"; // parametrize this later?
   fFullDeviceName = "Devices/"+devname;

   GetManager()->CreateMemoryPool(DABC_ROC_POOLNAME, BufferSize(), NumPoolBuffers());

   if (DoTaking()) {
      if (!GetManager()->FindDevice(devname.c_str())) {
         res = GetManager()->CreateDevice("RocDevice",devname.c_str());
         DOUT1(("Create Roc Device = %s", DBOOL(res)));
         if(!res) return false;
      }


      cmd = new dabc::CommandCreateModule("RocCombinerModule","RocComb");
      cmd->SetInt(DABC_ROC_COMPAR_BUFSIZE, BufferSize());
      cmd->SetInt(DABC_ROC_COMPAR_QLENGTH, PortQueueLength());
      cmd->SetInt(DABC_ROC_COMPAR_ROCSNUMBER, NumRocs());
      cmd->SetInt(DABC_ROC_COMPAR_NUMOUTPUTS, 2);
      res = GetManager()->CreateModule("RocCombinerModule","RocComb", "RocCombThrd", cmd);
      DOUT1(("Create ROC combiner module = %s", DBOOL(res)));
      if(!res) return false;
   }

   if (DoCalibr()) {
      cmd = new dabc::CommandCreateModule("RocCalibrModule","RocCalibr");
      cmd->SetInt(DABC_ROC_COMPAR_ROCSNUMBER, NumRocs());
      cmd->SetInt(DABC_ROC_COMPAR_BUFSIZE, BufferSize());
      cmd->SetInt(DABC_ROC_COMPAR_QLENGTH, PortQueueLength());
      cmd->SetInt(DABC_ROC_COMPAR_NUMOUTPUTS, 2);
      res = GetManager()->CreateModule("RocCalibrModule","RocCalibr", "RocCalibrThrd" ,cmd);
      DOUT1(("Create ROC calibration module = %s", DBOOL(res)));
      if(!res) return false;

      if (DoTaking()) {
         res = GetManager()->ConnectPorts("RocComb/Ports/Output0", "RocCalibr/Ports/Input");
         DOUT1(("Connect readout and calibr modules = %s", DBOOL(res)));
         if(!res) return false;
      }
   }

   res = GetManager()->CreateMemoryPools();
   DOUT1(("Create memory pools result=%s", DBOOL(res)));
   if(!res) return false;

   if (DoTaking()) {

   //// connect module to ROC device, one transport for each board:

      for(int t=0; t<NumRocs(); t++) {
         cmd= new dabc::CmdCreateTransport(); // container for additional bopard parameters
         cmd->SetStr(DABC_ROC_COMPAR_BOARDIP, RocIp(t));
         cmd->SetInt(DABC_ROC_COMPAR_BUFSIZE, BufferSize());
         cmd->SetInt(DABC_ROC_COMPAR_TRANSWINDOW, TransWindow());
         res=GetManager()->CreateTransport(fFullDeviceName.c_str(),FORMAT(("RocComb/Ports/Input%d", t)),cmd);
         DOUT1(("Connected readout module input %d  to ROC board %s, result %s",t, RocIp(t), DBOOL(res)));
         if(!res) return false;
      }

      // configure loop over all connected rocs:
       for(int t=0; t<NumRocs();++t) {
          res=ConfigureRoc(t);
          DOUT1(("Configure ROC %d returns %s", t, DBOOL(res)));
          if(!res) return false;
      }
   }

   if (OutputFileName().length()>0) {
      cmd = new dabc::CmdCreateDataTransport();
      if (OutFileLimit()>0) {
         cmd->SetInt("SizeLimit", OutFileLimit());
         cmd->SetInt("NumMulti", -1); // allow to create multiple files
      }

      if (DoTaking())
         res = GetManager()->CreateDataOutputTransport("RocComb/Ports/Output1", "OutputThrd", mbs::Factory::LmdFileType(), OutputFileName().c_str(), cmd);
      else
         res = GetManager()->CreateDataInputTransport("RocCalibr/Ports/Input", "InputThrd", mbs::Factory::LmdFileType(), OutputFileName().c_str(), cmd);

      DOUT1(("Create raw lmd %s file res = %s", (DoTaking() ? "output" : "input"), DBOOL(res)));
      if(!res) return false;
   }

   if ((CalibrFileName().length()>0) && DoCalibr()) {
      cmd = new dabc::CmdCreateDataTransport();

      const char* outtype = mbs::Factory::LmdFileType();

      if ((CalibrFileName().rfind(".root")!=dabc::String::npos) ||
          (CalibrFileName().rfind(".ROOT")!=dabc::String::npos))
             outtype = "RocTreeOutput";

      if (CalibrFileLimit()>0) {
         cmd->SetInt("SizeLimit", CalibrFileLimit());
         cmd->SetInt("NumMulti", -1); // allow to create multiple files
      }

      res = GetManager()->CreateDataOutputTransport("RocCalibr/Ports/Output1", "OutputThrd", outtype, CalibrFileName().c_str(), cmd);
      DOUT1(("Create calibr output file res = %s", DBOOL(res)));
      if(!res) return false;
   }


   if (DataServerKind() != mbs::NoServer) {

     // need mbs device for event server:
      if (!GetManager()->FindDevice("MBS")) {
         res=GetManager()->CreateDevice("MbsDevice", "MBS");
         DOUT1(("Create Mbs Device = %s", DBOOL(res)));
      }

   ///// connect module to mbs server:
      cmd = new dabc::CmdCreateTransport();
      cmd->SetInt("ServerKind", DataServerKind()); //mbs::StreamServer ,mbs::TransportServer
      //      cmd->SetInt("PortMin", 6002);
      //      cmd->SetInt("PortMax", 7000);
      cmd->SetUInt("BufferSize", BufferSize());

      const char* portname = DoCalibr() ? "RocCalibr/Ports/Output0" : "RocComb/Ports/Output0";
      res=GetManager()->CreateTransport("MBS", portname, cmd);
      DOUT1(("Connected readout module output to Mbs server = %s", DBOOL(res)));
      if(!res) return false;
   }

   return true;
}

bool roc::ReadoutApplication::WriteRocRegister(int rocid, int registr, int value)
{
   dabc::Command* cmd = new roc::CommandWriteRegister(rocid, registr, value);
   return GetManager()->Execute(GetManager()->LocalCmd(cmd, fFullDeviceName.c_str()), 1);
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

bool roc::ReadoutApplication::PluginWorking()
{
   return roc::RocCalibrModule::WorkingFlag();
}


bool roc::ReadoutApplication::IsModulesRunning()
{
   if (DoTaking())
      if (!dabc::mgr()->IsModuleRunning("RocComb")) return false;

   if (DoCalibr())
      if (!dabc::mgr()->IsModuleRunning("RocCalibr")) return false;

   return true;
}

