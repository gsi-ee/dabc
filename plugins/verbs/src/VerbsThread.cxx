#include "dabc/VerbsThread.h"

#include <sys/poll.h>

#include <infiniband/verbs.h>

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"
#include "dabc/Command.h"

#include "dabc/VerbsDevice.h"
#include "dabc/VerbsQP.h"
#include "dabc/VerbsCQ.h"
#include "dabc/VerbsMemoryPool.h"

ibv_gid VerbsConnThrd_Multi_Gid =  {   {
      0xFF, 0x12, 0xA0, 0x1C,
      0xFE, 0x80, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x33, 0x44, 0x55, 0x66
    } };


typedef struct VerbsConnectData {
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

// ____________________________________________________________________


dabc::VerbsProcessor::VerbsProcessor(VerbsQP* qp) : 
   WorkingProcessor(),
   fQP(qp)
{
}
         
dabc::VerbsProcessor::~VerbsProcessor()
{
   CloseQP();
}

void dabc::VerbsProcessor::SetQP(VerbsQP* qp)
{
   CloseQP();
   fQP = qp;
}

void dabc::VerbsProcessor::CloseQP()
{
   if (fQP!=0) {
      delete fQP;
      fQP = 0;   
   }
}

void dabc::VerbsProcessor::ProcessEvent(dabc::EventId evnt)
{
   switch (GetEventCode(evnt)) {

      case evntVerbsSendCompl:
         VerbsProcessSendCompl(GetEventArg(evnt));
         break;
         
      case evntVerbsRecvCompl:
         VerbsProcessRecvCompl(GetEventArg(evnt));
         break;
         
      case evntVerbsError:
         VerbsProcessOperError(GetEventArg(evnt));
         break;
            
      default:
         WorkingProcessor::ProcessEvent(evnt);
   }
}
// ___________________________________________________________________

dabc::VerbsConnectProcessor::VerbsConnectProcessor(VerbsThread* thrd) : 
   VerbsProcessor(0),
   fCQ(0),
   fPool(0),
   fUseMulti(false),
   fMultiAH(0),
   fConnCounter(0)
{
   VerbsDevice* dev = thrd->GetDevice();
   
   if (ServBufferSize < sizeof(VerbsConnectData)) {
      EOUT(("VerbsConnectData do not fit in UD buffer %d", sizeof(VerbsConnectData)));
      exit(1);
   }

   fCQ = new VerbsCQ(dev->context(), ServQueueSize*3, thrd->Channel());

   VerbsQP* qp = new VerbsQP(dev, IBV_QPT_UD, 
                             fCQ, ServQueueSize, 1, 
                             fCQ, ServQueueSize, 1);
   if (!qp->InitUD()) {
      EOUT(("fConnQP INIT FALSE"));
      exit(1);
   }
   
   SetQP(qp);
   
  if (dev->IsMulticast()) 
      if (dev->RegisterMultiCastGroup(&VerbsConnThrd_Multi_Gid, fMultiLID)) {
         fUseMulti = true;   
         
         if (qp->AttachMcast(&VerbsConnThrd_Multi_Gid, fMultiLID))
            fMultiAH = dev->CreateMAH(&VerbsConnThrd_Multi_Gid, fMultiLID);
      }
      
   fPool = new VerbsMemoryPool(dev, "ConnPool", ServQueueSize*2, ServBufferSize, true);
   
   for (int n=0;n<ServQueueSize;n++) {
      BufferId_t bufid = 0;
      if (!fPool->TakeRawBuffer(bufid)) {
         EOUT(("No free memory in connection pool"));
      } else   
         fQP->Post_Recv(fPool->GetRecvWR(bufid));
   }
}
   
dabc::VerbsConnectProcessor::~VerbsConnectProcessor()
{
   if (fUseMulti) {

      VerbsDevice* dev = ((VerbsThread*)ProcessorThread())->GetDevice();

      fQP->DetachMcast(&VerbsConnThrd_Multi_Gid, fMultiLID);
      
      dev->UnRegisterMultiCastGroup(&VerbsConnThrd_Multi_Gid, fMultiLID);
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

void dabc::VerbsConnectProcessor::VerbsProcessSendCompl(uint32_t bufid)
{
   fPool->ReleaseRawBuffer(bufid);
}

void dabc::VerbsConnectProcessor::VerbsProcessRecvCompl(uint32_t bufid)
{
   VerbsConnectData* inp = (VerbsConnectData*) fPool->GetSendBufferLocation(bufid);
   
   VerbsProtocolProcessor* findrec = 0;
   
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

void dabc::VerbsConnectProcessor::VerbsProcessOperError(uint32_t bufid)
{
   EOUT(("Error in connection processor !!!!!!"));
   fPool->ReleaseRawBuffer(bufid);
}

bool dabc::VerbsConnectProcessor::TrySendConnRequest(VerbsProtocolProcessor* rec)
{
   if ((rec==0) || (rec->fKindStatus==0)) return false;
   
   BufferId_t bufid = 0;
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
       tgt_qpn = 0xffffff;
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


dabc::VerbsProtocolProcessor::VerbsProtocolProcessor(VerbsThread* thrd,
                                                     VerbsQP* hs_qp,
                                                     bool server,
                                                     const char* portname,
                                                     Command* cmd) :
   VerbsProcessor(hs_qp),
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
   fPool = new VerbsMemoryPool(thrd->GetDevice(), "HandshakePool", 1, 1024, false);
}

dabc::VerbsProtocolProcessor::~VerbsProtocolProcessor()
{
   if (fPool) delete fPool; fPool = 0;
   
   if(f_ah!=0) ibv_destroy_ah(f_ah);
   
   CloseQP();
   if (fPortQP) delete fPortQP; fPortQP = 0;
   if (fPortCQ) delete fPortCQ; fPortCQ = 0;
   
   dabc::Command::Reply(fCmd, false);
}

void dabc::VerbsProtocolProcessor::OnThreadAssigned()
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


double dabc::VerbsProtocolProcessor::ProcessTimeout(double last_diff)
{
   if (fConnected) return -1;

   fTimeout -= last_diff;

   DOUT5(("Timedout %s %s restmout %5.1f", (fServer ? "SERVER" : "CLIENT"), fConnId.c_str(), fTimeout)); 
   
   VerbsConnectProcessor* conn = thrd()->Connect();
   
   if ((fTimeout<0) || (conn==0)) {
      EOUT(("Connection failed %s", fConnId.c_str()));
      DestroyProcessor();
      return -1;  
   }
   
   double next_tmout = 0.5; // default timeout
   
   if (fKindStatus!=0) 
     if (conn->TrySendConnRequest(this))
        next_tmout = 1.0;
     else
        next_tmout = 0.1; // repeat after short time;   
    
   return next_tmout;
}

bool dabc::VerbsProtocolProcessor::StartHandshake(bool dorecv, void* connrec)
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

   BufferId_t bufid = 0;
   
   if (dorecv) {
      QP()->Post_Recv(fPool->GetRecvWR(bufid));
   } else {
      strcpy((char*) fPool->GetSendBufferLocation(bufid), fConnId.c_str());
      QP()->Post_Send(fPool->GetSendWR(bufid, fConnId.length()+1));
   }
   
   return true;
}


void dabc::VerbsProtocolProcessor::VerbsProcessSendCompl(uint32_t bufid)
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
    
   thrd()->GetDevice()->CreateVerbsTransport(fThrdName.c_str(), fPortName.c_str(), fPortCQ, fPortQP);
   fPortQP = 0;
   fPortCQ = 0;
               
   DOUT1(("CREATE CLIENT %s",  fConnId.c_str()));
               
   dabc::Command::Reply(fCmd, true);
   fCmd = 0;
   fConnected = true;

   DestroyProcessor();
}

void dabc::VerbsProtocolProcessor::VerbsProcessRecvCompl(uint32_t bufid)
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
      
   thrd()->GetDevice()->CreateVerbsTransport(fThrdName.c_str(), fPortName.c_str(), fPortCQ, fPortQP);
   fPortQP = 0;
   fPortCQ = 0;
   DOUT1(("CREATE SERVER %s", fConnId.c_str()));
   dabc::Command::Reply(fCmd, true);
   fCmd = 0;
   fConnected = true;
   
   DestroyProcessor();
}

void dabc::VerbsProtocolProcessor::VerbsProcessOperError(uint32_t bufid)
{
   EOUT(("VerbsProtocolProcessor error %s", fConnId.c_str()));
}

#ifndef VERBS_USING_PIPE

// ____________________________________________________________________

namespace dabc {

   class VerbsTimeoutProcessor : public WorkingProcessor {
      
      protected:
         VerbsThread *fVerbsThrd;
         double fLastTm;
         
      public:
         VerbsTimeoutProcessor(VerbsThread* thrd) : 
            WorkingProcessor(),
            fVerbsThrd(thrd),
            fLastTm(-1)
         {
         }
         
         void OnThreadAssigned()
         {
            ActivateTimeout(fLastTm);
         }

         
         void DoTimeout(double tmout)
         {
            fLastTm = tmout;
            if (ProcessorThread()!=0) 
               ActivateTimeout(tmout); 
         }
            
         virtual double ProcessTimeout(double)
         {
            if (fVerbsThrd) fVerbsThrd->FireDoNothingEvent();
            return -1;
         }

   };
 
}

#endif

// ____________________________________________________________________

dabc::VerbsThread::VerbsThread(VerbsDevice* dev, Basic* parent, const char* name) : 
   WorkingThread(parent, name),
   fDevice(dev),
   fChannel(0),
#ifndef VERBS_USING_PIPE
   fLoopBackQP(0),
   fLoopBackPool(0),
   fLoopBackCnt(0),
   fTimeout(0),
#endif
   fMainCQ(0),
   fFireCounter(0),
   fWaitStatus(wsWorking),
   fWCSize(0),
   fWCs(0),
   fConnect(0),
   fFastModus(0)
{
   fChannel = ibv_create_comp_channel(fDevice->context());
   if (fChannel==0) {
      EOUT(("Cannot create completion channel - HALT"));
      exit(1);   
   }

   fWCSize = 128;
   fWCs = new struct ibv_wc[fWCSize];
   
   #ifdef VERBS_USING_PIPE

   fPipe[0] = 0;
   fPipe[1] = 0;
   pipe(fPipe);   

   #else
   
   fLoopBackQP = new VerbsQP(GetDevice(), IBV_QPT_RC, 
                             MakeCQ(), LoopBackQueueSize, 1,
                             MakeCQ(), LoopBackQueueSize, 1);
                                       
   if (!fLoopBackQP->Connect(GetDevice()->lid(), fLoopBackQP->qp_num(), fLoopBackQP->local_psn())) {
      EOUT(("fLoopBackQP CONNECTION FAILED"));
      exit(1);
   }
   
   fLoopBackPool = new VerbsMemoryPool(GetDevice(), "LoopBackPool", 2, 16, false);
   
   #endif

   DOUT3(("Verbs thread %s %p is created", GetName(), this));
}
   
dabc::VerbsThread::~VerbsThread()
{
   CloseThread();
   
   DOUT3(("Verbs thread %s destroyed", GetName()));
}

bool dabc::VerbsThread::CompatibleClass(const char* clname) const
{
   if (WorkingThread::CompatibleClass(clname)) return true;
   return strcmp(clname, "VerbsThread") == 0;
}

void dabc::VerbsThread::CloseThread()
{
   DOUT3(("VerbsThread::CloseThread() %s", GetName()));
    
   if (fConnect!=0) { delete fConnect; fConnect = 0; }
   
   if (!IsItself()) 
      Stop(true);
   else
      EOUT(("Bad idea - close thread from itself"));
   
   #ifdef VERBS_USING_PIPE
   if (fPipe[0] != 0) close(fPipe[0]); fPipe[0] = 0;
   if (fPipe[1] != 0) close(fPipe[1]); fPipe[1] = 0;

   #else
   if (fTimeout!=0) { delete fTimeout; fTimeout = 0; }
   delete fLoopBackQP; fLoopBackQP = 0;
   delete fLoopBackPool; fLoopBackPool = 0;
   #endif

   if (fMainCQ!=0) { delete fMainCQ; fMainCQ = 0; } 
    
   ibv_destroy_comp_channel(fChannel);
   
   if (fWCs!=0) {
      delete [] fWCs;
      fWCs = 0;
      fWCSize = 0;
   }

   DOUT3(("VerbsThread::CloseThread() %s done", GetName()));
}


dabc::VerbsCQ* dabc::VerbsThread::MakeCQ()
{
   if (fMainCQ!=0) return fMainCQ;
   
   fMainCQ = new VerbsCQ(fDevice->context(), 10000, fChannel);
   
   return fMainCQ;
}

int dabc::VerbsThread::ExecuteCommand(Command* cmd)
{
   int cmd_res = cmd_true; 
   if (cmd->IsName("EnableFastModus")) 
      fFastModus = cmd->GetInt("PoolingCounter", 1000);  
   else
   if (cmd->IsName("DisableFastModus")) 
      fFastModus = 0;
   else
      cmd_res = WorkingThread::ExecuteCommand(cmd);
   
   return cmd_res;
}

void dabc::VerbsThread::FireDoNothingEvent()
{
   // used by timout object to activate thread and leave WaitEvent function
   
   Fire(CodeEvent(evntDoNothing), -1);
}

void dabc::VerbsThread::_Fire(dabc::EventId arg, int nq)
{
   DOUT2(("dabc::VerbsThread %s   ::_Fire %lx status %d", GetName(), arg, fWaitStatus));
   
   _PushEvent(arg, nq);
   fFireCounter++;
   if (fWaitStatus == wsWaiting) 
   #ifdef VERBS_USING_PIPE
     {
        write(fPipe[1],"w", 1); 
        fWaitStatus = wsFired; 
     }  
   #else
      if (fLoopBackCnt<LoopBackQueueSize*2) {
         fLoopBackQP->Post_Recv(fLoopBackPool->GetRecvWR(0)); 
         fLoopBackQP->Post_Send(fLoopBackPool->GetSendWR(1, 0));
         fLoopBackCnt+=2;
         fWaitStatus = wsFired;
      }
   #endif
}

dabc::EventId dabc::VerbsThread::WaitEvent(double tmout_sec)
{
   
//   if (tmout_sec>=0) EOUT(("Non-empty timeout"));
   
   {
      dabc::LockGuard lock(fWorkMutex);

      if (fFireCounter>0) {
         fFireCounter--;
         return _GetNextEvent();
      }
      
      fWaitStatus = wsWaiting;
   }

   struct ibv_cq *ev_cq = 0;
   int nevents = 0;
   
   if ((fFastModus>0) && (fMainCQ!=0)) 
     for (int n=0;n<fFastModus;n++) {
        nevents = ibv_poll_cq(fMainCQ->cq(), fWCSize, fWCs);
        if (nevents>0) {
           ev_cq = fMainCQ->cq();
           break; 
        }
     }   

   if (nevents == 0) {


#ifdef VERBS_USING_PIPE

   struct pollfd ufds[2];

   ufds[0].fd = fChannel->fd;
   ufds[0].events = POLLIN;
   ufds[0].revents = 0;

   ufds[1].fd = fPipe[0];
   ufds[1].events = POLLIN;
   ufds[1].revents = 0;

   int tmout = tmout_sec < 0 ? -1 : int(tmout_sec*1000.);
   
   DOUT5(("VerbsThrd:%s start poll tmout:%d", GetName(), tmout));
   
   int res = poll(ufds, 2, tmout);

   DOUT5(("VerbsThrd:%s did poll res:%d", GetName(), res));
   
   // if no events on the main channel
   if ((res <= 0) || (ufds[0].revents == 0)) {
       
      dabc::LockGuard lock(fWorkMutex); 
      if (fWaitStatus == wsFired) {
         char sbuf;
         read(fPipe[0], &sbuf, 1);   
      }
      
      fWaitStatus = wsWorking;
      
      if (fFireCounter==0) return NullEventId;
      fFireCounter--;
      return _GetNextEvent();
   }
   
#else

   if (tmout_sec>=0.) {
      if (fTimeout==0) {
         fTimeout = new VerbsTimeoutProcessor(this);
         fTimeout->AssignProcessorToThread(GetManager()->ProcessorThread(), false);
      }  
      fTimeout->DoTimeout(tmout_sec);
   }
   
#endif
   
   void *ev_ctx = 0;
       
//      DOUT1(("Call ibv_get_cq_event")); 
       
   if (ibv_get_cq_event(fChannel, &ev_cq, &ev_ctx)) {
      EOUT(("ERROR when waiting for cq event"));
   } 

   ibv_req_notify_cq(ev_cq, 0);

   if(ev_ctx!=0) {
      VerbsCQContext* cq_ctx = (VerbsCQContext*) ev_ctx;
      if (cq_ctx->own_cq != ev_cq) {
         EOUT(("Missmatch in cq context"));
         exit(1);
      }
      
      cq_ctx->events_get++;
      if (cq_ctx->events_get>1000000) {
         ibv_ack_cq_events(ev_cq, cq_ctx->events_get); 
         cq_ctx->events_get = 0;
      }
   } else 
      ibv_ack_cq_events(ev_cq, 1);

   } // nevents==0

   dabc::LockGuard lock(fWorkMutex);

#ifdef VERBS_USING_PIPE
   if (fWaitStatus == wsFired) {
      char sbuf;
      read(fPipe[0], &sbuf, 1);   
   }
#endif

   fWaitStatus = wsWorking;

   
   while (true) {
      if (nevents==0)
         nevents = ibv_poll_cq(ev_cq, fWCSize, fWCs);
      
      if (nevents<=0) break;
      
      struct ibv_wc* wc = fWCs;
      
      while (nevents-->0) {
          
         uint32_t procid = fMap[wc->qp_num]; 

         if (procid!=0) {
            uint16_t evnt = 0; 
            if (wc->status != IBV_WC_SUCCESS) {
               evnt = VerbsProcessor::evntVerbsError;
               EOUT(("Verbs error %s isrecv %s operid %u", StatusStr(wc->status), DBOOL(wc->opcode & IBV_WC_RECV), wc->wr_id));
            }
            else   
               if (wc->opcode & IBV_WC_RECV) 
                  evnt = VerbsProcessor::evntVerbsRecvCompl;
               else   
                  evnt = VerbsProcessor::evntVerbsSendCompl;
            
           _PushEvent(CodeEvent(evnt, procid, wc->wr_id), 1);
           fFireCounter++;
#ifdef VERBS_USING_PIPE
         }
#else         
         } else {
            if (fLoopBackQP->qp_num()==wc->qp_num) 
               fLoopBackCnt--;
         }
#endif
         wc++;
      }
   }

   if (fFireCounter==0) return NullEventId;
   fFireCounter--;
   return _GetNextEvent();
}
         
