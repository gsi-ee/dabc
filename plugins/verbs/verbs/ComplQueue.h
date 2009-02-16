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
#ifndef VERBS_ComplQueue
#define VERBS_ComplQueue

#include <infiniband/verbs.h>

namespace verbs {

   class ComplQueue;

   struct ComplQueueContext {
      int    events_get;
      ComplQueue*   itself;
      struct ibv_cq* own_cq;
   };

   class ComplQueue  {
      public:
         ComplQueue(struct ibv_context *context,
                 int size,
                 struct ibv_comp_channel *channel);

         virtual ~ComplQueue();

         struct ibv_comp_channel *channel() const { return f_channel; }
         struct ibv_cq *cq() const { return f_cq; }

      protected:

         struct ibv_cq           *f_cq;
         struct ibv_comp_channel *f_channel;

         ComplQueueContext           fCQContext;
   };
}

#endif
