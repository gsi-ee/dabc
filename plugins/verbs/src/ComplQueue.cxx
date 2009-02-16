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
#include "verbs/ComplQueue.h"

#include "dabc/logging.h"

verbs::ComplQueue::ComplQueue(struct ibv_context *context,
                       int size,
                       struct ibv_comp_channel *channel) :
   f_cq(0),
   f_channel(channel)
{
   if (f_channel==0) {
      EOUT(("Complition channel not specified"));
      return;
   }

   f_cq = ibv_create_cq(context, size, &fCQContext, f_channel, 0);
   if (f_cq==0)
      EOUT(("Couldn't allocate completion queue (CQ)"));

   fCQContext.events_get = 0;
   fCQContext.itself = this;
   fCQContext.own_cq = f_cq;

   ibv_req_notify_cq(f_cq, 0);
}


verbs::ComplQueue::~ComplQueue()
{
   if (fCQContext.events_get > 0) {
      ibv_ack_cq_events(f_cq, fCQContext.events_get);
      fCQContext.events_get = 0;
   }

   if (ibv_destroy_cq(f_cq))
      EOUT(("Fail to destroy CQ"));
}