void dabc::VerbsThread::ProcessorNumberChanged()
{
   // we do not need locks while fProcessors and fMap can be changed only inside the thread
   
   DOUT5(("ProcessorNumberChanged started size:%u", fProcessors.size() ));
   
   fMap.clear();
   
   for (unsigned indx=0;indx<fProcessors.size();indx++) {
      DOUT5(("Test processor %u: %p", indx, fProcessors[indx]));
       
      VerbsProcessor* proc = dynamic_cast<VerbsProcessor*> (fProcessors[indx]);
      
      if (proc==0) continue;
      
      fMap[proc->QP()->qp_num()] = proc->ProcessorId();
   }
   
   DOUT5(("ProcessorNumberChanged finished"));
}


bool dabc::VerbsThread::DoServer(Command* cmd, Port* port, const char* portname)
{
   if ((cmd==0) || (port==0)) return false;

   DOUT3(("dabc::VerbsThread::DoServer %s", portname));

   if (!StartConnectProcessor()) return false;
   
   VerbsQP* hs_qp = new VerbsQP(fDevice, IBV_QPT_RC, 
                                MakeCQ(), 2, 1,
                                MakeCQ(), 2, 1);

   VerbsProtocolProcessor* rec = new VerbsProtocolProcessor(this, hs_qp, true, portname, cmd);
   
   rec->fConnType = cmd->GetInt("VerbsConnType", IBV_QPT_RC);
   rec->fTimeout = cmd->GetInt("Timeout", 10);
   const char* connid = cmd->GetPar("ConnId");
   if (connid!=0)
      rec->fConnId = connid;
   else    
      dabc::formats(rec->fConnId, "LID%04x-QPN%08x-CNT%04x", fDevice->lid(), fConnect->QP()->qp_num(), fConnect->NextConnCounter());

   DOUT3(("Start SERVER: %s", rec->fConnId.c_str()));

   cmd->SetPar("ServerId", FORMAT(("%04x:%08x", fDevice->lid(), fConnect->QP()->qp_num())));
   cmd->SetInt("VerbsConnType", rec->fConnType);
   
   cmd->SetPar("ConnId", rec->fConnId.c_str());
   
   cmd->SetBool("ServerUseAckn", port->IsUseAcknoledges());
   cmd->SetUInt("ServerHeaderSize", port->UserHeaderSize());

   rec->fThrdName = cmd->GetStr("TrThread","");

   fDevice->CreatePortQP(rec->fThrdName.c_str(), port, rec->fConnType, rec->fPortCQ, rec->fPortQP);

   rec->AssignProcessorToThread(this, false);

   return true; 
}

