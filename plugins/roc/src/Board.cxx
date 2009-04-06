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


const char* roc::xmlNumRocs         = "NumRocs";
const char* roc::xmlRocPool         = "RocPool";
const char* roc::xmlTransportWindow = "TransportWindow";
const char* roc::xmlBoardIP         = "BoardIP";

const char* roc::typeUdpDevice      = "roc::UdpDevice";


roc::Board::Board() :
   fRole(roleNone),
   fErrNo(0),
   fRocNumber(0)
{
}

roc::Board::~Board()
{
}

roc::Board* roc::Board::Connect(const char* name, BoardRole role)
{
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

   return brd;
}


bool roc::Board::startDaq()
{
   return true;
}

bool roc::Board::suspendDaq()
{
   return true;
}

bool roc::Board::stopDaq()
{
   return true;
}

bool roc::Board::getNextData(nxyter::Data& data, double tmout)
{
   return false;
}

