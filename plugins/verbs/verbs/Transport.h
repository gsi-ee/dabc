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

struct ibv_mr;
struct ibv_recv_wr;
struct ibv_send_wr;
struct ibv_sge;

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
         Device              *fVerbs;
         ComplQueue          *fCQ;
         PoolRegistry        *fPoolReg;
         struct ibv_recv_wr  *f_rwr; // field for recieve configs, allocated dynamically
         struct ibv_send_wr  *f_swr; // field for send configs, allocated dynamically
         struct ibv_sge      *f_sge; // memory segement description, used for both send/recv
         MemoryPool          *fHeadersPool; // polls with headers
         unsigned             fSegmPerOper;
         bool                 fFastPost;

         virtual void _SubmitSend(uint32_t recid);
         virtual void _SubmitRecv(uint32_t recid);

         virtual bool ProcessPoolRequest();

         virtual void ProcessEvent(dabc::EventId evnt);

      public:
         Transport(Device* dev, ComplQueue* cq, QueuePair* qp, dabc::Port* port, bool useackn);
         virtual ~Transport();

         virtual unsigned MaxSendSegments() { return fSegmPerOper - 1; }

         virtual void VerbsProcessSendCompl(uint32_t);
         virtual void VerbsProcessRecvCompl(uint32_t);
         virtual void VerbsProcessOperError(uint32_t);
   };
}

#endif
