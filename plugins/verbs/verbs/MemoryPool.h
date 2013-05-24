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

#ifndef VERBS_MemoryPool
#define VERBS_MemoryPool

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#ifndef VERBS_Context
#include "verbs/Context.h"
#endif

namespace verbs {

   class PoolRegistry;

   /** \brief Special memory pool, which automatically includes PoolRegistry */

   class MemoryPool : public dabc::MemoryPool {
      protected:
         bool                fUD;
         unsigned            fSendBufferOffset;
         PoolRegistryRef     fReg;
         struct ibv_recv_wr* f_rwr; // field for receive configs, allocated dynamically
         struct ibv_send_wr* f_swr; // field for send configs, allocated dynamically
         struct ibv_sge*     f_sge;  // memory segment description, used for both send/recv

      public:
    	  MemoryPool(ContextRef ctx,
                     const char* name,
                     int32_t number,
                     int64_t bufsize,
                     bool isud,
                     bool without_wr = false);

         virtual ~MemoryPool();

         uint32_t GetLkey(unsigned id)
         {
            return fReg() ? fReg()->GetLkey(id) : 0;
         }

         struct ibv_recv_wr* GetRecvWR(unsigned id);
         struct ibv_send_wr* GetSendWR(unsigned id, uint64_t size);

         void* GetSendBufferLocation(unsigned id);

         bool TakeRawBuffer(unsigned& indx) { return dabc::MemoryPool::TakeRawBuffer(indx); }

         /** Release raw buffer, allocated before by TakeRawBuffer */
         void ReleaseRawBuffer(unsigned indx) { dabc::MemoryPool::ReleaseRawBuffer(indx); }

   };
}

#endif
