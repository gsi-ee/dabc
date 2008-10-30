#include "MbsBuilderModule.h"

#include "dabc/logging.h"
#include "dabc/PoolHandle.h"
#include "dabc/Command.h"
#include "dabc/Port.h"
#include "dabc/Parameter.h"

#include "bnet/common.h"
#include "bnet/WorkerPlugin.h"

bnet::MbsBuilderModule::MbsBuilderModule(dabc::Manager* m, const char* name, WorkerPlugin* plugin) :
   BuilderModule(m, name, plugin),
   fCfgEventsCombine(plugin->CfgEventsCombine()),
   fEvntRate(0)
{
   fOut.buf = 0;
   fOut.ready = false;
   
   fEvntRate = CreateRateParameter("EventRate", false, 1.);
   fEvntRate->SetUnits("Ev/s");
   fEvntRate->SetLimits(0, 6000.);
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
      bufsize += sizeof(mbs::sMbsBufferHeader);
      
      if (bufsize<fOutBufferSize) bufsize = fOutBufferSize;
      
      fOut.buf = TakeBuffer(fOutPool, bufsize);
      
      if (!StartBuffer(fOut.buf, fOut.evptr, fOut.bufhdr, &fOut.tmp_bufhdr)) {
         EOUT(("Problem to start with MBS buffer"));
         exit(1);
      }
   }
}

void bnet::MbsBuilderModule::FinishOutputBuffer()
{
   dabc::BufferSize_t reslen = FinishBuffer(fOut.buf, fOut.evptr, fOut.bufhdr);
   
   if (reslen == dabc::BufferSizeError) {
      EOUT(("Missmatch in buffer size calculations"));
      exit(1);
   }
   
//   DOUT1(("Build buffer of size %d", outlen));

   if (fEvntRate) fEvntRate->AccountValue(fOut.bufhdr->iNumEvents);
   
   fOut.ready = true;
}

void bnet::MbsBuilderModule::SendOutputBuffer()
{
   dabc::Buffer* buf = fOut.buf;
   fOut.ready = false;
   fOut.buf = 0;
   Send(Output(0), buf);
}

typedef struct BufRec {
   dabc::Pointer evptr;
   mbs::eMbs101EventHeader *evhdr;
   mbs::eMbs101EventHeader tmp;
};

void bnet::MbsBuilderModule::DoBuildEvent(std::vector<dabc::Buffer*>& bufs)
{
//   DOUT1(("start DoBuildEvent n = %d", fCfgEventsCombine));

   std::vector<BufRec> recs;
   recs.resize(bufs.size());
   
   for (unsigned n=0;n<bufs.size();n++) {
       
      if (bufs[n]==0) {
         EOUT(("Buffer %d is NULL !!!!!!!!!!!!!!!", n));
         exit(1);  
      } 
       
      recs[n].evptr.reset(bufs[n]);
      recs[n].evhdr = 0;
      if (!mbs::GetEventHeader(recs[n].evptr, recs[n].evhdr, &(recs[n].tmp))) {
         EOUT(("Invalid MBS format on buffer %u size: %u", n, bufs[n]->GetTotalSize()));
         return;
      }
   }
   
   int nevent = 0;
   
   while (nevent<fCfgEventsCombine) {
      unsigned pmin(0), pmax(0);

      dabc::BufferSize_t fulleventlen = 0;

      for (unsigned n=0;n<recs.size();n++) {
         if (recs[n].evhdr->iCount < recs[pmin].evhdr->iCount) pmin = n; else
         if (recs[n].evhdr->iCount > recs[pmax].evhdr->iCount) pmax = n;
         
         if (n==0)
            fulleventlen += recs[n].evhdr->DataSize();
         else   
            fulleventlen += recs[n].evhdr->SubeventsDataSize();
      }
      
      if (recs[pmin].evhdr->iCount < recs[pmax].evhdr->iCount) {
         EOUT(("Skip subevent %u from buffer %u", recs[pmin].evhdr->iCount, pmin));
         if (!NextEvent(recs[pmin].evptr, recs[pmin].evhdr, &(recs[pmin].tmp))) return;
         // try to analyse events now
         continue;
      }
      
      if (fulleventlen==0) {
         EOUT(("Something wrong with data"));
         return;
      }

      // if rest of existing buffer less than new event data, close it
      if (!fOut.ready && fOut.buf && (fOut.evptr.fullsize() < fulleventlen)) 
         FinishOutputBuffer();
         
      // send buffer, if it is filled and ready to send 
      if (fOut.ready) SendOutputBuffer();
      
      // start new buffer if required
      if ((fOut.buf==0) && !fOut.ready) {
         StartOutputBuffer(fulleventlen);
         
         if (fOut.evptr.fullsize() < fulleventlen) {
            EOUT(("Single event do not pass in to the buffers"));
            exit(1);  
         }
      }

      dabc::Pointer outdataptr;
      
      mbs::eMbs101EventHeader *evhdr(0), tmp_evhdr;
      
      if (!mbs::StartEvent(fOut.evptr, outdataptr, evhdr, &tmp_evhdr)) {
         EOUT(("Problem to start event in buffer"));
         return;   
      }
      
      unsigned numstopacq = 0;

      for (unsigned n=0;n<recs.size();n++) {

         if (recs[n].evhdr->iTrigger == mbs::tt_StopAcq) numstopacq++;

         if (n==0) 
            evhdr->CopyFrom(*(recs[n].evhdr));
         
         dabc::Pointer subevdata(recs[n].evptr);
         subevdata += sizeof(mbs::eMbs101EventHeader);
         
         // copy all subevents without header
         outdataptr.copyfrom(subevdata, recs[n].evhdr->SubeventsDataSize());
         outdataptr += recs[n].evhdr->SubeventsDataSize();
      }

      mbs::FinishEvent(fOut.evptr, outdataptr, evhdr, fOut.bufhdr);
      
//      if (fEvntRate) fEvntRate->AccountValue(1.);
      
//      DOUT1(("$$$$$$$$ Did event %d size %d", evhdr->iCount, evhdr->DataSize()));
       
      nevent++; 
      
      unsigned numfinished = 0;
      
      // shift all pointers to next subevent
      if (nevent<fCfgEventsCombine)
         for (unsigned n=0;n<bufs.size();n++) 
           if (!mbs::NextEvent(recs[n].evptr, recs[n].evhdr, &(recs[n].tmp))) {
              EOUT(("Too few events (%d from %d) in subevent packet from buf %u", nevent, fCfgEventsCombine, n));
              numfinished++;  
           }
      
      if (numstopacq>0) {
         if (numstopacq<bufs.size())
            EOUT(("!!! Not all events has stop ACQ trigger"));
            
         if ((numfinished<numstopacq) && (nevent<fCfgEventsCombine))
            EOUT(("!!! Not all buffers finished after Stop ACQ  numstopacq:%u  numfinished:%u", numstopacq, numfinished));
            
         DOUT1(("Flush output buffer and wait until restart"));   
            
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


void bnet::MbsBuilderModule::BeforeModuleStart()
{
}

void bnet::MbsBuilderModule::AfterModuleStop()
{
}

