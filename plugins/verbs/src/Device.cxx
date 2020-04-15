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

#include "verbs/Device.h"

#include <sys/poll.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>


const char* verbs::xmlMcastAddr = "McastAddr";

#ifndef  __NO_MULTICAST__
#include "verbs/OpenSM.h"
#else
#include <infiniband/arch.h>
#endif

#include "dabc/timing.h"
#include "dabc/logging.h"

#include "dabc/MemoryPool.h"
#include "dabc/Manager.h"
#include "dabc/ConnectionRequest.h"
#include "dabc/ConnectionManager.h"

#include "verbs/ComplQueue.h"
#include "verbs/QueuePair.h"
#include "verbs/Thread.h"
#include "verbs/Transport.h"
#include "verbs/MemoryPool.h"

const int LoopBackQueueSize = 8;
const int LoopBackBufferSize = 64;

// this boolean indicates if one can use verbs calls from different threads
// if no, all post/recv/completion operation for all QP/CQ will happens in the same thread

bool verbs::Device::fThreadSafeVerbs = true;


namespace verbs {

   /** \brief %Addon to establish and verify QP connection with remote node */

   class ProtocolAddon : public WorkerAddon {
      public:
         std::string      fReqItem;

         dabc::Command    fLocalCmd; ///< command which should be replied when connection established or failed

         ContextRef       fIbContext;

         dabc::ThreadRef  fPortThrd;

         // address data of remote side
         unsigned        fRemoteLID;
         unsigned        fRemoteQPN;
         unsigned        fRemotePSN;

         MemoryPool      *fPool;

         bool             fConnected;

      public:
         ProtocolAddon(QueuePair* qp);
         virtual ~ProtocolAddon();

         virtual void VerbsProcessSendCompl(uint32_t);
         virtual void VerbsProcessRecvCompl(uint32_t);
         virtual void VerbsProcessOperError(uint32_t);

         virtual void OnThreadAssigned();
         virtual double ProcessTimeout(double last_diff);

         void Finish(bool res);
   };

}


verbs::ProtocolAddon::ProtocolAddon(QueuePair* qp) :
   WorkerAddon(qp),
   fReqItem(),
   fLocalCmd(),
   fPortThrd(),
   fRemoteLID(0),
   fRemoteQPN(0),
   fRemotePSN(0),
   fPool(0),
   fConnected(false)
{
}

verbs::ProtocolAddon::~ProtocolAddon()
{
   fReqItem.clear();

   if (fPool) delete fPool; fPool = 0;

   CloseQP();
}

void verbs::ProtocolAddon::OnThreadAssigned()
{
   dabc::WorkerAddon::OnThreadAssigned();

   dabc::ConnectionRequestFull req = dabc::mgr.FindPar(fReqItem);
   if (req.null()) {
      Finish(false);
      return;
   }

   unsigned bufid = 0;

   if (req.IsServerSide()) {
      QP()->Post_Recv(fPool->GetRecvWR(bufid));
   } else {
      std::string connid = req.GetConnId();

      strcpy((char*) fPool->GetSendBufferLocation(bufid), connid.c_str());
      QP()->Post_Send(fPool->GetSendWR(bufid, connid.length()+1));
   }

   ActivateTimeout(req.GetConnTimeout());
}

void verbs::ProtocolAddon::Finish(bool res)
{
   fConnected = res;

   fReqItem.clear();

   fLocalCmd.ReplyBool(res);

   fIbContext.Release();

   DeleteWorker();
}



double verbs::ProtocolAddon::ProcessTimeout(double last_diff)
{
   if (fConnected) return -1;

   EOUT("HANDSHAKE is timedout");

   Finish(false);

   return -1;
}

