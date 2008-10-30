#ifndef DABC_VerbsCQ
#define DABC_VerbsCQ

#include <infiniband/verbs.h>

namespace dabc {
    
   class VerbsCQ; 
    
   struct VerbsCQContext {
      int    events_get;
      VerbsCQ*   itself;
      struct ibv_cq* own_cq;
   }; 

   class VerbsCQ  {
      public:
         VerbsCQ(struct ibv_context *context,
                 int size,
                 struct ibv_comp_channel *channel);
                  
         virtual ~VerbsCQ();
   
         struct ibv_comp_channel *channel() const { return f_channel; }
         struct ibv_cq *cq() const { return f_cq; }
   
      protected:
   
         struct ibv_cq           *f_cq;
         struct ibv_comp_channel *f_channel;
         
         VerbsCQContext           fCQContext;
   };
}

#endif
