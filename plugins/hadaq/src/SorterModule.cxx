// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "hadaq/SorterModule.h"

#include <algorithm>

#include "dabc/Manager.h"

#include "hadaq/HadaqTypeDefs.h"
#include "hadaq/Iterator.h"


hadaq::SorterModule::SorterModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fFlushCnt(5),
   fBufCnt(0),
   fLastRet(false),
   fNextBufIndx(0),
   fReadyBufIndx(0),
   fSubs(),
   fOutBuf(),
   fOutPtr()
{
   // we need at least one input and one output port
   EnsurePorts(1, 1, dabc::xmlWorkPool);

   double flushtime = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(0.3);

   if (flushtime > 0.)
      CreateTimer("FlushTimer", flushtime, false);

   fTriggersRange = Cfg(hadaq::xmlHadaqTrignumRange, cmd).AsUInt(0x1000000);
   fLastTrigger = 0xffffffff;

   fSubs.reserve(1024);
}

void hadaq::SorterModule::DecremntInputIndex(unsigned cnt)
{
   // remove *cnt* buffers from the input queue
   // all references should be removed

   if (fNextBufIndx>cnt) fNextBufIndx-=cnt; else fNextBufIndx = 0;
   if (fReadyBufIndx>cnt) fReadyBufIndx-=cnt; else fReadyBufIndx = 0;

   unsigned tgt(0);

   for (unsigned n=0;n<fSubs.size();n++) {
      if (fSubs[n].buf<cnt) continue;
      fSubs[n].buf-=cnt;
      if (n!=tgt) fSubs[tgt] = fSubs[n];
      tgt++;
   }

   fSubs.resize(tgt);
}

bool hadaq::SorterModule::RemoveUsedSubevents(unsigned num)
{
   // remove used entries from subs list
   // return true if any buffer from inp

   if (num==0) return false;

   if (num >= fSubs.size()) {
      // this is full clear of indexed data
      unsigned maxbuf(0);
      for (unsigned n=0;n<fSubs.size();n++)
         if (fSubs[n].buf>maxbuf) maxbuf = fSubs[n].buf;

      fSubs.clear();

      DecremntInputIndex(maxbuf+1);

      SkipInputBuffers(0, maxbuf+1);

      return true;
   }

   unsigned minbuf(0xffffffff);

   // first check indexes of buffer which are removed
   for (unsigned n=num;n<fSubs.size();n++) {
      if (fSubs[n].buf<minbuf) minbuf = fSubs[n].buf;
      fSubs[n-num] = fSubs[n];
   }
   fSubs.resize(fSubs.size() - num);

   if (minbuf>0) {
      // we skip at least one buffer
      DecremntInputIndex(minbuf);
      SkipInputBuffers(0, minbuf);
      return true; // indicate that buffers were removed
   }

   return false; // no buffers removed
}


