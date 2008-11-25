#include "MbsFilterModule.h"

#include "dabc/logging.h"

#include "bnet/WorkerApplication.h"

#include "mbs/MbsTypeDefs.h"

bnet::MbsFilterModule::MbsFilterModule(const char* name, WorkerApplication* factory) :
   FilterModule(name, factory)
{
   DOUT5(("Create MbsFilterModule %s", name));
   fTotalCnt = 0;
}

bool bnet::MbsFilterModule::TestBuffer(dabc::Buffer* buf)
{
   if (fTotalCnt++ % 100 == 0)
      DOUT5(("Total filtered buffers %d", fTotalCnt));

   return (fTotalCnt++ % 2) > 0;
}
