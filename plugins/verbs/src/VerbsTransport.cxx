#include "dabc/VerbsTransport.h"

#include "dabc/logging.h"
#include "dabc/Port.h"

#include "dabc/VerbsDevice.h"
#include "dabc/VerbsQP.h"
#include "dabc/VerbsCQ.h"
#include "dabc/VerbsMemoryPool.h"

dabc::VerbsTransport::VerbsTransport(VerbsDevice* verbs, VerbsCQ* cq, VerbsQP* qp, Port* port) :
   NetworkTransport(verbs),
   VerbsProcessor(qp),
   fCQ(cq),
   fPoolReg(0),
   f_rwr(0),
   f_swr(0),
   f_sge(0),
   fHeadersPool(0),
   fSegmPerOper(2),
   fFastPost(true)
{
   Init(port);
   
   fFastPost = VerbsDevice::IsThreadSafeVerbs();

   if (fPool!=0) {
      fPoolReg = verbs->RegisterPool(fPool);
   } else
      EOUT(("Cannot make verbs transport without memory pool"));

   if (fNumRecs>0) {
      fHeadersPool = new VerbsMemoryPool(verbs, "HeadersPool", fNumRecs, fFullHeaderSize, false, true);

      // we use at least 2 segments per operation, one for header and one for buffer itself
      fSegmPerOper = fQP->NumSendSegs();
      if (fSegmPerOper<2) fSegmPerOper = 2; 
       
      f_rwr = new ibv_recv_wr [fNumRecs];
      f_swr = new ibv_send_wr [fNumRecs];
      f_sge = new ibv_sge [fNumRecs*fSegmPerOper]; 

      for (uint32_t n=0;n<fNumRecs;n++) {
         
         SetRecHeader(n, fHeadersPool->GetBufferLocation(n));
         
         for (unsigned seg_cnt=0; seg_cnt<fSegmPerOper; seg_cnt++) {
            unsigned nseg = n*fSegmPerOper + seg_cnt;
            f_sge[nseg].addr   = (uintptr_t) 0; // must be specified later
            f_sge[nseg].length = 0; // must be specified later
            f_sge[nseg].lkey   = 0; // must be specified later
         }
         
         f_swr[n].wr_id    = 0; // must be set later
         f_swr[n].sg_list  = 0;
         f_swr[n].num_sge  = 1;
         f_swr[n].opcode   = IBV_WR_SEND; 
         f_swr[n].next     = NULL;
         f_swr[n].send_flags = IBV_SEND_SIGNALED;
         
         f_rwr[n].wr_id     = 0; // must be set later
         f_rwr[n].sg_list   = 0;
         f_rwr[n].num_sge   = 1;
         f_rwr[n].next      = NULL;
      }
   }
}

dabc::VerbsTransport::~VerbsTransport()
{
   DOUT3(("VerbsTransport::~VerbsTransport %p starts", this));


   // we need this while at some point verbs thread will try to access
   // qp, which is destroyed at that moment
   RemoveProcessorFromThread(true); 

   DOUT3(("VerbsTransport::~VerbsTransport %p id: %d locked:%s", this, GetId(), DBOOL(fMutex.IsLocked())));
    
   VerbsQP* delqp = 0; 
    
   { 
      LockGuard guard(fMutex); 
      delqp = fQP; fQP = 0;
   }
   
   delete delqp;
   
   delete fCQ; fCQ = 0;

   delete[] f_rwr; f_rwr = 0;
   delete[] f_swr; f_swr = 0;
   delete[] f_sge; f_sge = 0;

   DOUT3(("VerbsTransport::~VerbsTransport %p id %d Close", this, GetId()));
   
   Cleanup();

   DOUT3(("VerbsTransport::~VerbsTransport %p id %d delete headers pool", this, GetId()));
   
   delete fHeadersPool; fHeadersPool = 0;

   DOUT3(("VerbsTransport::~VerbsTransport %p unregister pool"));

   ((VerbsDevice*) fDevice)->UnregisterPool(fPoolReg);
   fPoolReg = 0;

   DOUT3(("VerbsTransport::~VerbsTransport %p done", this));

}

