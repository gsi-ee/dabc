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

#include "verbs/QueuePair.h"

#include <netdb.h>
#include <cstdlib>
#include <cstring>

#include "dabc/logging.h"

#include <infiniband/arch.h>

#include "verbs/Device.h"
#include "verbs/ComplQueue.h"



uint32_t verbs::QueuePair::fQPCounter = 0;

// static int __qpcnt__ = 0;

verbs::QueuePair::QueuePair(ContextRef ctx, ibv_qp_type qp_type,
                                 ComplQueue* send_cq, int send_depth, int max_send_sge,
                                 ComplQueue* recv_cq, int recv_depth, int max_recv_sge) :
   fContext(ctx),
   fType(qp_type),
   f_qp(0),
   f_local_psn(0),
   f_remote_lid(0),
   f_remote_qpn(0),
   f_remote_psn(0),
   fNumSendSegs(max_send_sge)
{
   if ((send_cq==0) || (recv_cq==0)) {
      EOUT("No COMPLETION QUEUE WAS SPECIFIED");
      return;
   }

   struct ibv_qp_init_attr attr;
   memset(&attr, 0, sizeof(struct ibv_qp_init_attr));
   attr.send_cq = send_cq->cq();
   attr.recv_cq = recv_cq->cq();
//   attr.sq_sig_all = 1; // if signal all operation, set 1 ????

   attr.cap.max_send_wr  = send_depth;
   attr.cap.max_send_sge = max_send_sge;

   attr.cap.max_recv_wr  = recv_depth;
   attr.cap.max_recv_sge = max_recv_sge;

   attr.cap.max_inline_data = VERBS_MAX_INLINE;
   attr.qp_type = fType;

   f_qp = ibv_create_qp(fContext.pd(), &attr);
   if (f_qp==0) {
      EOUT("Couldn't create queue pair (QP)");
      return;
   }

   struct ibv_qp_attr qp_attr;
   qp_attr.qp_state        = IBV_QPS_INIT;
   qp_attr.pkey_index      = 0;
   qp_attr.port_num        = fContext.IbPort();
   int res = 0;

   if (qp_type == IBV_QPT_UD) {
      qp_attr.qkey         = VERBS_DEFAULT_QKEY;
      res = ibv_modify_qp(f_qp, &qp_attr, (ibv_qp_attr_mask)
                          (IBV_QP_STATE              |
                          IBV_QP_PKEY_INDEX         |
                          IBV_QP_PORT               |
                          IBV_QP_QKEY));
   } else {
//      qp_attr.qp_access_flags = 0; when no RDMA is required

      qp_attr.qp_access_flags =
         IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE |
                                   IBV_ACCESS_REMOTE_READ;
      res = ibv_modify_qp(f_qp, &qp_attr, (ibv_qp_attr_mask)
                          (IBV_QP_STATE              |
                          IBV_QP_PKEY_INDEX         |
                          IBV_QP_PORT               |
                          IBV_QP_ACCESS_FLAGS));
   }

   if (res!=0) {
      EOUT("Failed to modify QP to INIT state");
      return;
   }

   fQPCounter += 1;
   f_local_psn = fQPCounter;

   DOUT4("Create QueuePair %p", this);
}

verbs::QueuePair::~QueuePair()
{
   DOUT4("Destroy QueuePair %p", this);

   ibv_destroy_qp(f_qp);

   DOUT4("Destroy QueuePair %p done", this);
}

bool verbs::QueuePair::InitUD()
{
   struct ibv_qp_attr attr;
   memset(&attr, 0, sizeof attr);

   attr.qp_state       = IBV_QPS_RTR;
   attr.path_mtu       = fContext.mtu();

   if (ibv_modify_qp(qp(), &attr, IBV_QP_STATE )) {
       EOUT("Failed to modify UD QP to RTR");
      return false;
   }

   attr.qp_state   = IBV_QPS_RTS;
   attr.sq_psn     = local_psn();
   if (ibv_modify_qp(qp(), &attr, (ibv_qp_attr_mask)
                     (IBV_QP_STATE | IBV_QP_SQ_PSN))) {
      EOUT("Failed to modify UC/UD QP to RTS");
      return false;
   }

   f_remote_lid = 0;
   f_remote_qpn = 0;
   f_remote_psn = 0;

   return true;
}

