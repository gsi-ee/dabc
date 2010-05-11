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
#include "verbs/Transport.h"

#include "dabc/logging.h"
#include "dabc/Port.h"

#include "verbs/Device.h"
#include "verbs/QueuePair.h"
#include "verbs/ComplQueue.h"
#include "verbs/MemoryPool.h"

verbs::Transport::Transport(Device* verbs, ComplQueue* cq, QueuePair* qp, dabc::Port* port,
                            bool useackn, ibv_gid* multi_gid) :
   dabc::NetworkTransport(verbs),
   Processor(qp),
   fCQ(cq),
   fInitOk(false),
   fPoolReg(0),
   f_rwr(0),
   f_swr(0),
   f_sge(0),
   fHeadersPool(0),
   fSegmPerOper(2),
   fFastPost(true),
   f_ud_ah(0),
   f_ud_qpn(0),
   f_ud_qkey(0),
   f_multi(0),
   f_multi_lid(0),
   f_multi_attch(false)
{
   if (qp==0) return;

   Init(port, useackn);

   if (multi_gid) {

      if (!QP()->InitUD()) return;

      memcpy(f_multi_gid.raw, multi_gid->raw, sizeof(f_multi_gid.raw));

      f_multi = verbs->ManageMulticast(Device::mcst_Register, f_multi_gid, f_multi_lid);

      DOUT1(("Init multicast group LID:%x %s", f_multi_lid, ConvertGidToStr(f_multi_gid).c_str()));

      if (!f_multi) return;

      f_ud_ah = verbs->CreateMAH(f_multi_gid, f_multi_lid);
      if (f_ud_ah==0) return;

      f_ud_qpn = VERBS_MCAST_QPN;
      f_ud_qkey = VERBS_DEFAULT_QKEY;

      f_multi_attch = port->InputQueueCapacity() > 0;

      if (f_multi_attch)
         if (!QP()->AttachMcast(&f_multi_gid, f_multi_lid)) return;
   }

   fFastPost = Device::IsThreadSafeVerbs();

   if (fPool!=0) {
      fPoolReg = verbs->RegisterPool(fPool);
   } else {
      EOUT(("Cannot make verbs transport without memory pool"));
      return;
   }

   if (fNumRecs>0) {
      fHeadersPool = new MemoryPool(verbs, "HeadersPool", fNumRecs,
            fFullHeaderSize + (IsUD() ? VERBS_UD_MEMADDON : 0), IsUD(), true);

      // we use at least 2 segments per operation, one for header and one for buffer itself
      fSegmPerOper = fQP->NumSendSegs();
      if (fSegmPerOper<2) fSegmPerOper = 2;

      f_rwr = new ibv_recv_wr [fNumRecs];
      f_swr = new ibv_send_wr [fNumRecs];
      f_sge = new ibv_sge [fNumRecs*fSegmPerOper];

      for (uint32_t n=0;n<fNumRecs;n++) {

         SetRecHeader(n, fHeadersPool->GetSendBufferLocation(n));

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

   fInitOk = true;
}

verbs::Transport::~Transport()
{
   DOUT3(("verbs::Transport::~Transport %p starts", this));

   HaltProcessor();

   // we need this while at some point verbs thread will try to access
   // qp, which is destroyed at that moment
   RemoveProcessorFromThread(true);

   DOUT3(("verbs::Transport::~Transport %p id: %d locked:%s", this, GetId(), DBOOL(fMutex.IsLocked())));

   if (f_multi) {
      if (f_multi_attch)
         QP()->DetachMcast(&f_multi_gid, f_multi_lid);

      ((verbs::Device*) GetDevice())->ManageMulticast(f_multi, f_multi_gid, f_multi_lid);
      f_multi = 0;
   }

   if(f_ud_ah!=0) {
      ibv_destroy_ah(f_ud_ah);
      f_ud_ah = 0;
   }

   QueuePair* delqp = 0;

   {
      dabc::LockGuard guard(fMutex);
      delqp = fQP; fQP = 0;
   }

   delete delqp;

   delete fCQ; fCQ = 0;

   delete[] f_rwr; f_rwr = 0;
   delete[] f_swr; f_swr = 0;
   delete[] f_sge; f_sge = 0;

   DOUT3(("verbs::Transport::~Transport %p id %d Close", this, GetId()));

   Cleanup();

   DOUT3(("verbs::Transport::~Transport %p id %d delete headers pool", this, GetId()));

   delete fHeadersPool; fHeadersPool = 0;

   DOUT3(("verbs::Transport::~Transport %p unregister pool"));

   ((Device*) fDevice)->UnregisterPool(fPoolReg);
   fPoolReg = 0;

   DOUT3(("verbs::Transport::~Transport %p done", this));
}

void verbs::Transport::SetUdAddr(struct ibv_ah *ud_ah, uint32_t ud_qpn, uint32_t ud_qkey)
{
   f_ud_ah = ud_ah;
   f_ud_qpn = ud_qpn;
   f_ud_qkey = ud_qkey;
}


void verbs::Transport::_SubmitRecv(uint32_t recid)
{
   // only for RC or UC, not work for UD

//   DOUT1(("_SubmitRecv %u", recid));

   uint32_t segid = recid*fSegmPerOper;

   dabc::Buffer* buf = fRecs[recid].buf;

   f_rwr[recid].wr_id     = recid;
   f_rwr[recid].sg_list   = &(f_sge[segid]);
   f_rwr[recid].num_sge   = 1;
   f_rwr[recid].next      = NULL;

   if (IsUD()) {
      f_sge[segid].addr = (uintptr_t)  fHeadersPool->GetBufferLocation(recid);
      f_sge[segid].length = fFullHeaderSize + VERBS_UD_MEMADDON;
   } else {
      f_sge[segid].addr = (uintptr_t)  fRecs[recid].header;
      f_sge[segid].length = fFullHeaderSize;
   }
   f_sge[segid].lkey = fHeadersPool->GetLkey(recid);

   if ((buf!=0) && (fPoolReg!=0)) {

      if (buf->NumSegments() >= fSegmPerOper) {
         EOUT(("Too many segments"));
         exit(146);
      }

      dabc::LockGuard lock(fPoolReg->WorkMutex());

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

void verbs::Transport::_SubmitSend(uint32_t recid)
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

   if (f_ud_ah) {
      f_swr[recid].wr.ud.ah          = f_ud_ah;
      f_swr[recid].wr.ud.remote_qpn  = f_ud_qpn;
      f_swr[recid].wr.ud.remote_qkey = f_ud_qkey;
   }

   dabc::Buffer* buf = fRecs[recid].buf;

   if ((buf!=0) && (fPoolReg!=0)) {

      if (buf->NumSegments() >= fSegmPerOper) {
         EOUT(("Too many segments"));
         exit(147);
      }

      dabc::LockGuard lock(fPoolReg->WorkMutex());

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


void verbs::Transport::ProcessEvent(dabc::EventId evnt)
{
   switch (dabc::GetEventCode(evnt)) {

      case evntVerbsSendCompl:
//         DOUT1(("evntVerbsSendCompl %u", GetEventArg(evnt)));
         ProcessSendCompl(dabc::GetEventArg(evnt));
         break;

      case evntVerbsRecvCompl:
//         DOUT1(("evntVerbsRecvCompl %u", GetEventArg(evnt)));
         ProcessRecvCompl(dabc::GetEventArg(evnt));
         break;

      case evntVerbsError:
         EOUT(("Verbs error"));
         ErrorCloseTransport();
         break;

      case evntVerbsSendRec:
         fQP->Post_Send(&(f_swr[dabc::GetEventArg(evnt)]));
         break;

      case evntVerbsRecvRec:
         fQP->Post_Recv(&(f_rwr[dabc::GetEventArg(evnt)]));
         break;

      case evntVerbsPool:
         FillRecvQueue();
         break;

      default:
         Processor::ProcessEvent(evnt);
   }
}

void verbs::Transport::VerbsProcessSendCompl(uint32_t arg)
{
   ProcessSendCompl(arg);
}

void verbs::Transport::VerbsProcessRecvCompl(uint32_t arg)
{
   ProcessRecvCompl(arg);
}

void verbs::Transport::VerbsProcessOperError(uint32_t)
{
   EOUT(("Verbs error"));
   ErrorCloseTransport();
}


bool verbs::Transport::ProcessPoolRequest()
{
   FireEvent(evntVerbsPool);
   return true;
}

void verbs::Transport::HaltTransport()
{
   HaltProcessor();

   RemoveProcessorFromThread(true);
}
