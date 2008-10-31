#ifndef VERBS_MemoryPool
#define VERBS_MemoryPool

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#include <infiniband/verbs.h>

namespace verbs {

   class Device;
   class PoolRegistry;

   class MemoryPool : public dabc::MemoryPool {
      protected:
         bool fUD;
         unsigned fSendBufferOffset;
         PoolRegistry *fReg;
         struct ibv_recv_wr* f_rwr; // field for recieve configs, allocated dynamically
         struct ibv_send_wr* f_swr; // field for send configs, allocated dynamically
         struct ibv_sge* f_sge;  // memory segement description, used for both send/recv

      public:
    	  MemoryPool(Device* verbs,
                          const char* name,
                          int32_t number,
                          int64_t bufsize,
                          bool isud,
                          bool without_wr = false);

         virtual ~MemoryPool();

         uint32_t GetLkey(dabc::BufferId_t id);

         struct ibv_recv_wr* GetRecvWR(dabc::BufferId_t id);
         struct ibv_send_wr* GetSendWR(dabc::BufferId_t id, uint64_t size);

         void* GetSendBufferLocation(dabc::BufferId_t id);
   };
}

#endif
