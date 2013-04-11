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
#include "TestFilterModule.h"

#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Pointer.h"

#include "bnet/common.h"

bnet::TestFilterModule::TestFilterModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   CreatePoolHandle(bnet::EventPoolName);

   CreateInput("Input", Pool(), FilterInpQueueSize);

   CreateOutput("Output", Pool(), FilterOutQueueSize);
}

void bnet::TestFilterModule::ProcessItemEvent(dabc::ModuleItem*, uint16_t)
{
//   DOUT1("!!!!!!!! ProcessInputEvent port = %p", port);

   while (Input(0)->CanRecv() && Output(0)->CanSend()) {

      dabc::Buffer buf = Input(0)->Recv();

      if (buf.null()) {
         EOUT("Fail to receive data from Input");
         return;
      }

      dabc::Pointer(buf).copyto(&evid, sizeof(evid));

      if (evid % 2) {
    //   DOUT1("EVENT KILLED %llu", evid);
         buf.Release();
      } else
         Output(0)->Send(buf);
   }
}
