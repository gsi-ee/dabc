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
#ifndef VERBS_QueuePair
#define VERBS_QueuePair

#include <infiniband/verbs.h>

#define VERBS_MAX_INLINE  256
#define VERBS_DEFAULT_QKEY 0x01234567
#define VERBS_MCAST_QPN  0xffffff
#define VERBS_UD_MEMADDON  40


#define VERBSQP_RC 0
#define VERBSQP_UC 1
#define VERBSQP_UD 2

namespace verbs {

   class Device;
   class ComplQueue;

   class QueuePair {
      public:
         QueuePair(Device* verbs, ibv_qp_type qp_type,
                 ComplQueue* send_cq, int send_depth, int max_send_sge,
                 ComplQueue* recv_cq, int recv_depth, int max_recv_sge);
         virtual ~QueuePair();

         struct ibv_qp *qp() const { return f_qp; }

         ibv_qp_type qp_type() const { return fType; }
         bool is_ud() const { return qp_type()==IBV_QPT_UD; }
         uint32_t qp_num() const { return f_qp ? f_qp->qp_num : 0; }
         uint32_t local_psn() const { return f_local_psn; }

         unsigned NumSendSegs() const { return fNumSendSegs; }

         bool Connect(uint16_t lid, uint32_t qpn, uint32_t psn);
         bool InitUD();

         uint16_t remote_lid() const { return f_remote_lid; }
         uint32_t remote_qpn() const { return f_remote_qpn; }
         uint32_t remote_psn() const { return f_remote_psn; }

         bool Post_Send(struct ibv_send_wr* swr);
         bool Post_Recv(struct ibv_recv_wr* rwr);

         bool AttachMcast(ibv_gid* mgid, uint16_t mlid);
         bool DetachMcast(ibv_gid* mgid, uint16_t mlid);

      protected:
         static uint32_t fQPCounter;

         Device* fVerbs;

         ibv_qp_type fType;

         struct ibv_qp *f_qp;

         uint32_t f_local_psn;  // number used in connection

         uint16_t f_remote_lid;
         uint32_t f_remote_qpn;
         uint32_t f_remote_psn;

         unsigned fNumSendSegs;
   };
}

#endif
