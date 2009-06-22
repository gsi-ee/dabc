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
#include "dabc/Device.h"

#include "mbs/MbsTypeDefs.h"
#include "mbs/Factory.h"

#include "roc/defines.h"


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
   DOUT1(("CreateAppModules starts..·"));
   bool res = false;
   dabc::Command* cmd;

   dabc::lgr()->SetLogLimit(1000000);

   dabc::mgr()->CreateMemoryPool(roc::xmlRocPool,
                                 GetParInt(dabc::xmlBufferSize, 8192),
                                 GetParInt(dabc::xmlNumBuffers, 100));

   if (DoTaking()) {

      cmd = new dabc::CmdCreateModule("roc::CombinerModule", "RocComb", "RocCombThrd");
      cmd->SetInt(dabc::xmlNumOutputs, 2);
      res = dabc::mgr()->Execute(cmd);
      DOUT1(("Create ROC combiner module = %s", DBOOL(res)));
      if(!res) return false;

      for(int t=0; t<NumRocs(); t++) {

         std::string devname = dabc::format("Roc%ddev", t);

         dabc::Command* cmd = new dabc::CmdCreateDevice(typeUdpDevice, devname.c_str(), "RocsDevThread");
         cmd->SetStr(roc::xmlBoardIP, RocIp(t));
         cmd->SetInt(roc::xmlRole, roleDAQ);

         if (!dabc::mgr()->Execute(cmd)) {
            EOUT(("Cannot create device %s for roc %s", devname.c_str(), RocIp(t).c_str()));
            return false;
         }

         ConfigureRoc(t);

         if (!dabc::mgr()->CreateTransport(FORMAT(("RocComb/Input%d", t)), devname.c_str(), "RocsDevThread")) {
            EOUT(("Cannot connect readout module to device %s", devname.c_str()));
            return false;
         }
      }

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

roc::Board* roc::ReadoutApplication::GetBoard(int indx)
{
   std::string devname = dabc::format("Roc%ddev", indx);

   dabc::Device* dev = dabc::mgr()->FindDevice(devname.c_str());
   if (dev==0) return 0;

   std::string sptr = dev->ExecuteStr("GetBoardPtr", "BoardPtr");
   void *ptr = 0;

   if (sptr.empty() || (sscanf(sptr.c_str(),"%p", &ptr) != 1)) return 0;
   return (roc::Board*) ptr;
}

bool roc::ReadoutApplication::ConfigureRoc(int indx)
{
   roc::Board* brd = GetBoard(indx);
   if (brd==0) {
      EOUT(("Cannot configure ROC[%d]", indx));
      return false;
   }

   brd->GPIO_setCONFIG(SYNC_M, 0, 0, 1, 1);
   brd->GPIO_setCONFIG(SYNC_S0, 0, 0, 1, 1);
   brd->GPIO_setScaledown(3);

   return true;
}

bool roc::ReadoutApplication::IsModulesRunning()
{
   if (DoTaking())
      if (!dabc::mgr()->IsModuleRunning("RocComb")) return false;

   if (DoCalibr())
      if (!dabc::mgr()->IsModuleRunning("RocCalibr")) return false;

   return true;
}
