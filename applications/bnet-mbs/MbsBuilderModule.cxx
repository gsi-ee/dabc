#include "MbsBuilderModule.h"

#include "dabc/logging.h"
#include "dabc/PoolHandle.h"
#include "dabc/Command.h"
#include "dabc/Port.h"
#include "dabc/Parameter.h"

#include "bnet/common.h"

bnet::MbsBuilderModule::MbsBuilderModule(const char* name, dabc::Command* cmd) :
   BuilderModule(name, cmd),
   fEvntRate(0)
{
   fCfgEventsCombine = GetCfgInt(CfgEventsCombine, 1, cmd);

   fOut.buf = 0;
   fOut.ready = false;

   fEvntRate = CreateRateParameter("EventRate", false, 1., "", "", "Ev/s", 0., 10000.);
}

bnet::MbsBuilderModule::~MbsBuilderModule()
{
   dabc::Buffer::Release(fOut.buf);
   fOut.buf = 0;
   fOut.ready = false;
}

void bnet::MbsBuilderModule::StartOutputBuffer(dabc::BufferSize_t bufsize)
{
   if ((fOut.buf==0) && !fOut.ready) {

      // increase buffer size on the header information
      if (bufsize<fOutBufferSize) bufsize = fOutBufferSize;

      fOut.buf = TakeBuffer(fOutPool, bufsize);

      if (!fOut.iter.Reset(fOut.buf)) {
         EOUT(("Problem to start with MBS buffer"));
         exit(1);
      }
   }
}

void bnet::MbsBuilderModule::FinishOutputBuffer()
{
   fOut.iter.Close();

   // if (fEvntRate) fEvntRate->AccountValue(fOut.bufhdr->iNumEvents);

   fOut.ready = true;
}

void bnet::MbsBuilderModule::SendOutputBuffer()
{
   dabc::Buffer* buf = fOut.buf;
   fOut.ready = false;
   fOut.buf = 0;
   Send(Output(0), buf);
}

void bnet::MbsBuilderModule::DoBuildEvent(std::vector<dabc::Buffer*>& bufs)
{
//   DOUT1(("start DoBuildEvent n = %d", fCfgEventsCombine));

   std::vector<mbs::ReadIterator> recs;
   recs.resize(bufs.size());

   for (unsigned n=0;n<bufs.size();n++) {

      if (bufs[n]==0) {
         EOUT(("Buffer %d is NULL !!!!!!!!!!!!!!!", n));
         exit(1);
      }

      if (!recs[n].Reset(bufs[n]) || !recs[n].NextEvent()) {
         EOUT(("Invalid MBS format on buffer %u size: %u", n, bufs[n]->GetTotalSize()));
         return;
      }
   }

   int nevent = 0;

   while (nevent<fCfgEventsCombine) {
      unsigned pmin(0), pmax(0);

      uint32_t subeventslen = 0;

      for (unsigned n=0;n<recs.size();n++) {
         if (recs[n].evnt()->EventNumber() < recs[pmin].evnt()->EventNumber()) pmin = n; else
         if (recs[n].evnt()->EventNumber() > recs[pmax].evnt()->EventNumber()) pmax = n;

         subeventslen += recs[n].evnt()->SubEventsSize();
      }

      if (recs[pmin].evnt()->EventNumber() < recs[pmax].evnt()->EventNumber()) {
         EOUT(("Skip subevent %u from buffer %u", recs[pmin].evnt()->EventNumber(), pmin));
         if (!recs[pmin].NextEvent()) return;
         // try to analyze events now
         continue;
      }

//      DOUT1(("Build event %u len %u", recs[pmin].evnt()->EventNumber(), subeventslen));

      if (subeventslen==0) {
         EOUT(("Something wrong with data"));
         return;
      }

      // if rest of existing buffer less than new event data, close it
      if (!fOut.ready && fOut.buf && !fOut.iter.IsPlaceForEvent(subeventslen))
         FinishOutputBuffer();

      // send buffer, if it is filled and ready to send
      if (fOut.ready) SendOutputBuffer();

      // start new buffer if required
      if ((fOut.buf==0) && !fOut.ready) {
         StartOutputBuffer(subeventslen + sizeof(mbs::EventHeader));

         if (!fOut.iter.IsPlaceForEvent(subeventslen)) {
            EOUT(("Single event do not pass in to the buffers"));
            exit(1);
         }
      }

      if (!fOut.iter.NewEvent(0, subeventslen)) {
         EOUT(("Problem to start event in buffer"));
         return;
      }

      unsigned numstopacq = 0;

      for (unsigned n=0;n<recs.size();n++) {

         if (recs[n].evnt()->iTrigger == mbs::tt_StopAcq) numstopacq++;

         if (n==0)
            fOut.iter.evnt()->CopyHeader(recs[n].evnt());

         dabc::Pointer subevdata;
         recs[n].AssignEventPointer(subevdata);
         subevdata.shift(sizeof(mbs::EventHeader));

         fOut.iter.AddSubevent(subevdata);
      }

      fOut.iter.FinishEvent();

      if (fEvntRate) fEvntRate->AccountValue(1.);

//      DOUT1(("$$$$$$$$ Did event %d size %d", evhdr->iCount, evhdr->DataSize()));

      nevent++;

      unsigned numfinished = 0;

      // shift all pointers to next subevent
      if (nevent<fCfgEventsCombine)
         for (unsigned n=0;n<bufs.size();n++)
           if (!recs[n].NextEvent()) {
              EOUT(("Too few events (%d from %d) in subevent packet from buf %u", nevent, fCfgEventsCombine, n));
              numfinished++;
           }

      if (numstopacq>0) {
         if (numstopacq<bufs.size())
            EOUT(("!!! Not all events has stop ACQ trigger"));

         if ((numfinished<numstopacq) && (nevent<fCfgEventsCombine))
            EOUT(("!!! Not all buffers finished after Stop ACQ  numstopacq:%u  numfinished:%u", numstopacq, numfinished));

         DOUT5(("Flush output buffer and wait until restart"));

         FinishOutputBuffer();

         SendOutputBuffer();

         StopUntilRestart();

         return;
      }

      if (numfinished>0) {
         if (numfinished<bufs.size()) EOUT(("!!!!!!!!! Error - not all subbuffers are finished"));
         return;
      }
   }
}
