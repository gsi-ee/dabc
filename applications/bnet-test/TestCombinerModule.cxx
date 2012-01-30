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
#include "TestCombinerModule.h"

#include "dabc/logging.h"
#include "dabc/string.h"
#include "dabc/PoolHandle.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"
#include "dabc/Parameter.h"

bnet::TestCombinerModule::TestCombinerModule(const char* name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fNumReadout(1),
   fModus(0)
{
   fModus = Cfg(xmlCombinerModus,cmd).AsInt();
   fNumReadout = Cfg(xmlNumReadouts,cmd).AsInt(1);
   fOutBufferSize = Cfg(xmlTransportBuffer,cmd).AsInt(1024);

   fOutPool = CreatePoolHandle(bnet::TransportPoolName);

   fInpPool = CreatePoolHandle(Cfg(CfgReadoutPool, cmd).AsStr(ReadoutPoolName));

   fOutPort = CreateOutput("Output", fOutPool, SenderInQueueSize);

   for (int n=0;n<fNumReadout;n++) {
      CreateInput(FORMAT(("Input%d", n)), fInpPool, ReadoutQueueSize);
      fBuffers.push_back(0);
   }

   fLastInput = 0; // indicate id of the port where next packet must be read

   fOutBuffer = 0;

   fLastEvent = 0;
}

bnet::TestCombinerModule::~TestCombinerModule()
{
   for (unsigned n=0;n<fBuffers.size();n++)
      dabc::Buffer::Release(fBuffers[n]);

   dabc::Buffer::Release(fOutBuffer);

   DOUT5(("TestCombinerModule destroyed"));
}

void bnet::TestCombinerModule::ProcessUserEvent(dabc::ModuleItem*, uint16_t)
{
   while ((fLastInput>=0) && (fLastInput<fNumReadout)) {
      int ninp = fLastInput;

      if (!Input(ninp)->CanRecv()) return;

      dabc::Buffer* buf = Input(ninp)->Recv();
      if (buf==0) return;

      dabc::Buffer::Release(fBuffers[ninp]);

      fBuffers[ninp] = buf;

//      DOUT1(("Get buffer %p at input %d", buf, ninp));

      fLastInput++;
   }

   if (fOutBuffer==0) {

      uint64_t evid = 0;

      switch (fModus) {
         case 1:
            fOutBuffer = MakeSegmenetedBuf(evid);
            break;
         default:
            fOutBuffer = MakeMemCopyBuf(evid);
            break;
      }

      if (fOutBuffer==0) return;

      fOutBuffer->SetHeaderSize(sizeof(bnet::EventId));
      *((bnet::EventId*) fOutBuffer->GetHeader()) = evid;

//      DOUT1(("Combine subevent %llu", evid));

      fLastEvent = evid;
   }

   if (!Output(0)->CanSend()) return;

   dabc::Buffer* buf = fOutBuffer;
   fOutBuffer = 0;
   Output(0)->Send(buf);

   DOUT3(("Combiner produces event %llu", fLastEvent));

   fLastInput = 0; // start from the beginning
}

dabc::Buffer* bnet::TestCombinerModule::MakeSegmenetedBuf(uint64_t& evid)
{
   evid = 0;

   dabc::Buffer* buf = fOutPool->TakeEmptyBuffer();

   for (unsigned n=0;n<fBuffers.size();n++) {
      if (fBuffers[n]==0) {
         EOUT(("No buffer in the list"));
         exit(1);
      }

      uint64_t* mem = (uint64_t*) fBuffers[n]->GetDataLocation();
      if (n==0) evid = *mem; else
        if (evid != *mem) {
           EOUT(("Missmatch with evid %lld %lld", evid, *mem));
           exit(1);
        }

      // we adding references on the complete buffer with adoption
      buf->AddBuffer(fBuffers[n], true);

      // it means, that we just simple "forget" buffer after such call
      fBuffers[n] = 0;
   }

   if (buf->GetTotalSize() > fOutBufferSize) {
      EOUT(("Size missmtach %d %d", buf->GetTotalSize(), fOutBufferSize));
      exit(1);
   }

   return buf;
}

dabc::Buffer* bnet::TestCombinerModule::MakeMemCopyBuf(uint64_t& evid)
{
   dabc::BufferSize_t sz = 0;

   evid = 0;

   for (unsigned n=0;n<fBuffers.size();n++) {
      if (fBuffers[n]==0) {
         EOUT(("No buffer in the list n = %u sz = %u", n, fBuffers.size()));
         exit(1);
      }

      sz += fBuffers[n]->GetTotalSize();

      uint64_t* mem = (uint64_t*) fBuffers[n]->GetDataLocation();
      if (n==0) evid = *mem; else
      if (evid != *mem) {
         EOUT(("Missmatch with evid %lld %lld", evid, *mem));
         exit(1);
      }
   }

   dabc::Buffer* buf = fOutPool->TakeBufferReq(sz, sizeof(bnet::EventId));
   if (buf==0) return 0;

   // release all buffers for further usage
   for (unsigned n=0;n<fBuffers.size();n++) {
      dabc::Buffer::Release(fBuffers[n]);
      fBuffers[n] = 0;
   }

   uint64_t* mem = (uint64_t*) buf->GetDataLocation();
   *mem = evid;

   return buf;
}

