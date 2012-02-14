#include "bnet/VerbsRunnable.h"


#ifdef WITH_VERBS

bnet::VerbsRunnable::VerbsRunnable() :
   bnet::TransportRunnable(),
   fNumLids(1),
   fRelibaleConn(true),
   fIbContext(),
   f_rwr(0),
   f_swr(0),
   f_sge(0),
   fHeaderReg(),
   fMainReg()
{

}

bnet::VerbsRunnable::~VerbsRunnable()
{
   delete[] f_rwr; f_rwr = 0;
   delete[] f_swr; f_swr = 0;
   delete[] f_sge; f_sge = 0;

   fHeaderReg.Release();

   fMainReg.Release();

   fIbContext.Release();
}


bool bnet::VerbsRunnable::Configure(dabc::Module* m, dabc::MemoryPool* pool, dabc::Command cmd)
{
   fNumLids = m->Cfg("TestNumLids", cmd).AsInt(1);
   if (fNumLids>IBTEST_MAXLID) fNumLids = IBTEST_MAXLID;

   fRelibaleConn = m->Cfg("TestReliable", cmd).AsBool(true);

   bool res = TransportRunnable::Configure(m, pool, cmd);

   if (!res) return false;

   if (!fIbContext.OpenVerbs(true)) {
      EOUT(("Cannot initialize VERBs!!!!"));
      return false;
   }

   f_rwr = new ibv_recv_wr [fNumRecs];
   f_swr = new ibv_send_wr [fNumRecs];
   f_sge = new ibv_sge [fNumRecs*fSegmPerOper];

   fHeaderReg = new PoolRegistry(fIbContext, &fHeaderPool);

   fMainReg = fIbContext.RegisterPool(pool);

   for (uint32_t n=0;n<fNumRecs;n++) {

      // SetRecHeader(n, fHeadersPool->GetSendBufferLocation(n));

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


   return true;
}

bool bnet::VerbsRunnable::ExecuteCloseQPs()
{
   IsTransportThrd(__func__);

   for (int lid=0; lid<NumLids(); lid++) {
      if (fQPs[lid]!=0) {
         for (int n=0;n<NumNodes();n++) delete fQPs[lid][n];
         delete[] fQPs[lid];
         fQPs[lid] = 0;
      }

      delete[] fSendQueue[lid]; fSendQueue[lid] = 0;
      delete[] fRecvQueue[lid]; fRecvQueue[lid] = 0;
   }

   if (fCQ!=0) {
      delete fCQ;
      fCQ = 0;
   }

   return true;
}


bool bnet::VerbsRunnable::ExecuteCreateQPs(void* args, int argssize)
{
   IsTransportThrd(__func__);

   ExecuteCloseQPs();

   ibv_qp_type qp_type = IsReliableConn() ? IBV_QPT_RC : IBV_QPT_UC;

   if (qp_type == IBV_QPT_UC) DOUT0(("Testing unreliable connections"));

   int qpdepth = 128;

   if (argssize!=NumNodes()*NumLids()*sizeof(VerbsConnRec)) {
      EOUT(("Wrong arguments length for CreateQPs"));
      return false;
   }

   VerbsConnRec* recs = (VerbsConnRec*) args;

   if (fIbContext.null() || (recs==0)) return false;

   fCQ = new verbs::ComplQueue(fIbContext, NumNodes() * qpdepth * 6, 0, true);

   for (int lid = 0; lid<NumLids(); lid++) {

      fQPs[lid] = new verbs::QueuePair* [NumNodes()];
      for (int n=0;n<NumNodes();n++) fQPs[lid][n] = 0;

      for (int node=0;node<NumNodes();node++) {

         int indx = lid * NumNodes() + node;

         recs[indx].lid = fIbContext.lid() + lid;

         if ((node == NodeId()) || !IsActiveNode(node)) {
            recs[indx].qp = 0;
            recs[indx].psn = 0;
         } else {
            fQPs[lid][node] = new verbs::QueuePair(fIbContext, qp_type, fCQ, qpdepth, fSegmPerOper, fCQ, qpdepth, 2);
            if (fQPs[lid][node]->qp()==0) return false;
            recs[indx].qp = fQPs[lid][node]->qp_num();
            recs[indx].psn = fQPs[lid][node]->local_psn();

            //         DOUT0(("Create QP %d -> %d  %04x:%08x:%08x", Node(), node, recs[node].lid, recs[node].qp, recs[node].psn));
         }
      }
   }

   return true;
}

void bnet::VerbsRunnable::ResortConnections(void* _recs, void* _conn)
{
   IsModuleThrd(__func__);

   VerbsConnRec* conn = (VerbsConnRec*) _conn;
   VerbsConnRec* recs = (VerbsConnRec*) _recs;

   for (int n1 = 0; n1 < NumNodes(); n1++)
      for (int n2 = 0; n2 < NumNodes(); n2++)
         for (int lid=0; lid<NumLids(); lid++) {

            int src = n2 * NumNodes() * NumLids() + lid * NumNodes() + n1;
            int tgt = n1 * NumNodes() * NumLids() + lid * NumNodes() + n2;

            memcpy(conn + tgt, recs + src, sizeof(VerbsConnRec));
         }
}

bool bnet::VerbsRunnable::ExecuteConnectQPs(void* args, int argssize)
{
   IsTransportThrd(__func__);

   VerbsConnRec* recs = (VerbsConnRec*) args;

   if (argssize != ConnectionBufferSize()) {
      EOUT(("Wrong size of data for connecting QPs"));
      return false;
   }

   for (int lid=0; lid<NumLids(); lid++) {

      for (int node=0;node<NumNodes();node++) {

         int indx = lid * NumNodes() + node;

         if ((fQPs[lid][node]==0) || !IsActiveNode(node)) continue;

         // FIXME: one should deliver destination port as well
         if (!fQPs[lid][node]->Connect(recs[indx].lid, recs[indx].qp, recs[indx].psn, lid)) return false;

         DOUT3(("Connect QP[%d,%d] -> with  %04x:%08x:%08x", lid, node, recs[indx].lid, recs[indx].qp, recs[indx].psn));

      }
   }

   return true;
}

bool bnet::VerbsRunnable::DoPrepareRec(OperRec* rec)
{
   OperRec* rec = GetRec(recid);

   uint32_t segid = recid*fSegmPerOper;

   f_sge[segid].addr = (uintptr_t) rec->header;
   f_sge[segid].length = hdrsize;
   f_sge[segid].lkey = fHeaderReg()->GetLkey(recid);

   unsigned num_sge(1);

   // int senddtyp = PackHeader(recid);

/*
   if (f_ud_ah) {
      f_swr[recid].wr.ud.ah          = f_ud_ah;
      f_swr[recid].wr.ud.remote_qpn  = f_ud_qpn;
      f_swr[recid].wr.ud.remote_qkey = f_ud_qkey;
   }
*/
   if (!buf.null() && !fMainReg.null()) {

      if (buf.NumSegments() >= fSegmPerOper) {
         EOUT(("Too many segments (%d) in buffer allowed %d", buf.NumSegments(), fSegmPerOper-1));
         exit(147);
      }

      for (unsigned seg=0;seg<buf.NumSegments();seg++) {
         f_sge[segid+num_sge].addr   = (uintptr_t) buf.SegmentPtr(seg);
         f_sge[segid+num_sge].length = buf.SegmentSize(seg);
         f_sge[segid+num_sge].lkey   = fMainReg()->GetLkey(buf.SegmentId(seg));
         num_sge++;
      }
   }

   switch (rec->kind) {
      case kind_Send:
         f_swr[recid].wr_id    = recid;  // as operation identifier recid is used
         f_swr[recid].sg_list  = f_sge + segid;
         f_swr[recid].num_sge  = num_sge;
         f_swr[recid].opcode   = IBV_WR_SEND;
         f_swr[recid].next     = NULL;
         f_swr[recid].send_flags = IBV_SEND_SIGNALED;
         if ((num_sge==1) && (f_sge[segid].length<=256))
            // try to send small portion of data as inline
            f_swr[recid].send_flags = (ibv_send_flags) (IBV_SEND_SIGNALED | IBV_SEND_INLINE);
         break;
      case kind_Recv:
         f_rwr[recid].wr_id     = recid;
         f_rwr[recid].sg_list   = f_sge + segid;
         f_rwr[recid].num_sge   = num_sge;
         f_rwr[recid].next      = NULL;
         break;
      default:
         EOUT(("Wrong operation code"));
         break;
   }

   return true;
}

bool bnet::VerbsRunnable::DoPerformOperation(int recid)
{
   IsTransportThrd(__func__); // executed only in transport

   OperRec* rec = GetRec(recid);
   if (rec==0) return false;

   bool res(false);

   switch (rec->kind) {
      case kind_Send:
         res = fQPs[rec->tgtindx][rec->tgtnode]->Post_Send(f_swr + recid);
         break;
      case kind_Recv:
         res = fQPs[rec->tgtindx][rec->tgtnode]->Post_Recv(f_rwr + recid);
         break;
      default:
         EOUT(("Operation cannot be done in IB"));
         return false;
   }

   return res;
}

int bnet::VerbsRunnable::DoWaitOperation(double waittime, double fasttime)
{
   IsTransportThrd(__func__); // executed only in transport

   int res = fCQ->Wait(waittime, fasttime);

   if (res==0) return -1;

   int recid = (int) fCQ->arg();
   OperRec* rec = GetRec(recid);

   if (rec==0) {
      EOUT(("Wrong operation arg!!"));
      return -1;
   }

   if (res!=1) rec->err = true;

   return recid;
}




#endif
