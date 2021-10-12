// $Id$

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

#include "verbs/MemoryPool.h"

#include "verbs/QueuePair.h"
#include "verbs/Device.h"

verbs::MemoryPool::MemoryPool(ContextRef ctx,
                              const char* name,
                              int32_t number,
                              int64_t bufsize,
                              bool isud,
                              bool without_wr) :
   dabc::MemoryPool(name, false),
   fUD(isud),
   fSendBufferOffset(isud ? VERBS_UD_MEMADDON : 0),
   fReg(),
   f_rwr(0),
   f_swr(0),
   f_sge(0)
{
   DOUT4("Create verbs::Pool %s %p", GetName(), this);

   if (isud && (bufsize>0x1000)) bufsize = 0x1000;

   SetAlignment((bufsize<0x100) ? 0x10 : 0x100);

   Allocate(bufsize, number);

   fReg = new PoolRegistry(ctx, this);
   fReg()->SyncMRStructure();

   if (without_wr) return;

   f_rwr = new ibv_recv_wr [number];
   f_swr = new ibv_send_wr [number];
   f_sge = new ibv_sge [number];

   for (int32_t id = 0; id<number; id++) {
      f_sge[id].addr   = (uintptr_t) 0; // must be specified later
      f_sge[id].length = 0; // must be specified later
      f_sge[id].lkey   = // must be set later;

      f_swr[id].wr_id    = 0; // must be set later
      f_swr[id].sg_list  = &(f_sge[id]);
      f_swr[id].num_sge  = 1;
      f_swr[id].opcode   = IBV_WR_SEND;
      f_swr[id].next     = NULL;
      f_swr[id].send_flags = IBV_SEND_SIGNALED;

      f_rwr[id].wr_id     = 0; // must be set later
      f_rwr[id].sg_list   = &(f_sge[id]);
      f_rwr[id].num_sge   = 1;
      f_rwr[id].next      = NULL;
   }

}

verbs::MemoryPool::~MemoryPool()
{
   DOUT4("Destroy verbs::Pool %s %p refs:%u", GetName(), this, NumReferences());

   fReg.Destroy();

   delete[] f_rwr; f_rwr = 0;
   delete[] f_swr; f_swr = 0;
   delete[] f_sge; f_sge = 0;

   DOUT4("Destroy verbs::Pool %s %p refs:%u DONE", GetName(), this, NumReferences());

}

struct ibv_recv_wr* verbs::MemoryPool::GetRecvWR(unsigned bufid)
{
   // special case, when working without Buffer objects at all

   if ((f_rwr==0) || (f_sge==0)) return 0;

   f_sge[bufid].addr   = (uintptr_t) GetBufferLocation(bufid);
   f_sge[bufid].length = GetBufferSize(bufid);
   f_sge[bufid].lkey   = GetLkey(bufid); // we knew that here we work with 0 block always

   f_rwr[bufid].wr_id     = bufid;
   f_rwr[bufid].sg_list   = &(f_sge[bufid]);
   f_rwr[bufid].num_sge   = 1;
   f_rwr[bufid].next      = NULL;

   return &(f_rwr[bufid]);
}

struct ibv_send_wr* verbs::MemoryPool::GetSendWR(unsigned bufid, uint64_t size)
{
   // special case, when working without Buffer objects at all

   if ((f_swr==0) || (f_sge==0)) return 0;

   if (size==0) size = GetBufferSize(bufid) - fSendBufferOffset;

   f_sge[bufid].addr   = (uintptr_t) GetSendBufferLocation(bufid);
   f_sge[bufid].length = size;
   f_sge[bufid].lkey   = GetLkey(bufid); // here we always works

   f_swr[bufid].wr_id    = bufid;
   f_swr[bufid].sg_list  = &(f_sge[bufid]);
   f_swr[bufid].num_sge  = 1;
   f_swr[bufid].opcode   = IBV_WR_SEND;
   f_swr[bufid].next     = NULL;
   if (size <= VERBS_MAX_INLINE)
      f_swr[bufid].send_flags = (ibv_send_flags) (IBV_SEND_SIGNALED | IBV_SEND_INLINE);
   else
      f_swr[bufid].send_flags = IBV_SEND_SIGNALED;

   return &(f_swr[bufid]);
}

void* verbs::MemoryPool::GetSendBufferLocation(unsigned bufid)
{
    return (char*)GetBufferLocation(bufid) + fSendBufferOffset;
}
