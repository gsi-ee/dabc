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

#include <stdlib.h>

#include "roc/Board.h"

roc::ReadoutModule::ReadoutModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fDataCond(),
   fCurrBuf(),
   fNextBuf()
{
   EnsurePorts(1, 0);
}

roc::ReadoutModule::~ReadoutModule()
{
}

void roc::ReadoutModule::AfterModuleStop()
{
   fCurrBuf.Release();

   dabc::Buffer buf;
   {
      dabc::LockGuard lock(fDataCond.CondMutex());
      buf << fNextBuf;
   }
   buf.Release();
}

bool roc::ReadoutModule::ProcessRecv(unsigned port)
{
   // one may avoid locking while here is basic type check is done
   // if mutex is locked and user just reading last byte, it will
   // fire new event after buffer set to 0
   if (!fNextBuf.null()) return false;

   dabc::Buffer buf = Recv(port);

//   DOUT1("Read buffer %p from input", buf));

   dabc::LockGuard lock(fDataCond.CondMutex());
   if (!fNextBuf.null()) {
      EOUT("Something completely wrong");
      exit(111);
   }
   fNextBuf << buf;
   if (!fNextBuf.null() && fDataCond._Waiting()) fDataCond._DoFire();

   // no need to read one more buffer
   return false;
}

bool roc::ReadoutModule::getNextBuffer(void* &buf, unsigned& len, double tmout)
{
   if (!fCurrBuf.null())  fCurrBuf.Release();

   ProduceInputEvent();

   // lock only when new complete buffer is required
   {
      dabc::LockGuard lock(fDataCond.CondMutex());
      if (fNextBuf.null()) fDataCond._DoWait(tmout);
      fCurrBuf << fNextBuf;
   }

   if (fCurrBuf.null()) return false;

   buf = fCurrBuf.SegmentPtr();
   len = fCurrBuf.SegmentSize();

   return true;
}
