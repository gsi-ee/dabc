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
