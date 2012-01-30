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

#include "verbs/ComplQueue.h"

#include "dabc/logging.h"
#include "dabc/timing.h"

#include <poll.h>
#include <math.h>
#include <errno.h>

verbs::ComplQueue::ComplQueue(ContextRef ctx, int size,
                                    struct ibv_comp_channel *channel, bool useownchannel) :
   fContext(ctx),
   f_cq(0),
   f_channel(channel),
   f_ownchannel(false)
{
   if ((f_channel==0) && useownchannel) {
      f_ownchannel = true;
      f_channel = ibv_create_comp_channel(fContext.context());
   }

   if (f_channel==0) {
      EOUT(("Completion channel not specified"));
      return;
   }

   f_cq = ibv_create_cq(fContext.context(), size, &fCQContext, f_channel, 0);
   if (f_cq==0)
      EOUT(("Couldn't allocate completion queue (CQ)"));

   fCQContext.events_get = 0;
   fCQContext.itself = this;
   fCQContext.own_cq = f_cq;

   ibv_req_notify_cq(f_cq, 0);
}


verbs::ComplQueue::~ComplQueue()
{
   AcknoledgeEvents();

   if (ibv_destroy_cq(f_cq))
      EOUT(("Fail to destroy CQ"));

   f_cq = 0;

   if (f_ownchannel)
      ibv_destroy_comp_channel(f_channel);
   f_channel = 0;
}


int verbs::ComplQueue::Poll()
{
   // return 0 if no events
   //        1 if something exist
   //        2 error

   int ne = ibv_poll_cq(cq(), 1, &f_wc);

   if (ne==0) return 0;

   if (ne<0) {
      EOUT(("ibv_poll_cq error"));
      return 2;
   }

   if (f_wc.status != IBV_WC_SUCCESS) {
      EOUT(("Completion error=%d wr_id=%llu syndrom 0x%x qpnum=%x src_qp=%x",
      f_wc.status, f_wc.wr_id, f_wc.vendor_err, f_wc.qp_num, f_wc.src_qp));
      return 2;
   }

   return 1;
}

int verbs::ComplQueue::Wait(double timeout, double fasttm)
{
   int res = Poll();

   if ((res!=0) || (timeout<=0.) || (f_channel==0)) return res;

   dabc::TimeStamp now = dabc::Now();
   dabc::TimeStamp finish = now + timeout;
   dabc::TimeStamp fastfinish = now + fasttm;

   struct pollfd ufds;
   int timeout_ms(0), status(0);

   ufds.fd = f_channel->fd;
   ufds.events = POLLIN;
   ufds.revents = 0;

   while (now < finish) {

      if (now < fastfinish) {
         // polling some portion of time in the beginning
         res = Poll();
         if (res!=0) return res;
      } else {

         timeout_ms = lrint((finish-now)*1000);

         // no need to wait while no timeout is remaining
         if (timeout_ms==0) return Poll();

         status = poll(&ufds, 1, timeout_ms);

         if (status==0) return Poll();

         if (status>0) break;

         if ((status==-1) && (errno != EINTR)) {
            EOUT(("Error when waiting IB event"));
            return 2;
         }
      }

      now = dabc::Now();
   }

//   DOUT((3,"After wait revents = %d expects %d",ufds.revents,ufds.events));

   struct ibv_cq *ev_cq;
   ComplQueueContext *ev_ctx(0);
   if (ibv_get_cq_event(f_channel, &ev_cq, (void**)&ev_ctx)) {
      EOUT(("Failed to get cq_event"));
      return 2;
   }

   if ((ev_ctx==0) || (ev_ctx->own_cq != ev_cq)) {
      EOUT(("Error with getting context after ibv_get_cq_event"));
      return 2;
   }

   // instead of acknowledging every event, do it very rearly
   // ibv_ack_cq_events(ev_cq, 1);

   if (ev_ctx->events_get++ > 1000) {
      ibv_ack_cq_events(ev_cq, ev_ctx->events_get);
      ev_ctx->events_get = 0;
   }

   if (ibv_req_notify_cq(ev_cq, 0)) {
      EOUT(("Couldn't request CQ notification"));
      return 2;
   }

   return Poll();
}

void verbs::ComplQueue::AcknoledgeEvents()
{
   if (fCQContext.events_get > 0) {
      ibv_ack_cq_events(f_cq, fCQContext.events_get);
      fCQContext.events_get = 0;
   }
}

