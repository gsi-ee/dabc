#include "dabc/VerbsCQ.h"

#include "dabc/logging.h"

dabc::VerbsCQ::VerbsCQ(struct ibv_context *context,
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


dabc::VerbsCQ::~VerbsCQ()
{
   if (fCQContext.events_get > 0) {
      ibv_ack_cq_events(f_cq, fCQContext.events_get);
      fCQContext.events_get = 0;
   }
    
   if (ibv_destroy_cq(f_cq))
      EOUT(("Fail to destroy CQ"));
}

