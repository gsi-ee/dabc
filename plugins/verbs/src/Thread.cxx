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

#include "verbs/Thread.h"

#include <sys/poll.h>
#include <unistd.h>

#include <infiniband/verbs.h>

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Command.h"

#include "verbs/Device.h"
#include "verbs/QueuePair.h"
#include "verbs/ComplQueue.h"
#include "verbs/MemoryPool.h"
#include "verbs/Worker.h"

#ifndef VERBS_USING_PIPE

// ____________________________________________________________________

namespace verbs {

   /** \brief Timeout producer for \ref verbs::Thread when pipe cannot be used */

   class TimeoutWorker : public dabc::Worker {

      protected:
         Thread *fVerbsThrd;
         double fLastTm;

      public:
         TimeoutWorker(Thread* thrd) :
            dabc::Worker(),
            fVerbsThrd(thrd),
            fLastTm(-1)
         {
         }

         void OnThreadAssigned()
         {
            dabc::Worker::OnThreadAssigned();
            ActivateTimeout(fLastTm);
         }


         void DoTimeout(double tmout)
         {
            fLastTm = tmout;
            if (HasThread())
               ActivateTimeout(tmout);
         }

         virtual double ProcessTimeout(double)
         {
            if (fVerbsThrd) fVerbsThrd->FireDoNothingEvent();
            return -1;
         }

         virtual const char* ClassName() const { return "verbs::TimeoutWorker"; }

   };

}

#endif

// ____________________________________________________________________

verbs::Thread::Thread(dabc::Reference parent, const std::string &name, dabc::Command cmd, ContextRef ctx) :
   dabc::Thread(parent, name, cmd),
   fContext(ctx),
   fChannel(nullptr),
#ifndef VERBS_USING_PIPE
   fLoopBackQP(0),
   fLoopBackPool(0),
   fLoopBackCnt(0),
   fTimeout(0),
#endif
   fMainCQ(0),
   fWaitStatus(wsWorking),
   fWCSize(0),
   fWCs(nullptr),
   fFastModus(0),
   fCheckNewEvents(true)
{
   fChannel = ibv_create_comp_channel(fContext.context());
   if (fChannel==0) {
      EOUT("Cannot create completion channel - HALT");
      exit(143);
   }

   fWCSize = 128;
   fWCs = new struct ibv_wc[fWCSize];

   #ifdef VERBS_USING_PIPE

   fPipe[0] = 0;
   fPipe[1] = 0;
   pipe(fPipe);

   #else

   fLoopBackQP = new QueuePair(fContext, IBV_QPT_RC,
                                MakeCQ(), LoopBackQueueSize, 1,
                                MakeCQ(), LoopBackQueueSize, 1);

   if (!fLoopBackQP->Connect(fContext.lid(), fLoopBackQP->qp_num(), fLoopBackQP->local_psn())) {
      EOUT("fLoopBackQP CONNECTION FAILED");
      exit(144);
   }

   fLoopBackPool = new MemoryPool(fContext, "LoopBackPool", 2, 16, false);

   #endif

   DOUT3("Verbs thread %s %p is created", GetName(), this);
}

verbs::Thread::~Thread()
{
   CloseThread();

   DOUT3("Verbs thread %p %s destroyed", this, GetName());
}

bool verbs::Thread::CompatibleClass(const std::string &clname) const
{
   if (dabc::Thread::CompatibleClass(clname)) return true;
   return clname == verbs::typeThread;
}

void verbs::Thread::CloseThread()
{
   DOUT2("verbs::Thread::CloseThread() %s", GetName());

   if (!IsItself())
      Stop(2.);
   else
      EOUT("Bad idea - close thread from itself");

   DOUT2("verbs::Thread::CloseThread() %s - did stop", GetName());


   #ifdef VERBS_USING_PIPE
   if (fPipe[0] != 0) {
      close(fPipe[0]);
      fPipe[0] = 0;
   }
   if (fPipe[1] != 0) {
      close(fPipe[1]);
      fPipe[1] = 0;
   }
   #else
   if (fTimeout) { delete fTimeout; fTimeout = nullptr; }
   delete fLoopBackQP; fLoopBackQP = nullptr;
   delete fLoopBackPool; fLoopBackPool = nullptr;
   #endif

   if (fMainCQ) { delete fMainCQ; fMainCQ = nullptr; }

   ibv_destroy_comp_channel(fChannel);

   if (fWCs) {
      delete [] fWCs;
      fWCs = nullptr;
      fWCSize = 0;
   }

   DOUT2("verbs::Thread::CloseThread() %s done", GetName());
}

verbs::ComplQueue* verbs::Thread::MakeCQ()
{
   if (!fMainCQ)
      fMainCQ = new ComplQueue(fContext, 10000, fChannel);

   return fMainCQ;
}

int verbs::Thread::ExecuteThreadCommand(dabc::Command cmd)
{
   if (cmd.IsName("EnableFastModus")) {
      fFastModus = cmd.GetInt("PoolingCounter", 1000);
      return dabc::cmd_true;
   } else
   if (cmd.IsName("DisableFastModus")) {
      fFastModus = 0;
      return dabc::cmd_true;
   }

   return dabc::Thread::ExecuteThreadCommand(cmd);
}