bool hadaq::SorterModule::retransmit()
{
   bool new_data = false, full_recv_queue = RecvQueueFull(), flush_data = false;

   while (fNextBufIndx < NumCanRecv()) {

      // remember state of the queue before we access it
      full_recv_queue = RecvQueueFull();

      dabc::Buffer buf = RecvQueueItem(0, fNextBufIndx);

      // special handling for EOF buffer
      // either flush all data or just forward EOF buffer
      if (buf.GetTypeId()==dabc::mbt_EOF) {
         if (fNextBufIndx==0) {
            if (!CanSend()) { fLastRet = 50; return false; }
            buf = Recv();
            DecremntInputIndex();
            Send(buf);
            fFlushCnt = 5;
            fLastRet = 40;
            return true;
         }
         flush_data = true; break;
      }

      hadaq::ReadIterator iter(buf);
      fBufCnt++;
      bool was_empty = fSubs.size() == 0;

      // scan buffer
      while (iter.NextSubeventsBlock())
         while (iter.NextSubEvent()) {
            SubsRec rec;
            rec.subevnt = iter.subevnt();
            rec.buf = fNextBufIndx;
            rec.trig = (iter.subevnt()->GetTrigNr() >> 8) & (fTriggersRange-1);
            rec.sz = iter.subevnt()->GetPaddedSize();

            // DOUT1("Event 0x%06x size %3u", rec.trig, rec.sz);

            fSubs.push_back(rec);
            new_data = true;
         }

      // check if buffer can be used as is
      // all ids are in the order and corresponds to previous values
      if ((fReadyBufIndx == fNextBufIndx) && was_empty) {
         uint32_t prev = fLastTrigger;
         bool ok(true);
         for (unsigned n=0;n<fSubs.size();n++) {
            if (prev!=0xffffffff) {
               ok = Diff(prev, fSubs[n].trig)==1;
               if (!ok) break;
            }
            prev = fSubs[n].trig;
         }

         if (ok) {
            fLastTrigger = prev;
            fReadyBufIndx++;
            fSubs.clear(); // no need to keep array
            new_data = false;
         }
      }

      fNextBufIndx++;
   }

   // sort current array
   if (new_data)
      std::sort(fSubs.begin(), fSubs.end(), SubsComp(this));

   // simple case - retransmit buffer from input to output
   if ((fReadyBufIndx>0) && CanSend() && CanRecv()) {
      dabc::Buffer buf = Recv();
      DecremntInputIndex();
      Send(buf);
      fFlushCnt = 5;
      fLastRet = 30;
      return true;
   }

   // no need to try if cannot send buffer
   if (!CanSend()) { fLastRet = 20; return false; }

   if (fOutBuf.null()) {
      if (!CanTakeBuffer()) { fLastRet = 10; return false; }
      fOutBuf = TakeBuffer();
      fOutBuf.SetTypeId(hadaq::mbt_HadaqSubevents);
      fOutPtr.reset(fOutBuf);
   }

   // one could allow gaps in the trigger IDs if more than 2 items in the input queue
   unsigned cnt = 0;
   while (cnt < fSubs.size()) {
      int diff = 1;
      if (fLastTrigger!=0xffffffff)
         diff = Diff(fLastTrigger, fSubs[cnt].trig);

      if (diff!=1) {

         if (diff<0) {
            EOUT("Buf:%3d problem in sorting - older events appeared. Most probably, flush time has wrong value", fBufCnt);
            cnt++; // skip subevent
            continue;
         }

         // if buffer for such subevents in two last buffers, wait for next data
         // if EOF buffer was seen before, flush subevents immediately
         if ((fSubs[cnt].buf + 2 > fNextBufIndx) && !full_recv_queue && !flush_data) break;

         DOUT3("Buf:%3d  Saw difference %d with trigger 0x%06x cnt:%u", fBufCnt, diff, fSubs[cnt].trig, fOutPtr.distance_to_ownbuf());

         DOUT3("Allow gap full:%s numcanrecv:%u indx:%u nextbufind:%u", DBOOL(full_recv_queue), NumCanRecv(), fSubs[cnt].buf, fNextBufIndx);

         // even after the gap, event taken into output buffer
      }

      // check if output buffer has enough space
      if (fOutPtr.fullsize() < fSubs[cnt].sz) { flush_data = true; break; }

      memcpy(fOutPtr(), fSubs[cnt].subevnt,  fSubs[cnt].sz);
      fOutPtr.shift(fSubs[cnt].sz);

      fLastTrigger = fSubs[cnt++].trig;
   }

   if (full_recv_queue) flush_data = true;

   if (flush_data && (fOutPtr.distance_to_ownbuf()>0)) {
      fOutBuf.SetTotalSize(fOutPtr.distance_to_ownbuf());
      fOutPtr.reset();
      Send(fOutBuf);
      fFlushCnt = 5;
   }

   // if buffers were removed from input queue, call retransmit again
   if (RemoveUsedSubevents(cnt)) flush_data = true;

   fLastRet = 7;

   return true;
}


void hadaq::SorterModule::ProcessTimerEvent(unsigned)
{
   // timer events used for data flush
   // if after 3 timer events no data was send, any data filled into output buffer will be send
   // if nothing happened after 6 timer events, any indexed data will be placed into output buffer and send

   if (!CanSend()) return; // first of all, check if we can send data

   if (--fFlushCnt > 2) return;

   // flush buffer if any data is accumulated
   unsigned len = fOutPtr.distance_to_ownbuf();
   if (len>0) {
      DOUT1("Buf:%3d  Flush output counter %d subs.size %u nextbuf:%u numcanrev:%u lastret:%d", fBufCnt, fFlushCnt, fSubs.size(), fNextBufIndx, NumCanRecv(), fLastRet);
      fOutBuf.SetTotalSize(len);
      fOutPtr.reset();
      Send(fOutBuf);
      fFlushCnt = 5;
      return;
   }

   if (fFlushCnt >= 0) return;
   // send any remained data and clear buffers

}

int hadaq::SorterModule::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("GetHadaqTransportInfo")) {
      if (SubmitCommandToTransport(InputName(0), cmd)) return dabc::cmd_postponed;
      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}
