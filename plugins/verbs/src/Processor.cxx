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
#include "verbs/Processor.h"

#include <infiniband/verbs.h>

#include "dabc/Command.h"

#include "verbs/Device.h"
#include "verbs/QueuePair.h"
#include "verbs/ComplQueue.h"
#include "verbs/MemoryPool.h"
#include "verbs/Thread.h"

ibv_gid VerbsConnThrd_Multi_Gid =  {   {
      0xFF, 0x12, 0xA0, 0x1C,
      0xFE, 0x80, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x33, 0x44, 0x55, 0x66
    } };


struct VerbsConnectData
{
   uint64_t kind; // 111 - client request, 222 - server accept, 333 - server reject

   uint64_t node_LID;
   uint64_t node_Port;

   // UD connection port numbers
   uint64_t conn_QPN;
   uint64_t conn_PSN;

   // prepared transport numbers
   uint64_t tr_QPN;
   uint64_t tr_PSN;

   // handshake port numbers
   uint64_t hand_QPN;
   uint64_t hand_PSN;

   uint64_t ConnType;
   char     ConnId[128];
};


verbs::Processor::Processor(QueuePair* qp) :
   WorkingProcessor(),
   fQP(qp)
{
}

verbs::Processor::~Processor()
{
   CloseQP();
}

const char* verbs::Processor::RequiredThrdClass() const
{
   return VERBS_THRD_CLASSNAME;
}


void verbs::Processor::SetQP(QueuePair* qp)
{
   CloseQP();
   fQP = qp;
}

void verbs::Processor::CloseQP()
{
   if (fQP!=0) {
      delete fQP;
      fQP = 0;
   }
}

void verbs::Processor::ProcessEvent(dabc::EventId evnt)
{
   switch (dabc::GetEventCode(evnt)) {

      case evntVerbsSendCompl:
         VerbsProcessSendCompl(dabc::GetEventArg(evnt));
         break;

      case evntVerbsRecvCompl:
         VerbsProcessRecvCompl(dabc::GetEventArg(evnt));
         break;

      case evntVerbsError:
         VerbsProcessOperError(dabc::GetEventArg(evnt));
         break;

      default:
         dabc::WorkingProcessor::ProcessEvent(evnt);
   }
}
// ___________________________________________________________________

verbs::ConnectProcessor::ConnectProcessor(Thread* thrd) :
   Processor(0),
   fCQ(0),
   fPool(0),
   fUseMulti(false),
   fMultiAH(0),
   fConnCounter(0)
{
   Device* dev = thrd->GetDevice();

   if (ServBufferSize < sizeof(VerbsConnectData)) {
      EOUT(("VerbsConnectData do not fit in UD buffer %d", sizeof(VerbsConnectData)));
      exit(1);
   }

   fCQ = new ComplQueue(dev->context(), ServQueueSize*3, thrd->Channel());

   QueuePair* qp = new QueuePair(dev, IBV_QPT_UD,
                             fCQ, ServQueueSize, 1,
                             fCQ, ServQueueSize, 1);
   if (!qp->InitUD()) {
      EOUT(("fConnQP INIT FALSE"));
      exit(1);
   }

   SetQP(qp);

  if (dev->IsMulticast())
      if (dev->ManageMulticast(Device::mcst_Register, VerbsConnThrd_Multi_Gid, fMultiLID)) {
         fUseMulti = true;

         if (qp->AttachMcast(&VerbsConnThrd_Multi_Gid, fMultiLID))
            fMultiAH = dev->CreateMAH(VerbsConnThrd_Multi_Gid, fMultiLID);
      }

   fPool = new MemoryPool(dev, "ConnPool", ServQueueSize*2, ServBufferSize, true);

   for (int n=0;n<ServQueueSize;n++) {
      dabc::BufferId_t bufid = 0;
      if (!fPool->TakeRawBuffer(bufid)) {
         EOUT(("No free memory in connection pool"));
      } else
         fQP->Post_Recv(fPool->GetRecvWR(bufid));
   }
}