void verbs::Thread::_Fire(const dabc::EventId& evnt, int nq)
{
   DOUT4("verbs::Thread %s   ::_Fire %s status %d", GetName(), evnt.asstring().c_str(), fWaitStatus);

   _PushEvent(evnt, nq);
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

bool verbs::Thread::WaitEvent(dabc::EventId& evid, double tmout_sec)
{

   {
      dabc::LockGuard lock(ThreadMutex());

      // if we already have events in the queue,
      // check if we take them out or first check if new verbs events there

      if (_TotalNumberOfEvents() > 0) {

         if (!fCheckNewEvents) return _GetNextEvent(evid);

         // we have events in the queue, therefore do not wait - just check new events
         tmout_sec = 0.;
      }

      fWaitStatus = wsWaiting;
   }

   struct ibv_cq *ev_cq = nullptr;
   int nevents = 0;

   if ((fFastModus > 0) && fMainCQ)
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

   DOUT5("VerbsThrd:%s start poll tmout:%d", GetName(), tmout);

   int res = poll(ufds, 2, tmout);

   DOUT5("VerbsThrd:%s did poll res:%d", GetName(), res);

   // if no events on the main channel
   if ((res <= 0) || (ufds[0].revents == 0)) {

      dabc::LockGuard lock(ThreadMutex());
      if (fWaitStatus == wsFired) {
         char sbuf;
         read(fPipe[0], &sbuf, 1);
      }

      fWaitStatus = wsWorking;

      return _GetNextEvent(evid);
   }

#else

   if (tmout_sec>=0.) {
      if (fTimeout==0) {
         fTimeout = new TimeoutWorker(this);
         fTimeout->AssignToThread(dabc::mgr()->thread(), false);
      }
      fTimeout->DoTimeout(tmout_sec);
   }

#endif

   void *ev_ctx = nullptr;

//      DOUT1("Call ibv_get_cq_event");

   if (ibv_get_cq_event(fChannel, &ev_cq, &ev_ctx)) {
      EOUT("ERROR when waiting for cq event");
   }

   ibv_req_notify_cq(ev_cq, 0);

   if(ev_ctx) {
      ComplQueueContext* cq_ctx = (ComplQueueContext*) ev_ctx;
      if (cq_ctx->own_cq != ev_cq) {
         EOUT("Mismatch in cq context");
         exit(145);
      }

      cq_ctx->events_get++;
      if (cq_ctx->events_get>1000000) {
         ibv_ack_cq_events(ev_cq, cq_ctx->events_get);
         cq_ctx->events_get = 0;
      }
   } else
      ibv_ack_cq_events(ev_cq, 1);

   } // nevents==0

   dabc::LockGuard lock(ThreadMutex());

#ifdef VERBS_USING_PIPE
   if (fWaitStatus == wsFired) {
      char sbuf;
      read(fPipe[0], &sbuf, 1);
   }
#endif

   fWaitStatus = wsWorking;

   bool isany = false;

   while (true) {
      if (nevents == 0)
         nevents = ibv_poll_cq(ev_cq, fWCSize, fWCs);

      if (nevents <= 0) break;

      struct ibv_wc* wc = fWCs;

      while (nevents-->0) {

         uint32_t procid = fMap[wc->qp_num];

         if (procid != 0) {
            uint16_t evnt = 0;
            if (wc->status != IBV_WC_SUCCESS) {
               evnt = WorkerAddon::evntVerbsError;
               EOUT("Verbs error %s isrecv %s operid %lu", StatusStr(wc->status), DBOOL(wc->opcode & IBV_WC_RECV), (long unsigned) wc->wr_id);
            }
            else
               if (wc->opcode & IBV_WC_RECV)
                  evnt = WorkerAddon::evntVerbsRecvCompl;
               else
                  evnt = WorkerAddon::evntVerbsSendCompl;

           _PushEvent(dabc::EventId(evnt, procid, wc->wr_id), 1);

           isany = true;

           // FIXME: we should increase number of fired events by worker
           IncWorkerFiredEvents(fWorkers[procid]->work);
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

   // we put additional event to enable again events checking after current events are processed
   if (isany) {
      fCheckNewEvents = false;
      _PushEvent(evntEnableCheck, 1);
   }

   return _GetNextEvent(evid);
}

void verbs::Thread::ProcessExtraThreadEvent(const dabc::EventId& evid)
{
   switch (evid.GetCode()) {
      case evntEnableCheck:
         fCheckNewEvents = true;
         break;
      default:
         dabc::Thread::ProcessExtraThreadEvent(evid);
         break;
   }
}

void verbs::Thread::WorkersSetChanged()
{
   // we do not need locks while fWorkers and fMap can be changed only inside the thread

   DOUT5("WorkersNumberChanged started size:%u", fWorkers.size());

   fMap.clear();

   for (unsigned indx=0;indx<fWorkers.size();indx++) {
      verbs::WorkerAddon* addon = dynamic_cast<verbs::WorkerAddon*> (fWorkers[indx]->addon);

      DOUT5("Test processor %u: work %p addon %p", indx, fWorkers[indx]->work, addon);

      if (!addon || !addon->QP()) continue;

      fMap[addon->QP()->qp_num()] = indx;

      if (fWorkers[indx]->work->WorkerId() != indx) {
         EOUT("Mismatch of worker id");
         exit(44);
      }
   }

   fCheckNewEvents = true;

   DOUT5("WorkersNumberChanged finished");
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