bool dabc::VerbsThread::DoClient(Command* cmd, Port* port, const char* portname)
{
   if ((cmd==0) || (port==0)) return false;
   
   DOUT3(("dabc::VerbsThread::DoClient %s", portname));

   if (!StartConnectProcessor()) return false;

   const char* connid = cmd->GetPar("ConnId");
   if (connid==0) {
      EOUT(("Connection ID not specified"));
      return false;    
   } 

   DOUT3(("Start CLIENT: %s", connid));
   
   bool useakcn = cmd->GetBool("ServerUseAckn", false);
   if (useakcn != port->IsUseAcknoledges()) {
      EOUT(("Missmatch in acknowledges usage in ports")); 
      port->ChangeUseAcknoledges(useakcn);
   }
      
   unsigned headersize = cmd->GetUInt("ServerHeaderSize", 0);
   if (headersize != port->UserHeaderSize()) {
      EOUT(("Missmatch in configured header sizes: %d %d", headersize, port->UserHeaderSize()));
      port->ChangeUserHeaderSize(headersize); 
   }

   VerbsQP* hs_qp = new VerbsQP(fDevice, IBV_QPT_RC, 
                                MakeCQ(), 2, 1,
                                MakeCQ(), 2, 1);

   VerbsProtocolProcessor* rec = new VerbsProtocolProcessor(this, hs_qp, false, portname, cmd);
   
   rec->fTimeout = cmd->GetInt("Timeout",10);
   rec->fConnId = connid;
   
   const char* serverid = cmd->GetPar("ServerId");
   if (serverid!=0) {
       unsigned int lid, qpn;
       sscanf(serverid, "%x:%x", &lid, &qpn);
       rec->fRemoteLID = lid;
       rec->fRemoteQPN = qpn;
   }
   
   rec->fConnType = cmd->GetInt("VerbsConnType", IBV_QPT_RC);
   rec->fKindStatus = 111; // this indicate, that we send request to server
   
   rec->fThrdName = cmd->GetStr("TrThread","");

   fDevice->CreatePortQP(rec->fThrdName.c_str(), port, rec->fConnType, rec->fPortCQ, rec->fPortQP);
      
   rec->AssignProcessorToThread(this, false);

   return true; 
}

