#include "verbs/BnetRunnable.h"

verbs::BnetRunnable::BnetRunnable(const char* name) :
   bnet::BnetRunnable(name),
   fRelibaleConn(true),
   fIbContext(),
   f_rwr(0),
   f_swr(0),
   f_sge(0),
   fHeaderReg(),
   fMainReg()
{

}

verbs::BnetRunnable::~BnetRunnable()
{
   delete[] f_rwr; f_rwr = 0;
   delete[] f_swr; f_swr = 0;
   delete[] f_sge; f_sge = 0;

   fHeaderReg.Release();

   fMainReg.Release();

   fIbContext.Release();
}


bool verbs::BnetRunnable::Configure(dabc::Module* m, dabc::MemoryPool* pool, int numrecs)
{
   fRelibaleConn = m->Cfg("TestReliable").AsBool(true);

   if (!bnet::BnetRunnable::Configure(m, pool, numrecs)) return false;

   if (!fIbContext.OpenVerbs(true)) {
      EOUT(("Cannot initialize VERBs!!!!"));
      return false;
   }

   fCQ = 0;
   for (int lid=0; lid<bnet::MAXLID; lid++) {
      fQPs[lid] = 0;
      fSendQueue[lid] = 0;
      fRecvQueue[lid] = 0;
   }


   f_rwr = new ibv_recv_wr [fNumRecs];
   f_swr = new ibv_send_wr [fNumRecs];
   f_sge = new ibv_sge [fNumRecs*fSegmPerOper];

   fHeaderReg = new verbs::PoolRegistry(fIbContext, &fHeaderPool);
   fHeaderReg()->SyncMRStructure();

   fMainReg = fIbContext.RegisterPool(pool);

   for (int n=0;n<fNumRecs;n++) {

      // SetRecHeader(n, fHeadersPool->GetSendBufferLocation(n));

      for (int seg_cnt=0; seg_cnt<fSegmPerOper; seg_cnt++) {
         int nseg = n*fSegmPerOper + seg_cnt;
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

bool verbs::BnetRunnable::ExecuteCloseQPs()
{
   CheckTransportThrd();

   // DOUT0(("Executing BnetRunnable::ExecuteCloseQPs()   numruns:%d", fNumRunningRecs));

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


bool verbs::BnetRunnable::ExecuteCreateQPs(void* args, int argssize)
{
   CheckTransportThrd();

   ExecuteCloseQPs();

   ibv_qp_type qp_type = IsReliableConn() ? IBV_QPT_RC : IBV_QPT_UC;

   if (qp_type == IBV_QPT_UC) DOUT0(("Testing unreliable connections"));

   int qpdepth = 128;

   if ((unsigned) argssize != NumNodes()*NumLids()*sizeof(VerbsConnRec)) {
      EOUT(("Wrong arguments length for CreateQPs"));
      return false;
   }

   VerbsConnRec* recs = (VerbsConnRec*) args;

   if (fIbContext.null() || (recs==0)) return false;

   fCQ = new verbs::ComplQueue(fIbContext, NumNodes() * qpdepth * 6, 0, true);

   DOUT2(("Create CQ length %d", NumNodes() * qpdepth * 6));

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

            DOUT2(("Create QP[%d][%d]  length = %d segm %d", lid, node, qpdepth, fSegmPerOper));

            if (fQPs[lid][node]->qp()==0) return false;
            recs[indx].qp = fQPs[lid][node]->qp_num();
            recs[indx].psn = fQPs[lid][node]->local_psn();

            //         DOUT0(("Create QP %d -> %d  %04x:%08x:%08x", Node(), node, recs[node].lid, recs[node].qp, recs[node].psn));
         }
      }
   }

   return true;
}

void verbs::BnetRunnable::ResortConnections(void* _recs)
{
   CheckModuleThrd();

   int blocksize = ConnectionBufferSize();

   void* tmp = malloc(NumNodes() * blocksize);
   memset(tmp, 0, NumNodes() * blocksize);

   VerbsConnRec* conn = (VerbsConnRec*) tmp;
   VerbsConnRec* recs = (VerbsConnRec*) _recs;

   for (int n1 = 0; n1 < NumNodes(); n1++)
      for (int n2 = 0; n2 < NumNodes(); n2++)
         for (int lid=0; lid<NumLids(); lid++) {

            int src = n2 * NumNodes() * NumLids() + lid * NumNodes() + n1;
            int tgt = n1 * NumNodes() * NumLids() + lid * NumNodes() + n2;

            memcpy(conn + tgt, recs + src, sizeof(VerbsConnRec));
         }

   memcpy(_recs, tmp, NumNodes() * blocksize);

   free(tmp);
}

bool verbs::BnetRunnable::ExecuteConnectQPs(void* args, int argssize)
{
   CheckTransportThrd();

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

   DOUT0(("BnetRunnable::ExecuteConnectQPs done"));

   return true;
}

bool verbs::BnetRunnable::DoPrepareRec(int recid)
{
   bnet::OperRec* rec = GetRec(recid);

   unsigned segid(recid*fSegmPerOper), num_sge(1);

   f_sge[segid].addr = (uintptr_t) rec->header;
   f_sge[segid].length = rec->hdrsize;
   f_sge[segid].lkey = fHeaderReg()->GetLkey(recid);

   // int senddtyp = PackHeader(recid);

/*
   if (f_ud_ah) {
      f_swr[recid].wr.ud.ah          = f_ud_ah;
      f_swr[recid].wr.ud.remote_qpn  = f_ud_qpn;
      f_swr[recid].wr.ud.remote_qkey = f_ud_qkey;
   }
*/
   if (!rec->buf.null() && !fMainReg.null()) {

      if ((int) rec->buf.NumSegments() >= fSegmPerOper) {
         EOUT(("Too many segments (%u) in buffer allowed %d", rec->buf.NumSegments(), fSegmPerOper-1));
         exit(147);
      }

      for (unsigned seg=0;seg<rec->buf.NumSegments();seg++) {
         f_sge[segid+num_sge].addr   = (uintptr_t) rec->buf.SegmentPtr(seg);
         f_sge[segid+num_sge].length = rec->buf.SegmentSize(seg);
         f_sge[segid+num_sge].lkey   = fMainReg()->GetLkey(rec->buf.SegmentId(seg));
         num_sge++;
      }
   }

   switch (rec->kind) {
      case bnet::kind_Send:
         f_swr[recid].wr_id    = recid;  // as operation identifier recid is used
         f_swr[recid].sg_list  = f_sge + segid;
         f_swr[recid].num_sge  = num_sge;
         f_swr[recid].next     = NULL;
         f_swr[recid].opcode   = IBV_WR_SEND;
         f_swr[recid].send_flags = IBV_SEND_SIGNALED;
         if ((num_sge==1) && (f_sge[segid].length<=256))
            // try to send small portion of data as inline
            f_swr[recid].send_flags = (ibv_send_flags) (IBV_SEND_SIGNALED | IBV_SEND_INLINE);
         break;
      case bnet::kind_Recv:
         f_rwr[recid].wr_id     = recid;
         f_rwr[recid].sg_list   = f_sge + segid;
         f_rwr[recid].num_sge   = num_sge;
         f_rwr[recid].next      = NULL;
         break;
      default:
         EOUT(("Wrong operation code"));
         break;
   }

/*   DOUT0(("BnetRunnable::DoPrepareRec recid:%d  %s  numseg:%u  header %p len:%u lkey:%u",
         recid, rec->kind == kind_Send ? "kind_Send" : "kind_Recv", num_sge,
        f_sge[segid].addr, f_sge[segid].length,  f_sge[segid].lkey));
*/
   return true;
}

bool verbs::BnetRunnable::DoPerformOperation(int recid)
{
   CheckTransportThrd(); // executed only in transport

   bnet::OperRec* rec = GetRec(recid);
   if (rec==0) return false;

   // DOUT0(("BnetRunnable::DoPerformOperation recid:%d  %s", recid, rec->kind == kind_Send ? "kind_Send" : "kind_Recv"));

   switch (rec->kind) {
      case bnet::kind_None:
         EOUT(("Operation cannot be done in IB"));
         return false;

      case bnet::kind_Send:
         DOUT1(("Post send lid %d node %d rec %d", rec->tgtindx, rec->tgtnode, recid));
         return fQPs[rec->tgtindx][rec->tgtnode]->Post_Send(f_swr + recid);

      case bnet::kind_Recv:
         DOUT1(("Post recv lid %d node %d rec %d", rec->tgtindx, rec->tgtnode, recid));
         return fQPs[rec->tgtindx][rec->tgtnode]->Post_Recv(f_rwr + recid);
   }

   EOUT(("Operation cannot be done in IB"));

   return false;
}

int verbs::BnetRunnable::DoWaitOperation(double waittime, double fasttime)
{
//   CheckTransportThrd(); // executed only in transport

   int res = fCQ->Wait(waittime, fasttime);

//   int res = fCQ->Poll();

   if (res==0) return -1;

   int recid = (int) fCQ->arg();
   bnet::OperRec* rec = GetRec(recid);

   if (rec==0) {
      EOUT(("Wrong operation arg!!"));
      return -1;
   }

   if (res!=1) rec->err = true;

   DOUT2(("Obtain operation %d kind %d repeatcnt %d", recid, rec->kind, rec->repeatcnt));

   return recid;
}