void dabc::VerbsTransport::_SubmitRecv(uint32_t recid)
{
   // only for RC or UC, not work for UD
   
//   DOUT1(("_SubmitRecv %u", recid));
   
   uint32_t segid = recid*fSegmPerOper; 
   
   Buffer* buf = (Buffer*) fRecs[recid].buf;

   f_rwr[recid].wr_id     = recid; 
   f_rwr[recid].sg_list   = &(f_sge[segid]);
   f_rwr[recid].num_sge   = 1;
   f_rwr[recid].next      = NULL;
   
   f_sge[segid].addr = (uintptr_t) fRecs[recid].header;
   f_sge[segid].length = fFullHeaderSize;
   f_sge[segid].lkey = fHeadersPool->GetLkey(recid);

   if ((buf!=0) && (fPoolReg!=0)) {

      if (buf->NumSegments() >= fSegmPerOper) {
         EOUT(("Too many segments")); 
         exit(1);
      }
       
      LockGuard lock(fPoolReg->WorkMutex());
      
      fPoolReg->_CheckMRStructure(); 
   
      for (unsigned seg=0;seg<buf->NumSegments();seg++) {
         f_sge[segid+1+seg].addr   = (uintptr_t) buf->GetDataLocation(seg);
         f_sge[segid+1+seg].length = buf->GetDataSize(seg);
         f_sge[segid+1+seg].lkey   = fPoolReg->GetLkey(buf->GetId(seg));
      }
      
      f_rwr[recid].num_sge  += buf->NumSegments();
   }

   if (fFastPost)
      fQP->Post_Recv(&(f_rwr[recid]));
   else
      FireEvent(evntVerbsRecvRec, recid);
}

void dabc::VerbsTransport::_SubmitSend(uint32_t recid)
{
//   DOUT1(("_SubmitSend %u buf:%p", recid, fRecs[recid].buf));

   uint32_t segid = recid*fSegmPerOper;

   int hdrsize = PackHeader(recid);
   
   f_sge[segid].addr = (uintptr_t) fRecs[recid].header;
   f_sge[segid].length = hdrsize;
   f_sge[segid].lkey = fHeadersPool->GetLkey(recid);

   f_swr[recid].wr_id    = recid;
   f_swr[recid].sg_list  = &(f_sge[segid]);
   f_swr[recid].num_sge  = 1;
   f_swr[recid].opcode   = IBV_WR_SEND; 
   f_swr[recid].next     = NULL;
   f_swr[recid].send_flags = IBV_SEND_SIGNALED;

   Buffer* buf = fRecs[recid].buf;
   
   if ((buf!=0) && (fPoolReg!=0)) {

      if (buf->NumSegments() >= fSegmPerOper) {
         EOUT(("Too many segments")); 
         exit(1);
      }

      LockGuard lock(fPoolReg->WorkMutex());
      
      fPoolReg->_CheckMRStructure(); 
   
      for (unsigned seg=0;seg<buf->NumSegments();seg++) {
         f_sge[segid+1+seg].addr   = (uintptr_t) buf->GetDataLocation(seg);
         f_sge[segid+1+seg].length = buf->GetDataSize(seg);
         f_sge[segid+1+seg].lkey   = fPoolReg->GetLkey(buf->GetId(seg));
      }
      
      f_swr[recid].num_sge  += buf->NumSegments();
   }
   
   if ((f_swr[recid].num_sge==1) && (hdrsize<=256))
      // try to send small portion of data as inline 
      f_swr[recid].send_flags = (ibv_send_flags) (IBV_SEND_SIGNALED | IBV_SEND_INLINE);

   if (fFastPost)
      fQP->Post_Send(&(f_swr[recid]));
   else
      FireEvent(evntVerbsSendRec, recid);
}


void dabc::VerbsTransport::ProcessEvent(dabc::EventId evnt)
{
   switch (GetEventCode(evnt)) {

      case evntVerbsSendCompl:
//         DOUT1(("evntVerbsSendCompl %u", GetEventArg(evnt)));
         ProcessSendCompl(GetEventArg(evnt));
         break;
         
      case evntVerbsRecvCompl:
//         DOUT1(("evntVerbsRecvCompl %u", GetEventArg(evnt)));
         ProcessRecvCompl(GetEventArg(evnt));
         break;
         
      case evntVerbsError:
         EOUT(("Verbs error"));
         ErrorCloseTransport();
         break;
               
      case evntVerbsSendRec:
         fQP->Post_Send(&(f_swr[GetEventArg(evnt)]));
         break;
         
      case evntVerbsRecvRec:
         fQP->Post_Recv(&(f_rwr[GetEventArg(evnt)]));
         break;
         
      case evntVerbsPool:
         FillRecvQueue(); 
         break;
            
      default:
         VerbsProcessor::ProcessEvent(evnt);
   }
}

void dabc::VerbsTransport::VerbsProcessSendCompl(uint32_t arg)
{
   ProcessSendCompl(arg);
}

void dabc::VerbsTransport::VerbsProcessRecvCompl(uint32_t arg)
{
   ProcessRecvCompl(arg);
}

void dabc::VerbsTransport::VerbsProcessOperError(uint32_t)
{
   EOUT(("Verbs error"));
   ErrorCloseTransport();
}


bool dabc::VerbsTransport::ProcessPoolRequest()
{
   FireEvent(evntVerbsPool); 
   return true;
}
