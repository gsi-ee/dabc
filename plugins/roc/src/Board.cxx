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

#include "roc/Board.h"

#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/Factory.h"
#include "dabc/Device.h"

#include "roc/Defines.h"
#include "roc/ReadoutModule.h"


const char* roc::xmlNumRocs         = "NumRocs";
const char* roc::xmlRocPool         = "RocPool";
const char* roc::xmlTransportWindow = "TransportWindow";
const char* roc::xmlBoardIP         = "BoardIP";

const char* roc::typeUdpDevice      = "roc::UdpDevice";


roc::Board::Board() :
   fRole(roleNone),
   fErrNo(0),
   fRocNumber(0),
   fReadout(0)
{
}

roc::Board::~Board()
{
}

roc::Board* roc::Board::Connect(const char* name, BoardRole role)
{
   dabc::SetDebugLevel(0);

   if (dabc::mgr()==0)
      if (!dabc::Factory::CreateManager()) return 0;

   const char* devname = "RocUdp";

   dabc::Command* cmd = new dabc::CmdCreateDevice(typeUdpDevice, devname);
   cmd->SetStr(roc::xmlBoardIP, name);
   if (!dabc::mgr()->Execute(cmd)) return 0;

   dabc::Device* dev = dabc::mgr()->FindDevice(devname);
   if (dev==0) return 0;

   std::string sptr = dev->ExecuteStr("GetBoardPtr", "BoardPtr");
   void *ptr = 0;

   if (sptr.empty() || (sscanf(sptr.c_str(),"%p", &ptr) != 1)) {
      dev->DestroyProcessor();
      return 0;
   }

   roc::Board* brd = (roc::Board*) ptr;

   if ((brd==0) || !brd->initialise(role)) {
      dev->DestroyProcessor();
      return 0;
   }

   if (role == roleDAQ) {
      roc::ReadoutModule* m = new roc::ReadoutModule("RocReadout");
      dabc::mgr()->MakeThreadForModule(m, "ReadoutThrd");

      if (!dabc::mgr()->CreateTransport("RocReadout/Input", devname)) {
         EOUT(("Cannot connect readout module to device %s", devname));
         dabc::mgr()->DeleteModule("RocReadout");
      }

      brd->SetReadout(m);

      dabc::lgr()->SetLogLimit(1000000);
   }

   return brd;
}

bool roc::Board::Close(Board* brd)
{
   if (brd==0) return false;
   dabc::Device* dev = (dabc::Device*) brd->getdeviceptr();
   if (dev==0) return false;

   brd->SetReadout(0);
   dabc::mgr()->DeleteModule("RocReadout");

   if (!dev->Execute("Disconnect", 4.)) return false;
   dev->DestroyProcessor();
   return true;
}


bool roc::Board::startDaq()
{
   DOUT2(("Starting DAQ !!!!"));

   dabc::Device* dev = (dabc::Device*) getdeviceptr();

   if ((fReadout==0) || (dev==0)) return false;

   if (!dev->Execute("PrepareStartDaq", 5)) return false;

   fReadout->Start();

   return true;
}

bool roc::Board::suspendDaq()
{
   DOUT2(("Suspend DAQ !!!!"));

   dabc::Device* dev = (dabc::Device*) getdeviceptr();
   if (dev==0) return false;

   if (!dev->Execute("PrepareSuspendDaq", 5.)) return false;

   return true;
}

bool roc::Board::stopDaq()
{
   DOUT2(("Stop DAQ !!!!"));

   dabc::Device* dev = (dabc::Device*) getdeviceptr();
   if ((dev==0) || (fReadout==0)) return false;

   if (!dev->Execute("PrepareStopDaq", 5)) return false;

   fReadout->Stop();

   return true;
}

bool roc::Board::getNextData(nxyter::Data& data, double tmout)
{
   return fReadout ? fReadout->getNextData(data, tmout) : false;
}
