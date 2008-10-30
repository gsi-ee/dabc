#ifndef DABC_VerbsMemoryPool
#define DABC_VerbsMemoryPool

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#include <infiniband/verbs.h>

namespace dabc {
    
   class VerbsDevice; 
   class VerbsPoolRegistry;
   
   class VerbsMemoryPool : public MemoryPool {
      protected:
         bool fUD;
         unsigned fSendBufferOffset;
         VerbsPoolRegistry *fReg;
         struct ibv_recv_wr* f_rwr; // field for recieve configs, allocated dynamically
         struct ibv_send_wr* f_swr; // field for send configs, allocated dynamically
         struct ibv_sge* f_sge;  // memory segement description, used for both send/recv
         
      public:  
         VerbsMemoryPool(VerbsDevice* verbs,
                         const char* name,
                         int32_t number, 
                         int64_t bufsize, 
                         bool isud,
                         bool without_wr = false);
                         
         virtual ~VerbsMemoryPool();
         
         uint32_t GetLkey(BufferId_t id);
         
         /*
         struct ibv_recv_wr* GetRecvWR(Buffer* buf, uint64_t operid);
         struct ibv_send_wr* GetSendWR(Buffer* buf, uint64_t operid);
         */

         struct ibv_recv_wr* GetRecvWR(BufferId_t id);
         struct ibv_send_wr* GetSendWR(BufferId_t id, uint64_t size);
         
         void* GetSendBufferLocation(BufferId_t id);
   };
}


#endif