verbs::ConnectProcessor::~ConnectProcessor()
{
   if (fUseMulti) {

      Device* dev = ((Thread*)ProcessorThread())->GetDevice();

      fQP->DetachMcast(&VerbsConnThrd_Multi_Gid, fMultiLID);

      dev->ManageMulticast(Device::mcst_Unregister, VerbsConnThrd_Multi_Gid, fMultiLID);
      fUseMulti = false;
   }

   if(fMultiAH!=0) {
      ibv_destroy_ah(fMultiAH);
      fMultiAH = 0;
   }

   if (fPool) fPool->ReleaseAllRawBuffers();
   delete fPool; fPool = 0;

   CloseQP();
   delete fCQ; fCQ = 0;
}

void verbs::ConnectProcessor::VerbsProcessSendCompl(uint32_t bufid)
{
   fPool->ReleaseRawBuffer(bufid);
}

void verbs::ConnectProcessor::VerbsProcessRecvCompl(uint32_t bufid)
{
   VerbsConnectData* inp = (VerbsConnectData*) fPool->GetSendBufferLocation(bufid);

   ProtocolProcessor* findrec = 0;

   // normally this is UD packet after send via mcast group
   if ((inp->node_LID == thrd()->GetDevice()->lid()) && (inp->conn_QPN == QP()->qp_num())) {
      if (!fUseMulti) {
        EOUT(("Packet from myself without MULTICAST!!!"));
      }
   } else {

      findrec = thrd()->FindProtocol(inp->ConnId);

      DOUT3(("Get UD Request for connection %s rec %p kind %d serv %s", inp->ConnId, findrec, inp->kind, DBOOL(findrec->fServer)));

      if ((findrec==0) && !fUseMulti)
          EOUT(("Cannot find info for requested connection %s", inp->ConnId));
   }

   if (findrec!=0)
   switch (inp->kind) {

      case 111: { // this is request from client
         if (!findrec->fServer) break;

         findrec->fRemoteLID = inp->node_LID;
         findrec->fRemoteQPN = inp->conn_QPN;
         findrec->fTransportQPN = inp->tr_QPN;
         findrec->fTransportPSN = inp->tr_PSN;

//         findrec->fPortQP->Connect
//             (findrec->fRemoteLID, findrec->fTransportQPN, findrec->fTransportPSN);

         if (!findrec->StartHandshake(true, inp)) break;

         // this means send reply message
         findrec->fKindStatus = 222;

         break;
      }

      case 222: {    // this is accept from server
         if (findrec->fServer) break;

         findrec->fRemoteLID = inp->node_LID;
         findrec->fRemoteQPN = inp->conn_QPN;
         findrec->fTransportQPN = inp->tr_QPN;
         findrec->fTransportPSN = inp->tr_PSN;

//         findrec->fPortQP->Connect
//             (findrec->fRemoteLID, findrec->fTransportQPN, findrec->fTransportPSN);

         if (!findrec->StartHandshake(false, inp)) break;

         // this means that we should wait hand-shake send compl event
         // if it fails, complete connection will failed - no retry will be done
         findrec->fKindStatus = 0;

         break;
      }

      default:
         EOUT(("Unknown kind %d for connection %s", inp->kind, inp->ConnId));

    } // switch

   // resubmit buffer to receive it again
   fQP->Post_Recv(fPool->GetRecvWR(bufid));
}

void verbs::ConnectProcessor::VerbsProcessOperError(uint32_t bufid)
{
   EOUT(("Error in connection processor !!!!!!"));
   fPool->ReleaseRawBuffer(bufid);
}

