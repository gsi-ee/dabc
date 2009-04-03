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
#include "roc/CombinerModule.h"

#include "roc/Device.h"
#include "nxyter/Data.h"

#include "dabc/logging.h"
#include "dabc/PoolHandle.h"
#include "dabc/MemoryPool.h"
#include "dabc/Command.h"
#include "dabc/Port.h"
#include "dabc/Pointer.h"
#include "dabc/Manager.h"

#include "mbs/LmdTypeDefs.h"
#include "mbs/MbsTypeDefs.h"

roc::CombinerModule::CombinerModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleAsync(name, cmd),
   fPool(0),
   fOutBuf(0),
   f_outptr()
{
   int numrocs = GetCfgInt(roc::xmlNumRocs, 1, cmd);
   fBufferSize = GetCfgInt(dabc::xmlBufferSize, 16384, cmd);
   int numoutputs = GetCfgInt(dabc::xmlNumOutputs, 2, cmd);

   dabc::RateParameter* r = CreateRateParameter("CombinerRate", false, 3., "", "", "MB/s", 0., 10.);
   r->SetDebugOutput(true);

   DOUT1(("new roc::CombinerModule %s buff %d", GetName(), fBufferSize));
   fPool = CreatePoolHandle(roc::xmlRocPool, fBufferSize, 1);
   for(int inp=0; inp < numrocs; inp++)  {
      CreateInput(FORMAT(("Input%d", inp)), fPool, 10);
      Input(inp)->SetInpRateMeter(r);
      fInp.push_back(InputRec());
   }

   for(int n=0; n<numoutputs; n++)
      CreateOutput(FORMAT(("Output%d", n)), fPool, 10);

   fSimpleMode = false;

   fiEventCnt = 0;

   fErrorOutput = false;
}

roc::CombinerModule::~CombinerModule()
{
   dabc::Buffer::Release(fOutBuf); fOutBuf = 0;
}

void roc::CombinerModule::ProcessInputEvent(dabc::Port* inport)
{
   unsigned inpid = InputNumber(inport);

   if (!inport->CanRecv()) {
      EOUT(("Something wrong with input %u %s", inpid, inport->GetName()));
      return;
   }

   if (fSimpleMode) {

      dabc::Buffer* buf = inport->Recv();

      if (Output(0)->CanSend())
         Output(0)->Send(buf);
      else
         dabc::Buffer::Release(buf);

      return;
   }

//   DOUT3(("Get new buffer in input %u ready %s !!!", inpid, DBOOL(fInp[inpid].isready)));

   FindNextEvent(inpid);

   TryToProduceEventBuffers();
}