void verbs::ProtocolAddon::VerbsProcessSendCompl(uint32_t bufid)
{
   if (bufid!=0) {
      EOUT("Wrong buffer id %u", bufid);
      return;
   }

   dabc::ConnectionRequestFull req = dabc::mgr.FindPar(fReqItem);
   if (req.null()) {
      EOUT("Connection request disappear");
      Finish(true);
      return;
   }

   const char* connid = (const char*) fPool->GetSendBufferLocation(bufid);

   if (req.GetConnId().compare(connid)!=0) {
      EOUT("AAAAA !!!!! Mismatch with connid %s %s", connid, req.GetConnId().c_str());
   }

   // Here we are sure, that other side receive handshake packet,
   // therefore we can declare connection as done


   VerbsNetworkInetrface* addon = new VerbsNetworkInetrface(fIbContext, TakeQP());

   dabc::NetworkTransport::Make(req, addon, fPortThrd.GetName());

   DOUT0("CREATE VERBS CLIENT %s",  req.GetConnId().c_str());

   Finish(true);
}

void verbs::ProtocolAddon::VerbsProcessRecvCompl(uint32_t bufid)
{
   if (bufid!=0) {
      EOUT("Wrong buffer id %u", bufid);
      return;
   }

   dabc::ConnectionRequestFull req = dabc::mgr.FindPar(fReqItem);

   const char* connid = (const char*) fPool->GetBufferLocation(bufid);

   if (req.GetConnId().compare(connid)!=0) {
      EOUT("AAAAA !!!!! Mismatch with connid %s %s", connid, req.GetConnId().c_str());
   }

   // from here we knew, that client is ready and we also can complete connection

   VerbsNetworkInetrface* addon = new VerbsNetworkInetrface(fIbContext, TakeQP());

   dabc::NetworkTransport::Make(req, addon, fPortThrd.GetName());

   DOUT0("CREATE VERBS SERVER %s",  req.GetConnId().c_str());

   Finish(true);
}

void verbs::ProtocolAddon::VerbsProcessOperError(uint32_t bufid)
{
   EOUT("VerbsProtocolAddon error");

   Finish(false);
}

// ____________________________________________________________________

verbs::Device::Device(const std::string &name) :
   dabc::Device(name),
   fIbContext(),
   fAllocateIndividualCQ(false)
{
   DOUT1("Creating VERBS device %s  refcnt %u", GetName(), NumReferences());

   if (!fIbContext.OpenVerbs(true)) {
      EOUT("FATAL. Cannot start VERBS device");
      exit(139);
   }

   DOUT1("Creating thread for device %s", GetName());

   MakeThreadForWorker(GetName());

   DOUT1("Creating VERBS device %s done refcnt %u", GetName(), NumReferences());
}

verbs::Device::~Device()
{
   DOUT5("verbs::Device::~Device()");
}

verbs::QueuePair* verbs::Device::CreatePortQP(const std::string &thrd_name, dabc::Reference port, int conn_type, dabc::ThreadRef& thrd)
{
   ibv_qp_type qp_type = IBV_QPT_RC;

   if (conn_type>0) qp_type = (ibv_qp_type) conn_type;

   thrd = MakeThread(thrd_name.c_str(), true);

   verbs::Thread* thrd_ptr = dynamic_cast<verbs::Thread*> (thrd());

   if (thrd_ptr == 0) return 0;

   unsigned input_size(0), output_size(0);

   dabc::NetworkTransport::GetRequiredQueuesSizes(port, input_size, output_size);

   ComplQueue* port_cq = thrd_ptr->MakeCQ();

   int num_send_seg = fIbContext.max_sge() - 1;
   if (conn_type==IBV_QPT_UD) num_send_seg = fIbContext.max_sge() - 5; // I do not now why, but otherwise it fails
   if (num_send_seg<2) num_send_seg = 2;

   verbs::QueuePair* port_qp = new QueuePair(IbContext(), qp_type,
                                   port_cq, output_size + 1, num_send_seg,
                                   port_cq, input_size + 1, 2);

   if (port_qp->qp()==0) {
      delete port_qp;
      port_qp = 0;
   }

   return port_qp;
}

