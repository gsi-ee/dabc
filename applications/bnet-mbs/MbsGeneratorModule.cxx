#include "MbsGeneratorModule.h"

#include "dabc/logging.h"
#include "dabc/Buffer.h"
#include "mbs/MbsTypeDefs.h"

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
         EOUT(("Should not happend - one always should be able to produce subevents"));
         break;
      }

      evid++;
   }

   return true;
}

/*


bool mbs::GenerateMbsPacket(dabc::Buffer* buf,
                            int procid,
                            int &evid,
                            int SingleSubEventDataSize,
                            int MaxNumSubEvents,
                            int startacqevent,
                            int stopacqevent,
                            bool newformat)
{
   if (buf==0) return false;

   dabc::BufferSize_t minimumeventsize = sizeof(mbs::eMbs101EventHeader) +
        MaxNumSubEvents * (sizeof(mbs::eMbs101SubeventHeader) + SingleSubEventDataSize);

   dabc::BufferSize_t minimumsubeventsize = sizeof(mbs::eMbs101SubeventHeader) + SingleSubEventDataSize;

   dabc::Pointer evptr, subevptr, dataptr;

   mbs::sMbsBufferHeader* bufhdr(0), tmp0;
   mbs::eMbs101EventHeader *evhdr(0), tmp1;
   mbs::eMbs101SubeventHeader  *subevhdr(0), tmp2;

   bool res = false;

   if (StartBuffer(buf, evptr, bufhdr, &tmp0, newformat)) {

      while (StartEvent(evptr, subevptr, evhdr, &tmp1)) {

         evhdr->iCount = evid++;
         evhdr->iTrigger = mbs::tt_Event;
         evhdr->iDummy = 0;

         if (evhdr->iCount==startacqevent)
            evhdr->iTrigger = mbs::tt_StartAcq;
         else
         if (evhdr->iCount==stopacqevent)
            evhdr->iTrigger = mbs::tt_StopAcq;

         int cnt = MaxNumSubEvents;

         while ((cnt-->0) && StartSubEvent(subevptr, dataptr, subevhdr, &tmp2)) {
            subevhdr->iProcId = procid * MaxNumSubEvents + cnt;
            subevhdr->iSubcrate = procid;
            subevhdr->iControl = 2;

            dataptr.shift(SingleSubEventDataSize);
            FinishSubEvent(subevptr, dataptr, subevhdr);

            // if we have few memory left, stop generation
            if (subevptr.fullsize() < minimumsubeventsize) break;
         }

         FinishEvent(evptr, subevptr, evhdr, bufhdr);

//         DOUT1(("Generated event %d remain buffer %d", evid, evptr.fullsize()));

         // if buffer contains too few memory, just close it
         if (evptr.fullsize() < minimumeventsize) break;
      }

      res = FinishBuffer(buf, evptr, bufhdr);
   }

   return res;
}

*/
