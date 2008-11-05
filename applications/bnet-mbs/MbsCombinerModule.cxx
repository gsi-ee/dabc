#include "MbsCombinerModule.h"

#include "dabc/logging.h"
#include "dabc/string.h"
#include "dabc/PoolHandle.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"

#include "bnet/WorkerApplication.h"

bnet::MbsCombinerModule::MbsCombinerModule(const char* name,
                                           WorkerApplication* plugin) :
   FormaterModule(name, plugin),
   fCfgEventsCombine(plugin->CfgEventsCombine()),
   fTransportBufferSize(plugin->TransportBufferSize()),
   fMinEvId(0),
   fMaxEvId(0),
   fUsedEvents(0),
   fEvntRate(0)
{
   InputRec rec;

   for (int n=0;n<NumReadouts();n++) {
      fRecs.push_back(rec);
      for (int k=0;k<fCfgEventsCombine;k++)
         fSubEvnts.push_back(dabc::Pointer());
   }

   int nchannels = fCfgEventsCombine;
   if (nchannels < 5) nchannels = 5;

   fUsedEvents = new dabc::HistogramParameter(this, "PackedEvents", nchannels);
   fUsedEvents->SetLabels("PackedEvents", "Rel.un");

   fEvntRate = CreateRateParameter("EventRate", false, 1.);
   fEvntRate->SetUnits("Ev/s");
   fEvntRate->SetLimits(0, 6000.);
}

bnet::MbsCombinerModule::~MbsCombinerModule()
{
   fRecs.clear();

   DOUT5(("MbsCombinerModule destroyed"));
}

void bnet::MbsCombinerModule::SkipNotUsedBuffers()
{
   for (int ninp = 0; ninp < NumReadouts(); ninp++) {

      // calculate number of buffers, which must be skept from the queue
      // if necessary, last used buffer must be kept
      int nskip = fRecs[ninp].bufindx + 1;
      if (fRecs[ninp].headbuf) nskip--;

      if (nskip>0) {

//         DOUT1(("Skip %d buffers from input %d", nskip, ninp));

         Input(ninp)->SkipInputBuffers(nskip);
         fRecs[ninp].bufindx -= nskip;

//         DOUT1(("Skip done"));
      }
   }
}