bool roc::CombinerModule::FindNextEvent(unsigned ninp)
{
   dabc::Buffer* buf = 0;

   InputRec* rec = &(fInp[ninp]);

   if (rec->isready) return true;

   nxyter::Data databuf, *data(0);

   while ((buf = Input(ninp)->InputBuffer(rec->curr_nbuf)) != 0) {
      if (rec->curr_indx >= buf->GetTotalSize()) {
         if (rec->isprev || rec->isnext)
            rec->curr_nbuf++;
         else {
            // no need to keep this buffer in place if no epoch was found
            Input(ninp)->SkipInputBuffers(rec->curr_nbuf);
            rec->curr_nbuf = 0;
         }
         rec->curr_indx = 0;
         continue;
      }

//      bool dodump = (rec->curr_indx == 0);

      dabc::Pointer ptr(buf); ptr.shift(rec->curr_indx);

//      if (dodump) DumpData(ptr);

      while (ptr.fullsize()>=6) {
         if (ptr.rawsize()>=6) {
            data = (nxyter::Data*) ptr();
         } else {
            EOUT(("Segmented nxyter::Data ???"));
            ptr.copyto(&databuf, 6);
            data = &databuf;
         }

         if (data->isEpochMsg()) { // epoch marker
            rec->curr_epoch = data->getEpoch();
            rec->rocid = data->getRocNumber();
            rec->iscurrepoch = true;
         } else
         if (data->isSyncMsg()) {
            if (!rec->iscurrepoch) {
               EOUT(("Found SYNC marker %6x without epoch", data->getSyncEvNum()));
            } else {
               if (!rec->isprev) {
                  rec->prev_epoch = rec->curr_epoch;
                  rec->isprev = true;
                  rec->prev_nbuf = rec->curr_nbuf;
                  rec->prev_indx = rec->curr_indx;
                  rec->prev_evnt = data->getSyncEvNum();
                  rec->prev_stamp = data->getSyncTs();
               } else {
                  rec->next_epoch = rec->curr_epoch;
                  rec->isnext = true;
                  rec->next_nbuf = rec->curr_nbuf;
                  rec->next_indx = rec->curr_indx;
                  rec->next_evnt = data->getSyncEvNum();
                  rec->next_stamp = data->getSyncTs();
               }
            } // iscurrepoch
         }

         ptr.shift(6);
         rec->curr_indx+=6;

         if ((ptr.fullsize()<6) && (ptr.fullsize()!=0))
            EOUT(("Roc:%d strange rest size %d", ninp, ptr.fullsize()));

         if (rec->isprev && rec->isnext) {

//            DOUT1(("Tm: %7.5f Rocid:%u Find sync events %u - %u between %u:%u and %u:%u",
//                    TimeStamp()*1e-6, rec->rocid, rec->prev_evnt, rec->next_evnt,
//                    rec->prev_nbuf, rec->prev_indx, rec->next_nbuf, rec->next_indx));

            rec->isready = true;
            return true;
         }
      }
   }

   return false;
}


void roc::CombinerModule::ProcessOutputEvent(dabc::Port* inport)
{
   if (fSimpleMode) return;

   TryToProduceEventBuffers();
}


bool roc::CombinerModule::SkipEvent(unsigned inpid)
{
   InputRec* rec = &(fInp[inpid]);
   if (!rec->isprev || !rec->isnext) return false;

   rec->isready    = false;
   rec->isprev     = true;
   rec->prev_epoch = rec->next_epoch;
   rec->prev_nbuf  = rec->next_nbuf;
   rec->prev_indx  = rec->next_indx;
   rec->prev_evnt  = rec->next_evnt;
   rec->prev_stamp = rec->next_stamp;

   rec->isnext     = false;

   if (rec->prev_nbuf>0) {
      Input(inpid)->SkipInputBuffers(rec->prev_nbuf);
      rec->curr_nbuf -= rec->prev_nbuf;
      rec->prev_nbuf = 0;
   }

//   DOUT4(("Skip event done, search next from %u:%u",
//            rec->curr_nbuf, rec->curr_indx));

   return FindNextEvent(inpid);
}


