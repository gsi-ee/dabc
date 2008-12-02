#include "MbsGeneratorModule.h"

#include "dabc/logging.h"
#include "dabc/Buffer.h"
#include "mbs/MbsTypeDefs.h"
#include "mbs/Iterator.h"

void bnet::MbsGeneratorModule::GeneratePacket(dabc::Buffer* buf)
{
   int evid = fEventCnt;

   bnet::GenerateMbsPacket(buf, UniqueId(), evid, 480, 4);

   fEventCnt = evid;
}

bool bnet::GenerateMbsPacket(dabc::Buffer* buf,
                             int procid,
                             int &evid,
                             int SingleSubEventDataSize,
                             int MaxNumSubEvents,
                             int startacqevent,
                             int stopacqevent)
{
   if (buf==0) return false;


   uint32_t EventDataSize = MaxNumSubEvents * (sizeof(mbs::SubeventHeader) + SingleSubEventDataSize);

   mbs::WriteIterator iter(buf);

   while (iter.NewEvent(evid, EventDataSize)) {
      if (evid==startacqevent)
         iter.evnt()->iTrigger = mbs::tt_StartAcq;
      else
      if (evid==stopacqevent)
         iter.evnt()->iTrigger = mbs::tt_StopAcq;

      int cnt = MaxNumSubEvents;

      while ((cnt-->0) && iter.NewSubevent(SingleSubEventDataSize)) {

         iter.subevnt()->iProcId = procid * MaxNumSubEvents + cnt;
         iter.subevnt()->iSubcrate = procid;
         iter.subevnt()->iControl = 2;

         iter.FinishSubEvent(SingleSubEventDataSize);
      }

      if (cnt>0) {
         EOUT(("Should not happened - one always should be able to produce subevents"));
         break;
      }

      iter.FinishEvent();

      evid++;
   }

   return true;
}