dabc::Buffer* bnet::MbsCombinerModule::ProduceOutputBuffer()
{
   // here we check if any event information was accumulated
   // if not, just skip no longer used buffers and return
   if (fMinEvId == fMaxEvId) {
      SkipNotUsedBuffers();
      return 0;
   }

   DOUT3(("Produce output buffer for events %u %u", fMinEvId, fMaxEvId));

   // define here if we can pack all subevents from all readout channel in
   // transport buffer. If not, just take as much as we can

   dabc::BufferSize_t fullbufsize(0), usedbufsize(0);
   int NumUsedEvents = 0;

   for (int nevnt = 0; nevnt < fCfgEventsCombine; nevnt++) {
      for (int ninp = 0; ninp < NumReadouts(); ninp++) {
         int recid = nevnt * NumReadouts() + ninp;

         // if there is no first subevent, skip everything for that event
         if ((ninp==0) && fSubEvnts[recid].null()) {
            DOUT1(("No data for subevent %d", nevnt));
            break;
         }

         fullbufsize += fSubEvnts[recid].fullsize();

         if (ninp>0) fullbufsize -= sizeof(mbs::eMbs101EventHeader);
      }

      if (fullbufsize <= fTransportBufferSize) {
         NumUsedEvents = nevnt+1;
         usedbufsize = fullbufsize;
      }
   }

   fUsedEvents->Fill(NumUsedEvents);

   if (NumUsedEvents < fCfgEventsCombine) {
      EOUT(("Can take only %d events from %d, buffer size %u bigger than transport buf %u",
         NumUsedEvents, fCfgEventsCombine, fullbufsize, fTransportBufferSize));
   }

   dabc::BufferGuard buf = TakeBuffer(fOutPool, usedbufsize);

   dabc::BufferSize_t filledsize = 0;

   dabc::Pointer evptr(buf());

   for (int nevnt = 0; nevnt < NumUsedEvents; nevnt++) {

      int firtsrecid = nevnt * NumReadouts();

      if (fSubEvnts[firtsrecid].null()) continue;

      dabc::Pointer outdataptr;
      mbs::eMbs101EventHeader *evhdr(0), tmp1;
      mbs::eMbs101EventHeader *src_evhdr(0), src_tmp1;

      if (!mbs::StartEvent(evptr, outdataptr, evhdr, &tmp1)) {
         EOUT(("Cannot start event"));
         exit(1);
      }

      if (!mbs::GetEventHeader(fSubEvnts[firtsrecid], src_evhdr, &src_tmp1)) {
         EOUT(("Cannot locate source event header"));
         exit(1);
      }

      evhdr->CopyFrom(*src_evhdr);

      for (int ninp = 0; ninp < NumReadouts(); ninp++) {
         int recid = nevnt * NumReadouts() + ninp;

         dabc::Pointer subevptr(fSubEvnts[recid]);
         // skip now event header
         subevptr += sizeof(mbs::eMbs101EventHeader);

         // copy at one all subevents
         outdataptr.copyfrom(subevptr, subevptr.fullsize());
         outdataptr+=subevptr.fullsize();
      }

      dabc::BufferSize_t len = mbs::FinishEvent(evptr, outdataptr, evhdr, 0);

//      DOUT1(("Finish event %d sz %u", evhdr->iCount, len));

      if (len == dabc::BufferSizeError) {
         EOUT(("Something wrong with combiner"));
         exit(1);
      }

      filledsize += len;
   }

   if (filledsize != usedbufsize) {
      EOUT(("Error when filling data in combiner"));
      exit(1);
   }

   // this is last action with output buffer, send in beginning
   // we create bnet header for the buffer
   buf()->SetDataSize(filledsize);
   buf()->SetTypeId(mbs::mbt_MbsEvs10_1);
   buf()->SetHeaderSize(sizeof(bnet::EventId));
   *((bnet::EventId*) buf()->GetHeader()) = (fMinEvId - 1) / fCfgEventsCombine + 1;

//   DOUT1(("Finish buffer id %d sz %u", (fMinEvId - 1) / fCfgEventsCombine, fullbufsize));

   fMinEvId = 0;
   fMaxEvId = 0;
   for (unsigned n=0;n<fSubEvnts.size();n++)
      fSubEvnts[n].reset();

   SkipNotUsedBuffers();

   return buf.Take();
}


