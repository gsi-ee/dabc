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
#include "roc/ReadoutApplication.h"

#include "dabc/Parameter.h"
#include "dabc/Command.h"
#include "dabc/timing.h"
#include "dabc/CommandsSet.h"

#include "mbs/MbsTypeDefs.h"
#include "mbs/Factory.h"

#include "roc/Defines.h"


const char* roc::xmlDoCalibr          = "DoCalibr";
const char* roc::xmlRocIp             = "RocIp";
const char* roc::xmlRawFile           = "RawFile";
const char* roc::xmlCalibrFile        = "CalibrFile" ;
const char* roc::xmlReadoutAppClass   = "roc::Readout";


roc::ReadoutApplication::ReadoutApplication() :
   dabc::Application(roc::xmlReadoutAppClass)
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
   DOUT1(("CreateAppModules starts..�"));
   bool res = false;
   dabc::Command* cmd;

   fDevName = "ROC";

   dabc::mgr()->CreateMemoryPool(roc::xmlRocPool,
                                 GetParInt(dabc::xmlBufferSize, 8192),
                                 GetParInt(dabc::xmlNumBuffers, 100));

   if (DoTaking()) {
      res = dabc::mgr()->CreateDevice("roc::UdpDevice", fDevName.c_str());
      DOUT1(("Create Roc Device = %s", DBOOL(res)));
      if(!res) return false;

      cmd = new dabc::CmdCreateModule("roc::CombinerModule", "RocComb", "RocCombThrd");
      cmd->SetInt(dabc::xmlNumOutputs, 2);
      res = dabc::mgr()->Execute(cmd);
      DOUT1(("Create ROC combiner module = %s", DBOOL(res)));
      if(!res) return false;
   }

   if (DoCalibr()) {
      cmd = new dabc::CmdCreateModule("roc::CalibrationModule", "RocCalibr", "RocCalibrThrd");
      cmd->SetInt(dabc::xmlNumOutputs, 2);
      res = dabc::mgr()->Execute(cmd);
      DOUT1(("Create ROC calibration module = %s", DBOOL(res)));
      if(!res) return false;

      if (DoTaking()) {
         res = dabc::mgr()->ConnectPorts("RocComb/Output0", "RocCalibr/Input");
         DOUT1(("Connect readout and calibr modules = %s", DBOOL(res)));
         if(!res) return false;
      }
   }

   if (DoTaking()) {

   //// connect module to ROC device, one transport for each board:

      for(int t=0; t<NumRocs(); t++) {
         cmd = new dabc::CmdCreateTransport(FORMAT(("RocComb/Input%d", t)), fDevName.c_str()); // container for additional board parameters
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
         cmd = new dabc::CmdCreateTransport("RocComb/Output1", mbs::typeLmdOutput);
         // no need to set extra size parameters - it will be taken from application
         // if (FileSizeLimit()>0) cmd->SetInt(mbs::xmlSizeLimit, FileSizeLimit());
      } else {
         cmd = new dabc::CmdCreateTransport("RocCalibr/Input", mbs::typeLmdInput);
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

      cmd = new dabc::CmdCreateTransport("RocCalibr/Output1", outtype);

      cmd->SetStr(mbs::xmlFileName, CalibrFileName().c_str());
      // no need to set extra size parameters - it will be taken from application
      // if (FileSizeLimit()>0) cmd->SetInt(mbs::xmlSizeLimit, FileSizeLimit());

      res = dabc::mgr()->Execute(cmd);
      DOUT1(("Create calibr output file res = %s", DBOOL(res)));
      if(!res) return false;
   }


   if (DataServerKind() != mbs::NoServer) {

   ///// connect module to mbs server:
      const char* portname = DoCalibr() ? "RocCalibr/Output0" : "RocComb/Output0";
      cmd = new dabc::CmdCreateTransport(portname, mbs::typeServerTransport, "MbsServerThrd");

      // no need to set extra parameters - they will be taken from application !!!
//      cmd->SetStr(mbs::xmlServerKind, mbs::ServerKindToStr(DataServerKind())); //mbs::StreamServer ,mbs::TransportServer
//      cmd->SetInt(dabc::xmlBufferSize, GetParInt(dabc::xmlBufferSize, 8192));

      res = dabc::mgr()->Execute(cmd);
      DOUT1(("Connected readout module output to Mbs server = %s", DBOOL(res)));
      if(!res) return false;
   }

   return true;
}

roc::Device* roc::ReadoutApplication::GetBoardDevice(int indx)
{
   return dynamic_cast<roc::Device*> (dabc::mgr()->FindDevice(fDevName.c_str()));
}

bool roc::ReadoutApplication::ConfigureRoc(int indx)
{
   roc::Device* dev = GetBoardDevice(indx);
   if (dev==0) return false;

   bool res = true;

//   res = res && dev->Poke(ROC_NX_SELECT, 0);
//   res = res && dev->Poke(ROC_NX_ACTIVE, 0);
   res = res && dev->poke(ROC_SYNC_M_SCALEDOWN, 1);
   res = res && dev->poke(ROC_AUX_ACTIVE, 3);

/*
   res = res && dev->poke(ROC_ACTIVATE_LOW_LEVEL, 1);
   res = res && dev->poke(ROC_DO_TESTSETUP,1);
   res = res && dev->poke(ROC_ACTIVATE_LOW_LEVEL,0);

   res = res && dev->poke(ROC_NX_REGISTER_BASE + 0,255);
   res = res && dev->poke(ROC_NX_REGISTER_BASE + 1,255);
   res = res && dev->poke(ROC_NX_REGISTER_BASE + 2,0);
   res = res && dev->poke(ROC_NX_REGISTER_BASE + 18,35);
   res = res && dev->poke(ROC_NX_REGISTER_BASE + 32,1);

   res = res && dev->poke(ROC_FIFO_RESET,1);
   res = res && dev->poke(ROC_BUFFER_FLUSH_TIMER,1000);
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
