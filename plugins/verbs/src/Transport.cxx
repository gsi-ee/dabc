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

#include "verbs/Transport.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"

#include "verbs/Device.h"
#include "verbs/QueuePair.h"
#include "verbs/ComplQueue.h"
#include "verbs/MemoryPool.h"

verbs::VerbsNetworkInetrface::VerbsNetworkInetrface(verbs::ContextRef ctx, QueuePair* qp) :
   verbs::WorkerAddon(qp),
   NetworkInetrface(),
   fContext(ctx),
   fInitOk(false),
   fPoolReg(),
   f_rwr(0),
   f_swr(0),
   f_sge(0),
   fHeadersPool(0),
   fSegmPerOper(2),
   f_ud_ah(0),
   f_ud_qpn(0),
   f_ud_qkey(0),
   f_multi(0),
   f_multi_lid(0),
   f_multi_attch(false)
{
}

verbs::VerbsNetworkInetrface::~VerbsNetworkInetrface()
{
   if (f_multi) {
      if (f_multi_attch)
         QP()->DetachMcast(&f_multi_gid, f_multi_lid);

      fContext.ManageMulticast(f_multi, f_multi_gid, f_multi_lid);
      f_multi = 0;
   }

   if(f_ud_ah!=0) {
      ibv_destroy_ah(f_ud_ah);
      f_ud_ah = 0;
   }

   delete fQP; fQP = 0;

   delete[] f_rwr; f_rwr = 0;
   delete[] f_swr; f_swr = 0;
   delete[] f_sge; f_sge = 0;

   dabc::Object::Destroy(fHeadersPool); fHeadersPool = 0;
}

long verbs::VerbsNetworkInetrface::Notify(const std::string &cmd, int arg)
{
   if (cmd == "GetNetworkTransportInetrface") return (long) ((dabc::NetworkInetrface*) this);

   return verbs::WorkerAddon::Notify(cmd, arg);
}

void verbs::VerbsNetworkInetrface::SetUdAddr(struct ibv_ah *ud_ah, uint32_t ud_qpn, uint32_t ud_qkey)
{
   f_ud_ah = ud_ah;
   f_ud_qpn = ud_qpn;
   f_ud_qkey = ud_qkey;
}

bool verbs::VerbsNetworkInetrface::AssignMultiGid(ibv_gid* multi_gid)
{
   dabc::NetworkTransport* tr = (dabc::NetworkTransport*) fWorker();
   if (tr==0) return false;

   if (multi_gid == 0) return false;

   if (!QP()->InitUD()) return false;

   memcpy(f_multi_gid.raw, multi_gid->raw, sizeof(f_multi_gid.raw));

   f_multi = fContext.ManageMulticast(ContextRef::mcst_Register, f_multi_gid, f_multi_lid);

   DOUT3("Init multicast group LID:%x %s", f_multi_lid, ConvertGidToStr(f_multi_gid).c_str());

   if (!f_multi) return false;

   f_ud_ah = fContext.CreateMAH(f_multi_gid, f_multi_lid);
   if (f_ud_ah==0) return false;

   f_ud_qpn = VERBS_MCAST_QPN;
   f_ud_qkey = VERBS_DEFAULT_QKEY;

   f_multi_attch = tr->IsInputTransport();

   if (f_multi_attch)
      if (!QP()->AttachMcast(&f_multi_gid, f_multi_lid)) return false;

   return true;
}



void verbs::VerbsNetworkInetrface::AllocateNet(unsigned /* fulloutputqueue */, unsigned /* fullinputqueue */)
{
   dabc::NetworkTransport* tr = (dabc::NetworkTransport*) fWorker();

   if (!tr) return;

   DOUT3("++++ verbs::VerbsNetworkInetrface::AllocateNet tr = %p poolname %s", tr, tr->TransportPoolName().c_str());

   dabc::MemoryPoolRef pool = dabc::mgr.FindPool(tr->TransportPoolName());

   if (!pool.null()) {
      fPoolReg = fContext.RegisterPool(pool());
//      dabc::mgr()->RegisterDependency(this, fPoolReg());
   } else {
      EOUT("Cannot make verbs transport without memory pool");
      return;
   }

   if (tr->NumRecs() > 0) {
      fHeadersPool = new MemoryPool(fContext, "HeadersPool", tr->NumRecs(),
            tr->GetFullHeaderSize() + (IsUD() ? VERBS_UD_MEMADDON : 0), IsUD(), true);

      // we use at least 2 segments per operation, one for header and one for buffer itself
      fSegmPerOper = fQP->NumSendSegs();
      if (fSegmPerOper<2) fSegmPerOper = 2;

      f_rwr = new ibv_recv_wr [tr->NumRecs()];
      f_swr = new ibv_send_wr [tr->NumRecs()];
      f_sge = new ibv_sge [tr->NumRecs()*fSegmPerOper];

      for (uint32_t n=0;n<tr->NumRecs();n++) {

         tr->SetRecHeader(n, fHeadersPool->GetSendBufferLocation(n));

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
         f_swr[n].next     = nullptr;
         f_swr[n].send_flags = IBV_SEND_SIGNALED;

         f_rwr[n].wr_id     = 0; // must be set later
         f_rwr[n].sg_list   = 0;
         f_rwr[n].num_sge   = 1;
         f_rwr[n].next      = nullptr;
      }
   }

   fInitOk = true;
}

