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
#include "verbs/Thread.h"

#include <sys/poll.h>

#include <infiniband/verbs.h>

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"
#include "dabc/Command.h"

#include "verbs/Device.h"
#include "verbs/QueuePair.h"
#include "verbs/ComplQueue.h"
#include "verbs/MemoryPool.h"
#include "verbs/Processor.h"

#ifndef VERBS_USING_PIPE

// ____________________________________________________________________

namespace verbs {

   class TimeoutProcessor : public dabc::WorkingProcessor {

      protected:
         Thread *fVerbsThrd;
         double fLastTm;

      public:
         VerbsTimeoutProcessor(Thread* thrd) :
            dabc::WorkingProcessor(),
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

verbs::Thread::Thread(Device* dev, dabc::Basic* parent, const char* name) :
	dabc::WorkingThread(parent, name),
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

   fLoopBackQP = new QueuePair(GetDevice(), IBV_QPT_RC,
                             MakeCQ(), LoopBackQueueSize, 1,
                             MakeCQ(), LoopBackQueueSize, 1);

   if (!fLoopBackQP->Connect(GetDevice()->lid(), fLoopBackQP->qp_num(), fLoopBackQP->local_psn())) {
      EOUT(("fLoopBackQP CONNECTION FAILED"));
      exit(1);
   }

   fLoopBackPool = new MemoryPool(GetDevice(), "LoopBackPool", 2, 16, false);

   #endif

   DOUT3(("Verbs thread %s %p is created", GetName(), this));
}

verbs::Thread::~Thread()
{
   CloseThread();

   DOUT3(("Verbs thread %s destroyed", GetName()));
}

bool verbs::Thread::CompatibleClass(const char* clname) const
{
   if (dabc::WorkingThread::CompatibleClass(clname)) return true;
   return strcmp(clname, VERBS_THRD_CLASSNAME) == 0;
}

void verbs::Thread::CloseThread()
{
   DOUT3(("verbs::Thread::CloseThread() %s", GetName()));

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

   DOUT3(("verbs::Thread::CloseThread() %s done", GetName()));
}


verbs::ComplQueue* verbs::Thread::MakeCQ()
{
   if (fMainCQ!=0) return fMainCQ;

   fMainCQ = new ComplQueue(fDevice->context(), 10000, fChannel);

   return fMainCQ;
}

int verbs::Thread::ExecuteCommand(dabc::Command* cmd)
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

void verbs::Thread::_Fire(dabc::EventId arg, int nq)
{
   DOUT2(("verbs::Thread %s   ::_Fire %lx status %d", GetName(), arg, fWaitStatus));

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

dabc::EventId verbs::Thread::WaitEvent(double tmout_sec)
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
         fTimeout = new TimeoutProcessor(this);
         fTimeout->AssignProcessorToThread(dabc::mgr()->ProcessorThread(), false);
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
      ComplQueueContext* cq_ctx = (ComplQueueContext*) ev_ctx;
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
               evnt = Processor::evntVerbsError;
               EOUT(("Verbs error %s isrecv %s operid %u", StatusStr(wc->status), DBOOL(wc->opcode & IBV_WC_RECV), wc->wr_id));
            }
            else
               if (wc->opcode & IBV_WC_RECV)
                  evnt = Processor::evntVerbsRecvCompl;
               else
                  evnt = Processor::evntVerbsSendCompl;

           _PushEvent(dabc::CodeEvent(evnt, procid, wc->wr_id), 1);
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

void verbs::Thread::ProcessorNumberChanged()
{
   // we do not need locks while fProcessors and fMap can be changed only inside the thread

   DOUT5(("ProcessorNumberChanged started size:%u", fProcessors.size() ));

   fMap.clear();

   for (unsigned indx=0;indx<fProcessors.size();indx++) {
      DOUT5(("Test processor %u: %p", indx, fProcessors[indx]));

      Processor* proc = dynamic_cast<Processor*> (fProcessors[indx]);

      if (proc==0) continue;

      fMap[proc->QP()->qp_num()] = proc->ProcessorId();
   }

   DOUT5(("ProcessorNumberChanged finished"));
}


bool verbs::Thread::DoServer(dabc::Command* cmd, dabc::Port* port, const char* portname)
{
   if ((cmd==0) || (port==0)) return false;

   DOUT3(("verbs::Thread::DoServer %s", portname));

   if (!StartConnectProcessor()) return false;

   QueuePair* hs_qp = new QueuePair(fDevice, IBV_QPT_RC,
                                    MakeCQ(), 2, 1,
                                    MakeCQ(), 2, 1);

   ProtocolProcessor* rec = new ProtocolProcessor(this, hs_qp, true, portname, cmd);

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

   cmd->SetUInt("ServerHeaderSize", port->UserHeaderSize());

   rec->fThrdName = cmd->GetStr("TrThread","");

   fDevice->CreatePortQP(rec->fThrdName.c_str(), port, rec->fConnType, rec->fPortCQ, rec->fPortQP);

   rec->AssignProcessorToThread(this, false);

   return true;
}

bool verbs::Thread::DoClient(dabc::Command* cmd, dabc::Port* port, const char* portname)
{
   if ((cmd==0) || (port==0)) return false;

   DOUT3(("verbs::Thread::DoClient %s", portname));

   if (!StartConnectProcessor()) return false;

   const char* connid = cmd->GetPar("ConnId");
   if (connid==0) {
      EOUT(("Connection ID not specified"));
      return false;
   }

   DOUT3(("Start CLIENT: %s", connid));

   unsigned headersize = cmd->GetUInt("ServerHeaderSize", 0);
   if (headersize != port->UserHeaderSize()) {
      EOUT(("Missmatch in configured header sizes: %d %d", headersize, port->UserHeaderSize()));
      port->ChangeUserHeaderSize(headersize);
   }

   QueuePair* hs_qp = new QueuePair(fDevice, IBV_QPT_RC,
                                MakeCQ(), 2, 1,
                                MakeCQ(), 2, 1);

   ProtocolProcessor* rec = new ProtocolProcessor(this, hs_qp, false, portname, cmd);

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

void verbs::Thread::FillServerId(std::string& servid)
{
   StartConnectProcessor();

   dabc::formats(servid, "%04x:%08x", GetDevice()->lid(), fConnect->QP()->qp_num());
}

bool verbs::Thread::StartConnectProcessor()
{

   {
      dabc::LockGuard lock(fWorkMutex);
      if (fConnect!=0) return true;

      fConnect = new ConnectProcessor(this);
   }

   fConnect->AssignProcessorToThread(this);
   return true;
}

verbs::ProtocolProcessor* verbs::Thread::FindProtocol(const char* connid)
{
   dabc::LockGuard lock(fWorkMutex);

   for (unsigned n=1; n<fProcessors.size(); n++) {
      ProtocolProcessor* proc = dynamic_cast<ProtocolProcessor*> ( fProcessors[n]);
      if (proc!=0)
         if (proc->fConnId.compare(connid)==0) return proc;
   }

   return 0;
}

const char* verbs::Thread::StatusStr(int code)
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
