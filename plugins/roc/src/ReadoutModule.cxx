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

#include "roc/ReadoutModule.h"

#include "roc/Board.h"

#include "dabc/Port.h"

roc::ReadoutModule::ReadoutModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleAsync(name, cmd),
   fDataCond(),
   fCurrBuf(0),
   fCurrBufPos(0)
{
   fBufferSize = GetCfgInt(dabc::xmlBufferSize, 16384, cmd);
   int numoutputs = GetCfgInt(dabc::xmlNumOutputs, 1, cmd);

   DOUT2(("new roc::ReadoutModule %s buff %d", GetName(), fBufferSize));

   CreatePoolHandle(roc::xmlRocPool, fBufferSize, 10);

   CreateInput("Input", Pool(), 10);
   CreateRateParameter("InputRate", false, 3., "Input")->SetDebugOutput(true);

   for(int n=0; n<numoutputs; n++)
      CreateOutput(FORMAT(("Output%d", n)), Pool(), 10);
}

roc::ReadoutModule::~ReadoutModule()
{
}

void roc::ReadoutModule::AfterModuleStop()
{
   dabc::Buffer* buf = 0;
   {
      dabc::LockGuard lock(fDataCond.CondMutex());
      buf = fCurrBuf;
      fCurrBufPos = 0;
      fCurrBuf = 0;
   }
   dabc::Buffer::Release(buf);
}

void roc::ReadoutModule::ProcessInputEvent(dabc::Port* inport)
{
   if (!inport->CanRecv()) return;

   // one may avoid locking while here is basic type check is done
   // if mutex is locked and user just reading last byte, it will
   // fire new event after buffer set to 0
   if (fCurrBuf!=0) return;

   dabc::Buffer* buf = inport->Recv();

//   DOUT1(("Read buffer %p from input", buf));

   dabc::LockGuard lock(fDataCond.CondMutex());
   if (fCurrBuf!=0) {
      EOUT(("Something completely wrong"));
      exit(111);
   }
   fCurrBuf = buf;
   if ((fCurrBuf!=0) && fDataCond._Waiting()) fDataCond._DoFire();
}

void roc::ReadoutModule::ProcessOutputEvent(dabc::Port* inport)
{
}

bool roc::ReadoutModule::getNextData(nxyter::Data& data, double tmout)
{
   dabc::Buffer* buf = 0;
   bool res = false;

   {
      dabc::LockGuard lock(fDataCond.CondMutex());

      if (fCurrBuf==0)
         fDataCond._DoWait(tmout);

      if (fCurrBuf!=0) {
//         DOUT0(("Get next data buff %p pos %u", fCurrBuf, fCurrBufPos));

         void* src = (char*) (fCurrBuf->GetDataLocation()) + fCurrBufPos;
         memcpy(&data, src, sizeof(nxyter::Data));
         fCurrBufPos += sizeof(nxyter::Data);
         res = true;
         if (fCurrBufPos >= fCurrBuf->GetDataSize()) {
            buf = fCurrBuf;
            fCurrBuf = 0;
            fCurrBufPos = 0;
         }
      }
   }

   if (buf!=0) {
      dabc::Buffer::Release(buf);

      // one should fire extra event to get next buffer
//      DOUT0(("-------- Fire extra event -----------"));
      Input()->FireInput();
   }

   return res;
}