void verbs::VerbsNetworkInetrface::SubmitSend(uint32_t recid)
{
   dabc::NetworkTransport* tr = (dabc::NetworkTransport*) fWorker();
   if (tr==0) return;

   uint32_t segid = recid*fSegmPerOper;
   int senddtyp = tr->PackHeader(recid);

   dabc::NetworkTransport::NetIORec* rec = tr->GetRec(recid);

   if ((rec==0) || (f_sge==0)) {
      EOUT("Did not found rec %p or f_sge %p - abort", rec, f_sge);
      exit(7);
   }

   f_sge[segid].addr = (uintptr_t) rec->header;
   f_sge[segid].length = tr->GetFullHeaderSize();
   f_sge[segid].lkey = fHeadersPool->GetLkey(recid);

   f_swr[recid].wr_id    = recid;
   f_swr[recid].sg_list  = &(f_sge[segid]);
   f_swr[recid].num_sge  = 1;
   f_swr[recid].opcode   = IBV_WR_SEND;
   f_swr[recid].next     = nullptr;
   f_swr[recid].send_flags = IBV_SEND_SIGNALED;

   if (f_ud_ah) {
      f_swr[recid].wr.ud.ah          = f_ud_ah;
      f_swr[recid].wr.ud.remote_qpn  = f_ud_qpn;
      f_swr[recid].wr.ud.remote_qkey = f_ud_qkey;
   }

   if ((senddtyp==2) && !fPoolReg.null()) {

      if (rec->buf.NumSegments() >= fSegmPerOper) {
         EOUT("Too many segments");
         exit(147);
      }

      // FIXME: dangerous, acquire memory pool mutex when transport mutex is locked
      fPoolReg()->CheckMRStructure();

      for (unsigned seg=0;seg<rec->buf.NumSegments();seg++) {
         f_sge[segid+1+seg].addr   = (uintptr_t) rec->buf.SegmentPtr(seg);
         f_sge[segid+1+seg].length = rec->buf.SegmentSize(seg);
         f_sge[segid+1+seg].lkey   = fPoolReg()->GetLkey(rec->buf.SegmentId(seg));
      }

      f_swr[recid].num_sge  += rec->buf.NumSegments();
   }

   if ((f_swr[recid].num_sge==1) && (f_sge[segid].length<=256))
      // try to send small portion of data as inline
      f_swr[recid].send_flags = (ibv_send_flags) (IBV_SEND_SIGNALED | IBV_SEND_INLINE);

   fQP->Post_Send(&(f_swr[recid]));
}

void verbs::VerbsNetworkInetrface::SubmitRecv(uint32_t recid)
{
   dabc::NetworkTransport* tr = (dabc::NetworkTransport*) fWorker();
   if (tr==0) return;

   uint32_t segid = recid*fSegmPerOper;

   dabc::NetworkTransport::NetIORec* rec = tr->GetRec(recid);

   f_rwr[recid].wr_id     = recid;
   f_rwr[recid].sg_list   = &(f_sge[segid]);
   f_rwr[recid].num_sge   = 1;
   f_rwr[recid].next      = nullptr;

   if (IsUD()) {
      f_sge[segid].addr = (uintptr_t) fHeadersPool->GetBufferLocation(recid);
      f_sge[segid].length = tr->GetFullHeaderSize() + VERBS_UD_MEMADDON;
   } else {
      f_sge[segid].addr = (uintptr_t)  rec->header;
      f_sge[segid].length = tr->GetFullHeaderSize();
   }
   f_sge[segid].lkey = fHeadersPool->GetLkey(recid);

   if (!rec->buf.null() && !fPoolReg.null()) {

      if (rec->buf.NumSegments() >= fSegmPerOper) {
         EOUT("Too many segments");
         exit(146);
      }

      fPoolReg()->CheckMRStructure();

      for (unsigned seg=0;seg<rec->buf.NumSegments();seg++) {
         f_sge[segid+1+seg].addr   = (uintptr_t) rec->buf.SegmentPtr(seg);
         f_sge[segid+1+seg].length = rec->buf.SegmentSize(seg);
         f_sge[segid+1+seg].lkey   = fPoolReg()->GetLkey(rec->buf.SegmentId(seg));
      }

      f_rwr[recid].num_sge  += rec->buf.NumSegments();
   }

   fQP->Post_Recv(&(f_rwr[recid]));
}



void verbs::VerbsNetworkInetrface::VerbsProcessSendCompl(uint32_t arg)
{
   dabc::NetworkTransport* tr = (dabc::NetworkTransport*) fWorker();

   if (tr) tr->ProcessSendCompl(arg);
}

void verbs::VerbsNetworkInetrface::VerbsProcessRecvCompl(uint32_t arg)
{
   dabc::NetworkTransport* tr = (dabc::NetworkTransport*) fWorker();

   if (tr) tr->ProcessRecvCompl(arg);
}

void verbs::VerbsNetworkInetrface::VerbsProcessOperError(uint32_t)
{
   EOUT("Verbs error");
}