void roc::CombinerModule::TryToProduceEventBuffers()
{
   // method fill output buffer with complete sync event from all
   // available sources and makes correction against master (first roc)

   bool doagain = true;

   uint32_t evnt_cut = 0; //

   while (doagain) {

      doagain = false;

      uint32_t min_evnt(0xffffffL), max_evnt(0); // 24 bits

      for (unsigned ninp = 0; ninp<NumInputs(); ninp++) {
         // one should have at least epoch mark define
         InputRec* inp = &(fInp[ninp]);

         if (!inp->isprev) return;

         if (inp->prev_evnt < evnt_cut) continue;

         if (inp->prev_evnt < min_evnt) min_evnt = inp->prev_evnt;
         if (inp->prev_evnt > max_evnt) max_evnt = inp->prev_evnt;
      }

      if (min_evnt > max_evnt) {
         EOUT(("Nothing found!!!"));
         return;
      }

      // try to detect case of upper border crossong
      if (min_evnt != max_evnt)
         if ((min_evnt < 0x10000) && (max_evnt > 0xff0000L)) {
            evnt_cut = 0x800000;
            doagain = true;
            continue;
         }

      evnt_cut = 0;

      dabc::BufferSize_t totalsize = sizeof(mbs::EventHeader);

//      DOUT3(("Try to prepare for event %u", min_evnt));

      for (unsigned ninp = 0; ninp<NumInputs(); ninp++) {

         InputRec* inp = &(fInp[ninp]);

         inp->use = (inp->prev_evnt == min_evnt);
         inp->data_err = false;

         if (!inp->use) continue;

         // one should have next event completed to use it for combining
         if (!inp->isready) return;

         if (ninp==0) {
             if  (inp->prev_evnt != (inp->prev_epoch & 0xffffff)) {
                if (fErrorOutput)
                   EOUT(("Missmtach in epoch %8x and event number %6x for ROC0",
                         inp->prev_epoch, inp->prev_evnt));
//                inp->data_err = true;
             }

             if  (inp->next_evnt != (inp->next_epoch & 0xffffff)) {
                if (fErrorOutput)
                   EOUT(("Missmtach in epoch %8x and event number %6x for ROC0",
                         inp->next_epoch, inp->next_evnt));
//                inp->data_err = true;
             }
         } else
         if (!fInp[0].use || fInp[0].data_err) {
             inp->data_err = true;
         } else {
            if (inp->next_evnt != fInp[0].next_evnt) {
//               if (fErrorOutput)
                  EOUT(("Next event missmatch between ROC0:%06x and ROC%u:%06x",
                        fInp[0].next_evnt, ninp, inp->next_evnt));
               inp->data_err = true;
            }
         }

         totalsize += sizeof(mbs::SubeventHeader);

         totalsize += CalcDistance(ninp,
              inp->prev_nbuf, inp->prev_indx,
              inp->next_nbuf, inp->next_indx);

         // one should copy expoch of previos event and next event as well
         totalsize += 6 + 6;
      }

//      if (fInp.size()>1)
//         DOUT0(("Roc0:%06x Roc1:%06x %s",
//                 fInp[0].next_evnt, fInp[1].next_evnt,
//                 (fInp[0].data_err || fInp[1].data_err) ? "Error" : ""));

      bool skip_evnt = false;

      bool neednewbuf = fOutBuf==0;

      if (!neednewbuf)
         neednewbuf = totalsize > f_outptr.fullsize();

      if (neednewbuf) {
         dabc::BufferSize_t usedsize = 0;

         if (fOutBuf!=0) usedsize = fOutBuf->GetDataSize() - f_outptr.fullsize();

         if ((fOutBuf!=0) && (usedsize>0)) {

            // all outputs must be able to get buffer for sending
            if (!CanSendToAllOutputs()) return;

            fOutBuf->SetDataSize(usedsize);
            fOutBuf->SetTypeId(mbs::mbt_MbsEvents);

            SendToAllOutputs(fOutBuf);

            fOutBuf = 0;
            f_outptr.reset();
         }

         if (fOutBuf==0) {
            fOutBuf = fPool->TakeBuffer(fBufferSize);
            if (fOutBuf) fOutBuf->SetDataSize(fBufferSize);
         }

         if (fOutBuf==0) {
            EOUT(("Cannot get buffer from pool"));
            return;
         }

         if (fOutBuf->GetDataSize() < totalsize) {
            EOUT(("Cannot put event in buffer - skip event"));
            skip_evnt = true;
         }

         f_outptr.reset(fOutBuf);
      }

      if (!skip_evnt) {
         dabc::Pointer old(f_outptr);

//         DOUT5(("Start MBS event at pos %u", dabc::Pointer(fOutBuf).distance_to(f_outptr)));

         mbs::EventHeader* evhdr = (mbs::EventHeader*) f_outptr();
         evhdr->Init(fiEventCnt++);
         f_outptr.shift(sizeof(mbs::EventHeader));

//         DOUT1(("Fill event %7u  ~size %6u inp0 %ld outsize:%u",
//            min_evnt, totalsize, Input(0)->InputPending(), f_outptr.fullsize()));

//         DOUT1(("Distance to data %u", dabc::Pointer(fOutBuf).distance_to(f_dataptr)));

         unsigned filled_sz = FillRawSubeventsBuffer(f_outptr);

         if (filled_sz == 0) {
             EOUT(("Event data not filled - event skiped!!!"));
             f_outptr.reset(old);
         } else {
            evhdr->SetSubEventsSize(filled_sz);
         }
      }

      doagain = true;
      for (unsigned ninp = 0; ninp<NumInputs(); ninp++)
         if (fInp[ninp].use)
            if (!SkipEvent(ninp)) doagain = false;
   }
}