dabc::ThreadRef verbs::Device::MakeThread(const char* name, bool force)
{
   ThreadRef thrd = dabc::mgr.FindThread(name, verbs::typeThread);

   if (!thrd.null() || !force) return thrd;

   return dabc::mgr.CreateThread(name, verbs::typeThread, GetName());
}

dabc::Transport* verbs::Device::CreateTransport(dabc::Command cmd, const dabc::Reference& port)
{
   // TODO: implement multicast transport for IB

   dabc::PortRef portref = port;

   std::string maddr = portref.Cfg(xmlMcastAddr, cmd).AsStr();

   if (!maddr.empty()) {
      std::string thrdname = portref.Cfg(dabc::xmlThreadAttr,cmd).AsStr();

      if (thrdname.empty())
         switch (dabc::mgr.GetThreadsLayout()) {
            case dabc::layoutMinimalistic: thrdname = ThreadName(); break;
            case dabc::layoutPerModule: thrdname = portref.GetModule().ThreadName(); break;
            case dabc::layoutBalanced: thrdname = portref.GetModule().ThreadName() + (portref.IsInput() ? "Inp" : "Out"); break;
            case dabc::layoutMaximal: thrdname = portref.GetModule().ThreadName() + portref.GetName(); break;
            default: thrdname = portref.GetModule().ThreadName(); break;
         }

      ibv_gid multi_gid;

      if (!ConvertStrToGid(maddr, multi_gid)) {
         EOUT("Cannot convert address %s to ibv_gid type", maddr.c_str());
         return 0;
      }

      std::string buf = ConvertGidToStr(multi_gid);
      if (buf!=maddr) {
         EOUT("Addresses not the same: %s - %s", maddr.c_str(), buf.c_str());
         return 0;
      }



//      QueuePair*  port_qp(0);
//      ThreadRef   thrd;
//      if (CreatePortQP(thrdname.c_str(), port, IBV_QPT_UD, thrd, port_qp))
//         return new Transport(fIbContext, port_cq, port_qp, portref, false, &multi_gid);
   }

   return 0;
}

