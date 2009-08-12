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
#include "bnet/BuilderModuleAsync.h"

#include "dabc/logging.h"
#include "dabc/PoolHandle.h"
#include "dabc/Command.h"
#include "dabc/Port.h"
#include "dabc/Parameter.h"

#include "bnet/common.h"

bnet::BuilderModuleAsync::BuilderModuleAsync(const char* name, dabc::Command* cmd) :
   dabc::ModuleAsync(name, cmd),
   fInpPool(0),
   fOutPool(0),
   fNumSenders(1)
{
   fOutPool = CreatePoolHandle(GetCfgStr(dabc::xmlOutputPoolName, bnet::EventPoolName, cmd).c_str());

   CreateOutput("Output", fOutPool, BuilderOutQueueSize);

   fInpPool = CreatePoolHandle(GetCfgStr(dabc::xmlInputPoolName, bnet::TransportPoolName, cmd).c_str());

   CreateInput("Input", fInpPool, BuilderInpQueueSize, sizeof(bnet::SubEventNetHeader));

   fOutBufferSize = GetCfgInt(xmlEventBuffer, 2048, cmd);

   CreateParStr(parSendMask, "xxxx");
}

bnet::BuilderModuleAsync::~BuilderModuleAsync()
{
   for (unsigned n=0; n<fBuffers.size(); n++)
      dabc::Buffer::Release(fBuffers[n]);
   fBuffers.clear();
}

void bnet::BuilderModuleAsync::BeforeModuleStart()
{
   fNumSenders = bnet::NodesVector(GetParStr(parSendMask)).size();
}

void bnet::BuilderModuleAsync::ProcessUserEvent(dabc::ModuleItem*, uint16_t)
{
   while (true) {

      while (Input()->CanRecv() && (fBuffers.size() < fNumSenders)) {
         dabc::Buffer* buf = Input()->Recv();
         if (buf==0)
            EOUT(("Cannot read buffer from input - hard error"));
         else {
            fBuffers.push_back(buf);

            // should we check EOL status already here ???
            if (buf->GetTypeId() == dabc::mbt_EOL) break;
         }
      }

      bool iseol = false;
      for (unsigned n=0;n<fBuffers.size();n++)
         if (fBuffers[n]->GetTypeId() == dabc::mbt_EOL) iseol = true;

      // if we cannot send - nothing to do
      if (!Output()->CanSend()) return;

      if ((fBuffers.size() < fNumSenders) && !iseol) return;

      if (iseol) {
         DOUT1(("Send EOL buffer to the output"));
         dabc::Buffer* eolbuf = fOutPool->TakeEmptyBuffer();
         eolbuf->SetTypeId(dabc::mbt_EOL);
         Output(0)->Send(eolbuf);
      } else
         DoBuildEvent(fBuffers);

      // release all buffers
      for (unsigned n=0;n<fBuffers.size();n++)
         dabc::Buffer::Release(fBuffers[n]);
      fBuffers.clear();
   }
}
