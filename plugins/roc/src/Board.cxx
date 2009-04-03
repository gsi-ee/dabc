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

#include "roc/Device.h"
#include "roc/Defines.h"


roc::Board::Board(Device* dev, BoardRole role) :
   fDev(dev),
   fRole(roleNone)
{
}

roc::Board::~Board()
{
   if (fDev) fDev->DestroyProcessor();
}

roc::Board* roc::Board::Connect(const char* name, BoardRole role)
{
   if (dabc::mgr()==0)
      if (!dabc::Factory::CreateManager()) return 0;

   const char* devname = "RocUdp";

   dabc::Command* cmd = new dabc::CmdCreateDevice(typeUdpDevice, devname);
   cmd->SetStr(roc::xmlBoardIP, name);
   if (!dabc::mgr()->Execute(cmd)) return 0;

   roc::Device* dev = dynamic_cast<roc::Device*> (dabc::mgr()->FindDevice(devname));
   if (dev==0) return 0;

   dev->poke(ROC_MASTER_LOGIN, 0);

   dev->peek(ROC_NUMBER);

   return new roc::Board(dev, role);
}

int roc::Board::errno() const
{
   return 0;
}


bool roc::Board::poke(uint32_t addr, uint32_t value)
{
   return true;
}

uint32_t roc::Board::peek(uint32_t addr)
{
   return 0;
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

