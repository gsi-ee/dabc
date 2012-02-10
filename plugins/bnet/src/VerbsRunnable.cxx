#include "bnet/VerbsRunnable.h"

#ifdef WITH_VERBS

bnet::VerbsRunnable::VerbsRunnable() :
   bnet::TransportRunnable(),
   fNumLids(1),
   fRelibaleConn(true),
   fIbContext()
{

}

bnet::VerbsRunnable::~VerbsRunnable()
{
   fIbContext.Release();
}


bool bnet::VerbsRunnable::Configure(dabc::Module* m, dabc::Command cmd)
{
   fNumLids = m->Cfg("TestNumLids", cmd).AsInt(1);
   if (fNumLids>IBTEST_MAXLID) fNumLids = IBTEST_MAXLID;

   fRelibaleConn = m->Cfg("TestReliable", cmd).AsBool(true);

   bool res = TransportRunnable::Configure(m);

   if (!res) return false;

   if (!fIbContext.OpenVerbs(true)) {
      EOUT(("Cannot initialize VERBs!!!!"));
      return false;
   }

   return true;
}

bool bnet::VerbsRunnable::ExecuteCloseQPs()
{
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


bool bnet::VerbsRunnable::ExecuteCreateQPs()
{
   ExecuteCloseQPs();

   ibv_qp_type qp_type = IsReliableConn() ? IBV_QPT_RC : IBV_QPT_UC;

   if (qp_type == IBV_QPT_UC) DOUT0(("Testing unreliable connections"));

   int qpdepth = 128;

   VerbsConnRec* recs = (VerbsConnRec*) fCmdDataBuffer;

   fCmdDataSize = NumNodes()*NumLids()*sizeof(VerbsConnRec);

   if (fCmdDataSize > fCmdBufferSize) {
      EOUT(("Not enough space to store all connections records in command buffer"));
      return false;
   }

   if (fIbContext.null() || (recs==0)) return false;

   fCQ = new verbs::ComplQueue(fIbContext, NumNodes() * qpdepth * 6, 0, true);

   for (int lid = 0; lid<NumLids(); lid++) {

      fQPs[lid] = new verbs::QueuePair* [NumNodes()];
      for (int n=0;n<NumNodes();n++) fQPs[lid][n] = 0;

      for (int node=0;node<NumNodes();node++) {

         int indx = lid * NumNodes() + node;

         recs[indx].lid = fIbContext.lid() + lid;

         if ((node == Node()) || !IsActiveNode(node)) {
            recs[indx].qp = 0;
            recs[indx].psn = 0;
         } else {
            fQPs[lid][node] = new verbs::QueuePair(fIbContext, qp_type, fCQ, qpdepth, 2, fCQ, qpdepth, 2);
            if (fQPs[lid][node]->qp()==0) return false;
            recs[indx].qp = fQPs[lid][node]->qp_num();
            recs[indx].psn = fQPs[lid][node]->local_psn();

            //         DOUT0(("Create QP %d -> %d  %04x:%08x:%08x", Node(), node, recs[node].lid, recs[node].qp, recs[node].psn));
         }
      }
   }
}

void bnet::VerbsRunnable::ResortConnections(void* _recs, void* _conn)
{
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

bool bnet::VerbsRunnable::ExecuteConnectQPs()
{
   VerbsConnRec* recs = (VerbsConnRec*) fCmdDataBuffer;

   if (fCmdDataSize!=ConnectionBufferSize()) {
      EOUT(("Wrong size of data for connecting QPs"));
      return false;
   }

   fCmdDataSize = 0;

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


#endif
