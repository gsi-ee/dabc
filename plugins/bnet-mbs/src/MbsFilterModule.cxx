#include "bnet/MbsFilterModule.h"

#include "dabc/logging.h"

#include "bnet/WorkerApplication.h"

#include "mbs/MbsTypeDefs.h"

#include "mbs/Iterator.h"


bnet::MbsFilterModule::MbsFilterModule(const char* name, dabc::Command* cmd) :
   FilterModule(name, cmd)
{
   DOUT5(("Create MbsFilterModule %s", name));
   fTotalCnt = 0;
}

bool bnet::MbsFilterModule::TestBuffer(dabc::Buffer* buf)
{
   if (fTotalCnt++ % 100 == 0)
      DOUT5(("Total filtered buffers %d", fTotalCnt));

   /*


   unsigned numevnts = mbs::ReadIterator::NumEvents(buf);

   DOUT1(("NumEvents %u", numevnts));

   mbs::ReadIterator iter(buf);

   while (iter.NextEvent()) {
      DOUT1(("Event %u size %u", iter.evnt()->EventNumber(), iter.evnt()->FullSize()));

      while (iter.NextSubEvent()) {
         uint32_t* rawdata = (uint32_t*) iter.rawdata();
         DOUT1(("Subevent crate %u procid %u size %u rawdata0 %u rawdata1 %u",
               iter.subevnt()->iSubcrate, iter.subevnt()->iProcId, iter.subevnt()->FullSize(),
               rawdata[0], rawdata[1]));
      }
   }

   */

   return (fTotalCnt++ % 2) > 0;
}
