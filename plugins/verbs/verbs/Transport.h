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
#ifndef VERBS_Transport
#define VERBS_Transport

#ifndef DABC_NetworkTransport
#include "dabc/NetworkTransport.h"
#endif

#ifndef VERBS_Processor
#include "verbs/Processor.h"
#endif

#ifndef VERBS_Device
#include "verbs/Device.h"
#endif

namespace verbs {

   class Device;
   class PoolRegistry;

   class Transport : public dabc::NetworkTransport,
                     public Processor {

      friend class Device;

      enum Events {
         evntVerbsSendRec = evntVerbsLast,
         evntVerbsRecvRec,
         evntVerbsPool
      };

      protected:
         ComplQueue          *fCQ;
         bool                 fInitOk;
         PoolRegistry        *fPoolReg;
         struct ibv_recv_wr  *f_rwr; // field for receive config, allocated dynamically
         struct ibv_send_wr  *f_swr; // field for send config, allocated dynamically
         struct ibv_sge      *f_sge; // memory segment description, used for both send/recv
         MemoryPool          *fHeadersPool; // polls with headers
         unsigned             fSegmPerOper;
         bool                 fFastPost;
         struct ibv_ah       *f_ud_ah;
         uint32_t             f_ud_qpn;
         uint32_t             f_ud_qkey;
         bool                 f_multi;     // is transport qp connected to multicast group
         ibv_gid              f_multi_gid;
         uint16_t             f_multi_lid;

         virtual void _SubmitSend(uint32_t recid);
         virtual void _SubmitRecv(uint32_t recid);

         virtual bool ProcessPoolRequest();

         virtual void ProcessEvent(dabc::EventId evnt);

      public:
         Transport(Device* dev, ComplQueue* cq, QueuePair* qp,
                   dabc::Port* port, bool useackn, ibv_gid* multi_gid = 0);
         virtual ~Transport();

         bool IsInitOk() const { return fInitOk; }

         void SetUdAddr(struct ibv_ah *ud_ah, uint32_t ud_qpn, uint32_t ud_qkey);

         virtual unsigned MaxSendSegments() { return fSegmPerOper - 1; }

         virtual void VerbsProcessSendCompl(uint32_t);
         virtual void VerbsProcessRecvCompl(uint32_t);
         virtual void VerbsProcessOperError(uint32_t);
   };
}

#endif