void bnet::MbsCombinerModule::MainLoop()
{
   dabc::BufferGuard outbuf;

   bool isstopacq = false;

   while (TestWorking()) {

      // check output buffer
      if (outbuf()!=0)
         Send(fOutPort, outbuf);

      if (isstopacq) {

         DOUT1(("Send EOL buffer and wait for resume"));
         dabc::BufferGuard eolbuf = fOutPool->TakeEmptyBuffer();
         eolbuf()->SetTypeId(dabc::mbt_EOL);
         Send(fOutPort, eolbuf);

         StopUntilRestart();
         isstopacq = false;
      }

      bool isemptybuffer = false;

      // first check, if some of inputs miss buffer
      for (int ninp=0; ninp<NumReadouts(); ninp++) {

         if (fRecs[ninp].headbuf) continue;

         dabc::Buffer* buf = 0;

         fRecs[ninp].bufindx++;

         if ((int) Input(ninp)->InputPending() == fRecs[ninp].bufindx)
            WaitInput(Input(ninp), fRecs[ninp].bufindx + 1);

         buf = Input(ninp)->InputBuffer(fRecs[ninp].bufindx);

         DOUT5(("Requested buffer from input %d indx %d buf %p", ninp, fRecs[ninp].bufindx, buf));

         if (buf==0) {
            fRecs[ninp].bufindx--;
            EOUT(("Should not happen !!!"));
         } else
         if (buf->GetTypeId() == dabc::mbt_EOF) {
            DOUT1(("Find EOF in the input %d, skip it", ninp));
            buf = 0;
         }

         // check all inputs until the end,
         // next time we will wait forever for next event
         if (buf==0) {
            isemptybuffer = true;
            continue;
         }

         dabc::Pointer bufptr(buf);

//         DOUT1(("Extract first event from %p", buf));

         if (FirstEvent(bufptr, fRecs[ninp].evptr, fRecs[ninp].evhdr, &(fRecs[ninp].tmp_evhdr))) {
            fRecs[ninp].headbuf = buf;

            mbs::sMbsBufferHeader* bufhdr = (mbs::sMbsBufferHeader*) bufptr();

            if (fEvntRate) fEvntRate->AccountValue(bufhdr->iNumEvents);

            DOUT5(("Extract first event from %p done ninp %d hdr %p evid %d", buf, ninp, fRecs[ninp].evhdr, fRecs[ninp].evhdr->iCount));
         } else {
            // no error recovery, just continue
            EOUT(("Get buffer of non MBS-format"));
            isemptybuffer = true;
         }
      }

      if (isemptybuffer) continue;

//      DOUT1(("Get buffer on each input n = %d", fCfgEventsCombine));

      // now check that every input gets the same eventid
      mbs::MbsEventId mineventid = 0;
      mbs::MbsEventId maxeventid = 0;
      int mininp = 0;

      for (int ninp=0; ninp<NumReadouts(); ninp++) {

         mbs::MbsEventId evid = fRecs[ninp].evhdr->iCount;

//         DOUT1(("Extract evid from inp %d hfd %p evid %d", ninp, fRecs[ninp].evhdr, evid));

         if (ninp==0) {
            mineventid = evid;
            maxeventid = evid;
         } else {
            if (mineventid < evid) { mineventid = evid; mininp = ninp; } else
            if (maxeventid > evid) maxeventid = evid;
         }
      }

//      DOUT1(("Min %d max %d", mineventid, maxeventid));

      // if one has not everywhere the same event,
      // skip minimum and continue from the beginning
      if (mineventid < maxeventid) {
         EOUT(("Problem with event ids, skip event on input %d", mininp));

         // if no more events in the buffer, release it
         if (!NextEvent(fRecs[mininp].evptr, fRecs[mininp].evhdr, &(fRecs[mininp].tmp_evhdr))) {
            fRecs[mininp].headbuf = 0;

            // we copy all accumulated data to output buffer when input we using is full
            // we cannot wait longer while data from this input will be blocked
            if (Input(mininp)->InputQueueFull())
               outbuf = ProduceOutputBuffer();
         }

         continue;
      }

      // this is a case of empty list
      if (fMinEvId==fMaxEvId) {
         fMinEvId = (mineventid - 1) / fCfgEventsCombine * fCfgEventsCombine + 1;
         fMaxEvId = fMinEvId + fCfgEventsCombine;
      }

      // in case if event jump out of the preselected range,
      // we should close output buffer and send it over net
      if ((mineventid < fMinEvId) ||
          (mineventid >= fMaxEvId)) {
             outbuf = ProduceOutputBuffer();
             continue;
          }

      for (int ninp=0; ninp<NumReadouts(); ninp++) {

         int recid = (mineventid - fMinEvId) * NumReadouts() + ninp;

//         DOUT1(("Fill data for rec %d  size %d", recid, fRecs[ninp].evhdr->DataSize()));

         // assign pointer on the region, where subevent is situated
         fSubEvnts[recid].reset(fRecs[ninp].evptr, fRecs[ninp].evhdr->DataSize());

         if (fRecs[ninp].evhdr->iTrigger==mbs::tt_StopAcq) {
            isstopacq = true;
         }

         if (!NextEvent(fRecs[ninp].evptr, fRecs[ninp].evhdr, &(fRecs[ninp].tmp_evhdr)))
            fRecs[ninp].headbuf = 0;
      }

      if (isstopacq) {
         DOUT1(("See stop ACQ - flush buffer at event %d", mineventid));
      }

      // if we fill last event, just produce output buffer
      if ((mineventid == fMaxEvId-1) || isstopacq)
         outbuf = ProduceOutputBuffer();
   }
}