void dabc::VerbsThread::FillServerId(String& servid)
{
   StartConnectProcessor();
   
   dabc::formats(servid, "%04x:%08x", GetDevice()->lid(), fConnect->QP()->qp_num());
}

bool dabc::VerbsThread::StartConnectProcessor()
{
   
   {
      dabc::LockGuard lock(fWorkMutex);
      if (fConnect!=0) return true;

      fConnect = new VerbsConnectProcessor(this);
   }
   
   fConnect->AssignProcessorToThread(this);
   return true;
}

dabc::VerbsProtocolProcessor* dabc::VerbsThread::FindProtocol(const char* connid)
{
   dabc::LockGuard lock(fWorkMutex);
   
   for (unsigned n=1; n<fProcessors.size(); n++) {
      VerbsProtocolProcessor* proc = dynamic_cast<VerbsProtocolProcessor*> ( fProcessors[n]);
      if (proc!=0) 
         if (proc->fConnId.compare(connid)==0) return proc;
   }
   
   return 0;
}

const char* dabc::VerbsThread::StatusStr(int code)
{
    static struct {
       const char  *name;
       ibv_wc_status  value;
    } verbs_status[] = {
#   define VERBSstatus(x) { # x, x }
    
    VERBSstatus (IBV_WC_SUCCESS),
    VERBSstatus (IBV_WC_LOC_LEN_ERR),
    VERBSstatus (IBV_WC_LOC_QP_OP_ERR),
    VERBSstatus (IBV_WC_LOC_EEC_OP_ERR),
    VERBSstatus (IBV_WC_LOC_PROT_ERR),
    VERBSstatus (IBV_WC_WR_FLUSH_ERR),
    VERBSstatus (IBV_WC_MW_BIND_ERR),
    VERBSstatus (IBV_WC_BAD_RESP_ERR),
    VERBSstatus (IBV_WC_LOC_ACCESS_ERR),
    VERBSstatus (IBV_WC_REM_INV_REQ_ERR),
    VERBSstatus (IBV_WC_REM_ACCESS_ERR),
    VERBSstatus (IBV_WC_REM_OP_ERR),
    VERBSstatus (IBV_WC_RETRY_EXC_ERR),
    VERBSstatus (IBV_WC_RNR_RETRY_EXC_ERR),
    VERBSstatus (IBV_WC_LOC_RDD_VIOL_ERR),
    VERBSstatus (IBV_WC_REM_INV_RD_REQ_ERR),
    VERBSstatus (IBV_WC_REM_ABORT_ERR),
    VERBSstatus (IBV_WC_INV_EECN_ERR),
    VERBSstatus (IBV_WC_INV_EEC_STATE_ERR),
    VERBSstatus (IBV_WC_FATAL_ERR),
    VERBSstatus (IBV_WC_RESP_TIMEOUT_ERR),
    VERBSstatus (IBV_WC_GENERAL_ERR)
#undef VERBSstatus    
    };
    
   for (unsigned i = 0;  i < sizeof(verbs_status)/sizeof(verbs_status[0]);  i++)
     if (verbs_status[i].value == code)
        return verbs_status[i].name;

   return "Invalid_VERBS_Status_Code";


}
