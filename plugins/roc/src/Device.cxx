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

#include "roc/Device.h"

const char* roc::xmlNumRocs         = "NumRocs";
const char* roc::xmlRocPool         = "RocPool";
const char* roc::xmlTransportWindow = "TransportWindow";
const char* roc::xmlBoardIP         = "BoardIP";

roc::Device::Device(dabc::Basic* parent, const char* name) :
   dabc::Device(parent, name),
   fErrNo(0)
{

}

roc::Device::~Device()
{

}

int roc::Device::ExecuteCommand(dabc::Command* cmd)
{
   return cmd_false;
}


int roc::Device::CreateTransport(dabc::Command* cmd, dabc::Port* port)
{
   return dabc::Device::CreateTransport(cmd, port);
}


bool roc::Device::poke(uint32_t addr, uint32_t value)
{
   fErrNo = 0;
   return true;
}

uint32_t roc::Device::peek(uint32_t addr)
{
   fErrNo = 0;
   return 0;
}