bool verbs::QueuePair::Connect(uint16_t dest_lid, uint32_t dest_qpn, uint32_t dest_psn, uint8_t src_path_bits)
{
   if (qp_type() == IBV_QPT_UD) {
      EOUT("QueuePair::Connect not supported for unreliable datagram connection. Use InitUD() instead");
      return false;
   }

   DOUT3("Start QP connect with %x:%x:%x", dest_lid, dest_qpn, dest_psn);

   struct ibv_qp_attr attr;
   memset(&attr, 0, sizeof attr);

   attr.qp_state   = IBV_QPS_RTR;
   attr.path_mtu   = fContext.mtu();

/*
   switch (f_mtu) {
      case 256: attr.path_mtu = IBV_MTU_256; break;
      case 512: attr.path_mtu = IBV_MTU_512; break;
      case 1024: attr.path_mtu = IBV_MTU_1024; break;
      case 2048: attr.path_mtu = IBV_MTU_2048; break;
      case 4096: attr.path_mtu = IBV_MTU_4096; break;
      default: EOUT("Wrong mtu value %u", f_mtu);
   }
*/

//   FIXME: why mtu was fixed to 1024
//   attr.path_mtu = IBV_MTU_1024;

   attr.dest_qp_num  = dest_qpn;
   attr.rq_psn       = dest_psn;
   if (qp_type() == IBV_QPT_RC) {
      attr.max_dest_rd_atomic     = 1;
      attr.min_rnr_timer          = 12;
   }

   attr.ah_attr.is_global  = 0;
   attr.ah_attr.dlid       = dest_lid;
   attr.ah_attr.sl         = 0;
   // DOUT0("Configure static rate");
   // attr.ah_attr.static_rate = 3 /*IBV_RATE_5_GBPS*/; // SL: no idea how static rate works
   attr.ah_attr.src_path_bits = src_path_bits;
   attr.ah_attr.port_num   = fContext.IbPort();

   DOUT3("Modify to RTR");

   if (qp_type() == IBV_QPT_RC) {
      if (ibv_modify_qp(qp(), &attr, (ibv_qp_attr_mask)
             (IBV_QP_STATE              |
              IBV_QP_AV                 |
              IBV_QP_PATH_MTU           |
              IBV_QP_DEST_QPN           |
              IBV_QP_RQ_PSN             |
              IBV_QP_MIN_RNR_TIMER      |
              IBV_QP_MAX_DEST_RD_ATOMIC))) {
         EOUT("Failed here to modify RC QP to RTR lid: %x, qpn: %x, psn:%x", dest_lid, dest_qpn, dest_psn);
         return false;
      }
   } else

   if (qp_type() == IBV_QPT_UC) {
      if (ibv_modify_qp(qp(), &attr, (ibv_qp_attr_mask)
             (IBV_QP_STATE              |
              IBV_QP_AV                 |
              IBV_QP_PATH_MTU           |
              IBV_QP_DEST_QPN           |
              IBV_QP_RQ_PSN))) {
         EOUT("Failed to modify UC QP to RTR");
         return false;
      }
   } else

   if (qp_type() == IBV_QPT_UD) {
      if (ibv_modify_qp(qp(), &attr,
              IBV_QP_STATE )) {
         EOUT("Failed to modify UD QP to RTR");
         return false;
      }
   }

   DOUT3("Modify to RTS");

   attr.qp_state   = IBV_QPS_RTS;
   attr.sq_psn     = local_psn();
   if (qp_type() == IBV_QPT_RC) {
      attr.timeout       = 14;
      attr.retry_cnt     = 7;
      attr.rnr_retry     = 7;
      attr.max_rd_atomic = 1;
      if (ibv_modify_qp(qp(), &attr, (ibv_qp_attr_mask)
             (IBV_QP_STATE              |
              IBV_QP_SQ_PSN             |
              IBV_QP_TIMEOUT            |
              IBV_QP_RETRY_CNT          |
              IBV_QP_RNR_RETRY          |
              IBV_QP_MAX_QP_RD_ATOMIC))) {
         EOUT("Failed to modify RC QP to RTS");
         return false;
      }
   } else { /*both UC and UD */
      if (ibv_modify_qp(qp(), &attr, (ibv_qp_attr_mask)
             (IBV_QP_STATE              |
              IBV_QP_SQ_PSN))) {
         EOUT("Failed to modify UC/UD QP to RTS");
         return false;
      }
   }

   f_remote_lid = dest_lid;
   f_remote_qpn = dest_qpn;
   f_remote_psn = dest_psn;

   DOUT3("QP connected !!!");

//   if (qp_type()!= IBV_QPT_UD)
//      DOUT1("DO CONNECT local_qpn=%x remote_qpn=%x", qp_num(), dest_qpn);

   return true;
}

bool verbs::QueuePair::Post_Send(struct ibv_send_wr* swr)
{
   struct ibv_send_wr* bad_swr = 0;

   if (ibv_post_send(qp(), swr, &bad_swr)) {
      EOUT("ibv_post_send fails arg %lx", bad_swr->wr_id);
      return false;
   }

   return true;
}

bool verbs::QueuePair::Post_Recv(struct ibv_recv_wr* rwr)
{
   struct ibv_recv_wr* bad_rwr = 0;

   if (ibv_post_recv(qp(), rwr, &bad_rwr)) {
      EOUT("ibv_post_recv fails arg = %lx", bad_rwr->wr_id);
      return false;
   }

   return true;
}

bool verbs::QueuePair::AttachMcast(ibv_gid* mgid, uint16_t mlid)
{
   return ibv_attach_mcast(qp(), mgid, mlid) == 0;
}

bool verbs::QueuePair::DetachMcast(ibv_gid* mgid, uint16_t mlid)
{
  return ibv_detach_mcast(qp(), mgid, mlid) == 0;
}