bool verbs::ConnectProcessor::TrySendConnRequest(ProtocolProcessor* rec)
{
   if ((rec==0) || (rec->fKindStatus==0)) return false;

   dabc::BufferId_t bufid = 0;
   if (!fPool->TakeRawBuffer(bufid)) {
      EOUT(("No buffers in connection pool"));
      return false;
   }

   VerbsConnectData out;

   out.kind = rec->fKindStatus;
   out.node_LID = thrd()->GetDevice()->lid();
   out.node_Port = thrd()->GetDevice()->IbPort();
   out.conn_QPN = fQP->qp_num();
   out.conn_PSN = fQP->local_psn();

   out.tr_QPN = rec->fPortQP->qp_num();
   out.tr_PSN = rec->fPortQP->local_psn();

   out.hand_QPN = rec->QP()->qp_num();
   out.hand_PSN = rec->QP()->local_psn();

   out.ConnType = rec->fConnType;
   strcpy(out.ConnId, rec->fConnId.c_str());

   memcpy(fPool->GetSendBufferLocation(bufid), &out, sizeof(out));

   if ((rec->f_ah==0) && (rec->fRemoteLID>0))
      rec->f_ah = thrd()->GetDevice()->CreateAH(rec->fRemoteLID);

   // send request information to server UD

   struct ibv_ah *tgt_ah = rec->f_ah;
   uint32_t tgt_qpn = rec->fRemoteQPN;

   if (fUseMulti && fMultiAH && (tgt_ah==0)) {
       tgt_ah = fMultiAH;
       tgt_qpn = VERBS_MCAST_QPN;
   }

   if (tgt_ah==0) {
      EOUT(("=========== AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));
      exit(1);
   }

//       fConnQP->Post_Send_UD(ref, (uint64_t) ref, tgt_ah, tgt_qpn);

   struct ibv_send_wr* swr = fPool->GetSendWR(bufid, sizeof(VerbsConnectData));
   swr->wr.ud.ah          = tgt_ah;
   swr->wr.ud.remote_qpn  = tgt_qpn;
   swr->wr.ud.remote_qkey = VERBS_DEFAULT_QKEY;
   QP()->Post_Send(swr);

   DOUT3(("Sending UD packet for connection %s kind %d ", out.ConnId, out.kind));

   // one should retry many times only from client side
   if (rec->fKindStatus!=111)
     rec->fKindStatus = 0;

   return true;
}


// ____________________________________________________________________


verbs::ProtocolProcessor::ProtocolProcessor(Thread* thrd,
                                            QueuePair* hs_qp,
                                            bool server,
                                            const char* portname,
                                            dabc::Command* cmd) :
   Processor(hs_qp),
   fServer(server),
   fPortName(portname),
   fCmd(cmd),
   fPortQP(0),
   fPortCQ(0),
   fConnType(0),
   fThrdName(),
   fTimeout(10.), // per default, 10 sec for connection, can be changed
   fConnId(),
   fLastAttempt(0.),
   fUseAckn(false),
   fKindStatus(0),
   fRemoteLID(0),
   fRemoteQPN(0),
   fTransportQPN(0),
   fTransportPSN(0),
   f_ah(0),
   fConnectedHS(false),
   fPool(0),
   fConnected(false)
{
   // we need the only buffer, which used either for receive (by server) or send (by client)
   fPool = new MemoryPool(thrd->GetDevice(), "HandshakePool", 1, 1024, false);
   if (fCmd) fUseAckn = fCmd->GetBool(dabc::xmlUseAcknowledge, false);

}

verbs::ProtocolProcessor::~ProtocolProcessor()
{
   if (fPool) delete fPool; fPool = 0;

   if(f_ah!=0) ibv_destroy_ah(f_ah);

   CloseQP();
   if (fPortQP) delete fPortQP; fPortQP = 0;
   if (fPortCQ) delete fPortCQ; fPortCQ = 0;

   dabc::Command::Reply(fCmd, false);
}

void verbs::ProtocolProcessor::OnThreadAssigned()
{
   if (fServer && fCmd)
     if (fCmd->GetBool("ImmidiateReply", false)) {
        dabc::Command::Reply(fCmd, true);
        fCmd = 0;
     }

   DOUT5(("Activate %s %s ", (fServer ? "SERVER" : "CLIENT"), fConnId.c_str()));
   // enter timeout as soon as possible
   ActivateTimeout(0.);
}


double verbs::ProtocolProcessor::ProcessTimeout(double last_diff)
{
   if (fConnected) return -1;

   fTimeout -= last_diff;

   DOUT5(("Timedout %s %s restmout %5.1f", (fServer ? "SERVER" : "CLIENT"), fConnId.c_str(), fTimeout));

   ConnectProcessor* conn = thrd()->Connect();

   if ((fTimeout<0) || (conn==0)) {
      EOUT(("Connection failed %s", fConnId.c_str()));
      DestroyProcessor();
      return -1;
   }

   double next_tmout = 0.5; // default timeout

   if (fKindStatus!=0) {
     if (conn->TrySendConnRequest(this))
        next_tmout = 1.0;
     else
        next_tmout = 0.1; // repeat after short time;
   }

   return next_tmout;
}

bool verbs::ProtocolProcessor::StartHandshake(bool dorecv, void* connrec)
{
   if ((QP()==0) || (fPool==0) || (connrec==0)) return false;

   VerbsConnectData* rec = (VerbsConnectData*) connrec;

   if (fConnectedHS) {

      if ((rec->node_LID != QP()->remote_lid()) ||
          (rec->hand_QPN != QP()->remote_qpn()) ||
          (rec->hand_PSN != QP()->remote_psn())) {
             EOUT(("Already connected handshake was requested to connect with other QP"));
             EOUT(("Error: ConnId %s", fConnId.c_str()));
             EOUT(("Error: %x:%x  %x:%x  %x:%x",
                     rec->node_LID, QP()->remote_lid(),
                     rec->hand_QPN, QP()->remote_qpn(),
                     rec->hand_PSN,  QP()->remote_psn()));
          }
      return true;
   }

   if (!QP()->Connect(rec->node_LID, rec->hand_QPN, rec->hand_PSN)) {
      EOUT(("CANNOT CONNECT HandShake QP"));
      return false;
   }

   fConnectedHS = true;

   dabc::BufferId_t bufid = 0;

   if (dorecv) {
      QP()->Post_Recv(fPool->GetRecvWR(bufid));
   } else {
      strcpy((char*) fPool->GetSendBufferLocation(bufid), fConnId.c_str());
      QP()->Post_Send(fPool->GetSendWR(bufid, fConnId.length()+1));
   }

   return true;
}


void verbs::ProtocolProcessor::VerbsProcessSendCompl(uint32_t bufid)
{
   if (bufid!=0) {
      EOUT(("Wrong buffer id %u", bufid));
      return;
   }

   if (fServer) {
      EOUT(("Something wrong. Server cannot send via handshake channel"));
      return;
   }

   const char* connid = (const char*) fPool->GetSendBufferLocation(bufid);

   if (fConnId.compare(connid)!=0) {
      EOUT(("AAAAA !!!!! Missmatch with connid %s %s", connid, fConnId.c_str()));
   }

   // Here we are sure, that other side recieve handshake packet,
   // therefore we can declare connection as done

   fPortQP->Connect(fRemoteLID, fTransportQPN, fTransportPSN);

   thrd()->GetDevice()->CreateVerbsTransport(fThrdName.c_str(), fPortName.c_str(), fUseAckn, fPortCQ, fPortQP);
   fPortQP = 0;
   fPortCQ = 0;

   DOUT1(("CREATE CLIENT %s",  fConnId.c_str()));

   dabc::Command::Reply(fCmd, true);
   fCmd = 0;
   fConnected = true;

   DestroyProcessor();
}

void verbs::ProtocolProcessor::VerbsProcessRecvCompl(uint32_t bufid)
{
   if (bufid!=0) {
      EOUT(("Wrong buffer id %u", bufid));
      return;
   }

   if (!fServer) {
      EOUT(("Something wrong. Client cannot recv packets via handshake"));
      return;
   }

   const char* connid = (const char*) fPool->GetBufferLocation(bufid);

   if (fConnId.compare(connid)!=0) {
      EOUT(("AAAAA !!!!! Missmatch with connid %s %s", connid, fConnId.c_str()));
   }

   // from here we knew, that client is ready and we also can complete connection

   fPortQP->Connect(fRemoteLID, fTransportQPN, fTransportPSN);

   thrd()->GetDevice()->CreateVerbsTransport(fThrdName.c_str(), fPortName.c_str(), fUseAckn, fPortCQ, fPortQP);
   fPortQP = 0;
   fPortCQ = 0;
   DOUT1(("CREATE SERVER %s", fConnId.c_str()));
   dabc::Command::Reply(fCmd, true);
   fCmd = 0;
   fConnected = true;

   DestroyProcessor();
}

void verbs::ProtocolProcessor::VerbsProcessOperError(uint32_t bufid)
{
   EOUT(("VerbsProtocolProcessor error %s", fConnId.c_str()));
}