int verbs::Device::HandleManagerConnectionRequest(dabc::Command cmd)
{
   std::string reqitem = cmd.GetStr(dabc::CmdConnectionManagerHandle::ReqArg());

   dabc::ConnectionRequestFull req = dabc::mgr.FindPar(reqitem);

   if (req.null()) return dabc::cmd_false;

   switch (req.progress()) {

      // here on initializes connection
      case dabc::ConnectionManager::progrDoingInit: {

         dabc::PortRef port = req.GetPort();
         if (port.null()) {
            EOUT("No port is available for the request");
            return dabc::cmd_false;
         }

         dabc::ThreadRef thrd;

         // FIXME: ConnectionRequest should be used
         QueuePair* port_qp = CreatePortQP(req.GetConnThread(), port, 0, thrd);
         if (port_qp==0) return dabc::cmd_false;

         std::string portid = dabc::format("%04X:%08X:%08X", (unsigned) fIbContext.lid(), (unsigned) port_qp->qp_num(), (unsigned) port_qp->local_psn());
         DOUT0("CREATE CONNECTION %s", portid.c_str());

         ProtocolAddon* addon = new ProtocolAddon(port_qp);
         addon->fPortThrd << thrd;

         if (req.IsServerSide())
            req.SetServerId(portid);
         else
            req.SetClientId(portid);

         // make backpointers, fCustomData is reference, automatically cleaned up by the connection manager
         req.SetCustomData(dabc::Reference(addon));

         addon->fReqItem = reqitem;

         addon->fIbContext = fIbContext;

         return dabc::cmd_true;
      }

      case dabc::ConnectionManager::progrDoingConnect: {
         // one should register request and start connection here

         DOUT2("****** VERBS START: %s %s CONN: %s *******", (req.IsServerSide() ? "SERVER" : "CLIENT"), req.GetConnId().c_str(), req.GetConnInfo().c_str());

         // once connection is started, custom data is no longer necessary by connection record
         // protocol worker will be cleaned up automatically either when connection is done or when connection is timedout

         // by coping of the reference source reference will be cleaned
         // we use reference on addon while it will remain even if worker will be destroyed
         dabc::Reference prot_ref = req.TakeCustomData();

         ProtocolAddon* proto = dynamic_cast<ProtocolAddon*> (prot_ref());

         if (proto == 0) {
            EOUT("SOMETHING WRONG - NO PROTOCOL addon for the connection request");
            return dabc::cmd_false;
         }

         std::string remoteid;
         bool res = true;

         if (req.IsServerSide())
            remoteid = req.GetClientId();
         else
            remoteid = req.GetServerId();

         if (sscanf(remoteid.c_str(),"%X:%X:%X", &proto->fRemoteLID, &proto->fRemoteQPN, &proto->fRemotePSN)!=3) {
            EOUT("Cannot decode remote id string %s", remoteid.c_str());
            res = false;
         }

         // reply remote command that one other side can start connection
         req.ReplyRemoteCommand(res);

         if (!res) {
            prot_ref.Release();
            return dabc::cmd_false;
         }

         DOUT0("CONNECT TO REMOTE %04x:%08x:%08x - %s", proto->fRemoteLID, proto->fRemoteQPN, proto->fRemotePSN, remoteid.c_str());

         // FIXME: remote port should be handled correctly
         if (proto->QP()->Connect(proto->fRemoteLID, proto->fRemoteQPN, proto->fRemotePSN)) {

            proto->fPool = new verbs::MemoryPool(fIbContext, "HandshakePool", 1, 1024, false);

            proto->fLocalCmd = cmd;

            // we need to preserve thread reference until transport itself will be created
            proto->fPortThrd.MakeWorkerFor(proto);

            return dabc::cmd_postponed;
         }

         return dabc::cmd_false;
      }


      default:
         EOUT("Request from connection manager in undefined situation progress = %d ???", req.progress());
         return dabc::cmd_false;
         break;
   }

   return dabc::cmd_true;
}


int verbs::Device::ExecuteCommand(dabc::Command cmd)
{
   int cmd_res = dabc::cmd_true;

   DOUT5("Execute command %s", cmd.GetName());


   if (cmd.IsName(dabc::CmdConnectionManagerHandle::CmdName())) {

      cmd_res = HandleManagerConnectionRequest(cmd);

   } else

     cmd_res = dabc::Device::ExecuteCommand(cmd);

   return cmd_res;
}

double verbs::Device::ProcessTimeout(double last_diff)
{
   return -1;
}

bool verbs::ConvertStrToGid(const std::string &s, ibv_gid &gid)
{
   unsigned raw[16];

   if (sscanf(s.c_str(),
                 "%2X:%2X:%2X:%2X:%2X:%2X:%2X:%2X:%2X:%2X:%2X:%2X:%2X:%2X:%2X:%2X",
                 raw, raw+1, raw+2, raw+3, raw+4, raw+5, raw+6, raw+7,
                 raw+8, raw+9, raw+10, raw+11, raw+12, raw+13, raw+14, raw+15) != 16) return false;
   for (unsigned n=0;n<16;n++)
      gid.raw[n] = raw[n];
   return true;
}

std::string verbs::ConvertGidToStr(ibv_gid &gid)
{
   unsigned raw[16];
   for (unsigned n=0;n<16;n++)
      raw[n] = gid.raw[n];

   std::string res;

   dabc::formats(res, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                       raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], raw[6], raw[7],
                       raw[8], raw[9], raw[10], raw[11], raw[12], raw[13], raw[14], raw[15]);
  return res;
}
