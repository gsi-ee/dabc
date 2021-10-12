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

#ifndef VERBS_Transport
#define VERBS_Transport

#ifndef DABC_NetworkTransport
#include "dabc/NetworkTransport.h"
#endif

#ifndef VERBS_Worker
#include "verbs/Worker.h"
#endif

#ifndef VERBS_Context
#include "verbs/Context.h"
#endif

namespace verbs {

   class Device;
   class PoolRegistry;

   /** \brief Implementation of NetworkTransport for VERBS */


   class VerbsNetworkInetrface : public WorkerAddon,
                                 public dabc::NetworkInetrface {
      protected:
         friend class Device;

         enum Events {
            evntVerbsSendRec = evntVerbsLast,
            evntVerbsRecvRec,
            evntVerbsPool
         };


         verbs::ContextRef   fContext;

         bool                 fInitOk;
         PoolRegistryRef      fPoolReg;
         struct ibv_recv_wr  *f_rwr; // field for receive config, allocated dynamically
         struct ibv_send_wr  *f_swr; // field for send config, allocated dynamically
         struct ibv_sge      *f_sge; // memory segment description, used for both send/recv
         MemoryPool          *fHeadersPool; // polls with headers
         unsigned             fSegmPerOper;
         struct ibv_ah       *f_ud_ah;
         uint32_t             f_ud_qpn;
         uint32_t             f_ud_qkey;
         int                  f_multi;     // return value by ManageMulticast
         ibv_gid              f_multi_gid;
         uint16_t             f_multi_lid;
         bool                 f_multi_attch; // true if QP was attached to multicast group


         virtual long Notify(const std::string&, int);

         virtual void VerbsProcessSendCompl(uint32_t);
         virtual void VerbsProcessRecvCompl(uint32_t);
         virtual void VerbsProcessOperError(uint32_t);

      public:
         VerbsNetworkInetrface(verbs::ContextRef ctx, QueuePair* qp);
         virtual ~VerbsNetworkInetrface();

         bool AssignMultiGid(ibv_gid* multi_gid);

         bool IsInitOk() const { return fInitOk; }
         bool IsUD() const { return f_ud_ah!=0; }

         void SetUdAddr(struct ibv_ah *ud_ah, uint32_t ud_qpn, uint32_t ud_qkey);


         virtual void AllocateNet(unsigned fulloutputqueue, unsigned fullinputqueue);
         virtual void SubmitSend(uint32_t recid);
         virtual void SubmitRecv(uint32_t recid);

   };


}

#endif
