#include "MbsFilterModule.h"

#include "dabc/logging.h"

#include "bnet/WorkerPlugin.h"

#include "mbs/MbsTypeDefs.h"

bnet::MbsFilterModule::MbsFilterModule(dabc::Manager* m, const char* name, WorkerPlugin* factory) : 
   FilterModule(m, name, factory)
{
   DOUT5(("Create MbsFilterModule %s", name)); 
   fTotalCnt = 0;
}

bool bnet::MbsFilterModule::TestBuffer(dabc::Buffer* buf)
{
   if (fTotalCnt++ % 100 == 0) {
//      mbs::NewIterateBuffer(buf);
      DOUT5(("Total filtered buffers %d", fTotalCnt));
   }

   return (fTotalCnt++ % 2) > 0;
}