dabc::BufferSize_t roc::CombinerModule::CalcDistance(unsigned ninp, unsigned nbuf1, unsigned indx1, unsigned nbuf2, unsigned indx2)
{
   dabc::BufferSize_t datasize = 0;

   while (nbuf1<nbuf2) {
      dabc::Buffer* buf = Input(ninp)->InputBuffer(nbuf1);
      if (buf!=0) datasize += (buf->GetTotalSize()-indx1);
      indx1 = 0;
      nbuf1++;
   }

   datasize+= indx2-indx1;

   return datasize;
}

unsigned roc::CombinerModule::FillRawSubeventsBuffer(dabc::Pointer& outptr)
{
   unsigned filled_size = 0;

   for (unsigned ninp = 0; ninp<NumInputs(); ninp++) {
      InputRec* rec = &(fInp[ninp]);
      if (!rec->use) continue;

      mbs::SubeventHeader* subhdr = (mbs::SubeventHeader*) outptr();
      subhdr->Init();
      subhdr->iProcId = rec->data_err ? roc::proc_ErrEvent : roc::proc_RocEvent;
      subhdr->iSubcrate = rec->rocid;

      outptr.shift(sizeof(mbs::SubeventHeader));
      filled_size+=sizeof(mbs::SubeventHeader);

      unsigned subeventsize = 6;

      nxyter::Data* data = (nxyter::Data*) outptr();
      data->setMessageType(ROC_MSG_EPOCH);
      data->setRocNumber(rec->rocid);
      data->setEpoch(rec->prev_epoch);
      data->setMissed(0);
      outptr.shift(6);

      unsigned nbuf = rec->prev_nbuf;

      while (nbuf<=rec->next_nbuf) {
         dabc::Buffer* buf = Input(ninp)->InputBuffer(nbuf);
         if (buf==0) {
            EOUT(("Internal error"));
            return 0;
         }

         // if next epoch in the same buffer, limit its size by next_indx + 6 (with next event)
         dabc::Pointer ptr(buf);
         if (nbuf == rec->next_nbuf) ptr.setfullsize(rec->next_indx + 6);
         if (nbuf == rec->prev_nbuf) ptr.shift(rec->prev_indx);

         outptr.copyfrom(ptr, ptr.fullsize());
         outptr.shift(ptr.fullsize());
         subeventsize += ptr.fullsize();
         nbuf++;
      }

      filled_size+=subeventsize;
      subhdr->SetRawDataSize(subeventsize);

      data = (nxyter::Data*) subhdr->RawData();

      if (data->getMessageType() != ROC_MSG_EPOCH)
         EOUT(("internal error !!!!!!"));

//      DOUT1(("Subevent size %u", subhdr->FullSize()));
   }

   return filled_size;
}

void roc::CombinerModule::DumpData(dabc::Pointer ptr)
{
   while (ptr.fullsize()>=6) {
      nxyter::Data* data = (nxyter::Data*) ptr();
      data->printData(3);
      ptr.shift(6);
   }
}
