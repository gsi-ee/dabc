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
