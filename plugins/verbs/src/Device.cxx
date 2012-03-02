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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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
#include "dabc/Port.h"
#include "dabc/Factory.h"
#include "dabc/ConnectionRequest.h"
#include "dabc/ConnectionManager.h"

#include "verbs/ComplQueue.h"
#include "verbs/QueuePair.h"
#include "verbs/Thread.h"
#include "verbs/Transport.h"
#include "verbs/MemoryPool.h"
#include "verbs/BnetRunnable.h"

const int LoopBackQueueSize = 8;
const int LoopBackBufferSize = 64;

// this boolean indicates if one can use verbs calls from different threads
// if no, all post/recv/completion operation for all QP/CQ will happens in the same thread

bool verbs::Device::fThreadSafeVerbs = true;

namespace verbs {
   class VerbsFactory : public dabc::Factory {
      public:
         VerbsFactory(const char* name) : dabc::Factory(name) {}

         virtual dabc::Reference CreateObject(const char* classname, const char* objname, dabc::Command cmd);

         virtual dabc::Device* CreateDevice(const char* classname,
                                            const char* devname,
                                            dabc::Command cmd);

         virtual dabc::Reference CreateThread(dabc::Reference parent, const char* classname, const char* thrdname, const char* thrddev, dabc::Command cmd);
   };


   VerbsFactory verbsfactory("verbs");
}

dabc::Reference verbs::VerbsFactory::CreateObject(const char* classname, const char* objname, dabc::Command cmd)
{
   if (strcmp(classname, "verbs::BnetRunnable")==0)
      return new verbs::BnetRunnable(objname);

   return 0;
}

dabc::Device* verbs::VerbsFactory::CreateDevice(const char* classname,
                                                const char* devname,
                                                dabc::Command cmd)
{
	if (strcmp(classname, "verbs::Device")!=0)
	   return dabc::Factory::CreateDevice(classname, devname, cmd);

	return new Device(devname);
}

dabc::Reference verbs::VerbsFactory::CreateThread(dabc::Reference parent, const char* classname, const char* thrdname, const char* thrddev, dabc::Command cmd)
{
   if ((classname==0) || (strcmp(classname, VERBS_THRD_CLASSNAME)!=0)) return dabc::Reference();

   if (thrddev==0) {
      EOUT(("Device name not specified to create verbs thread"));
      return dabc::Reference();
   }

   verbs::DeviceRef dev = dabc::mgr()->FindDevice(thrddev);

   if (dev.null()) {
      EOUT(("Did not found verbs device with name %s", thrddev));
      return dabc::Reference();
   }

   verbs::Thread* thrd = new verbs::Thread(parent, dev()->IbContext(), thrdname);

   return dabc::Reference(thrd);
}


// ____________________________________________________________________


namespace verbs {

   class ProtocolWorker : public Worker {
      public:
         std::string      fReqItem;

         dabc::Command    fLocalCmd; //!< command which should be replied when connection established or failed

         verbs::DeviceRef fDevice;

         dabc::ThreadRef  fPortThrd;
         QueuePair       *fPortQP;
         ComplQueue      *fPortCQ;

         // address data of remote side
         unsigned        fRemoteLID;
         unsigned        fRemoteQPN;
         unsigned        fRemotePSN;

         MemoryPool      *fPool;

         bool             fConnected;

      public:
         ProtocolWorker();

         /*
         ProtocolWorker(Thread* thrd,
                           QueuePair* hs_qp,
                           bool server,
                           const char* portname,
                           dabc::Command* cmd);
         */

         virtual ~ProtocolWorker();

         virtual void VerbsProcessSendCompl(uint32_t);
         virtual void VerbsProcessRecvCompl(uint32_t);
         virtual void VerbsProcessOperError(uint32_t);

         virtual void OnThreadAssigned();
         virtual double ProcessTimeout(double last_diff);

         void Finish(bool res);
   };

   class ProtocolWorkerRef : public WorkerRef {

      DABC_REFERENCE(ProtocolWorkerRef, WorkerRef, verbs::ProtocolWorker)

   };
}


verbs::ProtocolWorker::ProtocolWorker() :
   Worker(0),
   fReqItem(),
   fLocalCmd(),
   fPortThrd(0),
   fPortQP(0),
   fPortCQ(0),
   fRemoteLID(0),
   fRemoteQPN(0),
   fRemotePSN(0),
   fPool(0),
   fConnected(false)
{
}

verbs::ProtocolWorker::~ProtocolWorker()
{
   fReqItem.clear();

   if (fPool) delete fPool; fPool = 0;

   if (fPortQP!=QP()) delete fPortQP;
   fPortQP = 0;
   if (fPortCQ) delete fPortCQ;
   fPortCQ = 0;

   CloseQP();
}

void verbs::ProtocolWorker::OnThreadAssigned()
{
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

void verbs::ProtocolWorker::Finish(bool res)
{
   fConnected = res;

   fReqItem.clear();

   fLocalCmd.ReplyBool(res);

   fDevice.Release();

   DeleteThis();
}



double verbs::ProtocolWorker::ProcessTimeout(double last_diff)
{
   if (fConnected) return -1;

   EOUT(("HANDSHAKE is timedout"));

   Finish(false);

   return -1;
}

void verbs::ProtocolWorker::VerbsProcessSendCompl(uint32_t bufid)
{
   if (bufid!=0) {
      EOUT(("Wrong buffer id %u", bufid));
      return;
   }

   dabc::ConnectionRequestFull req = dabc::mgr.FindPar(fReqItem);
   if (req.null()) {
      EOUT(("Connection request disappear"));
      Finish(true);
      return;
   }


   const char* connid = (const char*) fPool->GetSendBufferLocation(bufid);

   if (req.GetConnId().compare(connid)!=0) {
      EOUT(("AAAAA !!!!! Mismatch with connid %s %s", connid, req.GetConnId().c_str()));
   }

   // Here we are sure, that other side receive handshake packet,
   // therefore we can declare connection as done


   TakeQP(); // we remove queue pair from the processor

   dabc::PortRef port = req.GetPort();

   if (!fDevice.null() && !port.null())
      fDevice()->CreateVerbsTransport(fPortThrd, port(), req.GetUseAckn(), fPortCQ, fPortQP);
   else {
      EOUT(("Cannot find verbs::Device to create transport"));
   }

   fPortQP = 0;
   fPortCQ = 0;

   DOUT0(("CREATE VERBS CLIENT %s",  req.GetConnId().c_str()));

   Finish(true);
}

void verbs::ProtocolWorker::VerbsProcessRecvCompl(uint32_t bufid)
{
   if (bufid!=0) {
      EOUT(("Wrong buffer id %u", bufid));
      return;
   }

   dabc::ConnectionRequestFull req = dabc::mgr.FindPar(fReqItem);

   const char* connid = (const char*) fPool->GetBufferLocation(bufid);

   if (req.GetConnId().compare(connid)!=0) {
      EOUT(("AAAAA !!!!! Mismatch with connid %s %s", connid, req.GetConnId().c_str()));
   }

   // from here we knew, that client is ready and we also can complete connection

   TakeQP(); // we remove queue pair from the processor

   dabc::PortRef port = req.GetPort();

   if (!fDevice.null() && !port.null())
      fDevice()->CreateVerbsTransport(fPortThrd, port(), req.GetUseAckn(), fPortCQ, fPortQP);
   else {
      EOUT(("Cannot find verbs::Device to create transport"));
   }

   fPortQP = 0;
   fPortCQ = 0;

   DOUT0(("CREATE VERBS SERVER %s",  req.GetConnId().c_str()));

   Finish(true);
}

void verbs::ProtocolWorker::VerbsProcessOperError(uint32_t bufid)
{
   EOUT(("VerbsProtocolWorker error"));

   Finish(false);
}

// ____________________________________________________________________

verbs::Device::Device(const char* name) :
   dabc::Device(name),
   fIbContext(),
   fAllocateIndividualCQ(false)
{
   DOUT2(("Creating VERBS device %s  refcnt %u", name, NumReferences()));

   if (!fIbContext.OpenVerbs(true)) {
      EOUT(("FATAL. Cannot start VERBS device"));
      exit(139);
   }

   DOUT3(("Creating thread for device %s", GetName()));

   dabc::mgr()->MakeThreadFor(this, GetName());

   DOUT2(("Creating VERBS device %s done refcnt %u", name, NumReferences()));
}

verbs::Device::~Device()
{
   DOUT5(("verbs::Device::~Device()"));
}

void verbs::Device::ObjectCleanup()
{
   dabc::Device::ObjectCleanup();
}

bool verbs::Device::CreatePortQP(const char* thrd_name, dabc::Port* port, int conn_type,
                                 dabc::ThreadRef &thrd, ComplQueue* &port_cq, QueuePair* &port_qp)
{
   ibv_qp_type qp_type = IBV_QPT_RC;

   if (conn_type>0) qp_type = (ibv_qp_type) conn_type;

   thrd = MakeThread(thrd_name, true);

   verbs::Thread* thrd_ptr = dynamic_cast<verbs::Thread*> (thrd());

   if (thrd_ptr == 0) return false;

   bool isowncq = IsAllocateIndividualCQ() && !thrd_ptr->IsFastModus();

   if (isowncq)
      port_cq = new ComplQueue(fIbContext, port->NumOutputBuffersRequired() + port->NumInputBuffersRequired() + 2, thrd_ptr->Channel());
   else
      port_cq = thrd_ptr->MakeCQ();

   int num_send_seg = fIbContext.max_sge() - 1;
   if (conn_type==IBV_QPT_UD) num_send_seg = fIbContext.max_sge() - 5; // I do not now why, but otherwise it fails
   if (num_send_seg<2) num_send_seg = 2;

   port_qp = new QueuePair(IbContext(), qp_type,
                           port_cq, port->NumOutputBuffersRequired(), num_send_seg,
                           port_cq, port->NumInputBuffersRequired(), /*fIbContext.max_sge() / 2*/ 2);

   if (!isowncq)
      port_cq = 0;

   return port_qp->qp()!=0;
}


void verbs::Device::CreateVerbsTransport(dabc::ThreadRef thrd, dabc::Reference portref, bool useackn, ComplQueue* cq, QueuePair* qp)
{
   dabc::Port* port = (dabc::Port*) portref();

   if (thrd.null() || (port==0) || (qp==0)) {
      EOUT(("verbs::Thread %p or Port %p or QueuePair %p disappear!!!", thrd(), port, qp));
      delete qp;
      delete cq;
      return;
   }

   Transport* tr = new Transport(fIbContext, cq, qp, portref, useackn);

   dabc::Reference handle(tr);

   if (tr->AssignToThread(thrd))
      port->AssignTransport(handle, tr);
   else {
      EOUT(("No thread for transport"));
      handle.Destroy();
   }
}

dabc::ThreadRef verbs::Device::MakeThread(const char* name, bool force)
{
   ThreadRef thrd = dabc::mgr()->FindThread(name, VERBS_THRD_CLASSNAME);

   if (!thrd.null() || !force) return thrd;

   return dabc::mgr()->CreateThread(name, VERBS_THRD_CLASSNAME, GetName());
}

dabc::Transport* verbs::Device::CreateTransport(dabc::Command cmd, dabc::Reference portref)
{
   dabc::Port* port = (dabc::Port*) portref();
   if (port==0) return 0;

   std::string maddr = port->Cfg(xmlMcastAddr, cmd).AsStdStr();

   if (!maddr.empty()) {
      std::string thrdname = port->Cfg(dabc::xmlTrThread,cmd).AsStdStr(ThreadName());

      ibv_gid multi_gid;

      if (!ConvertStrToGid(maddr, multi_gid)) {
         EOUT(("Cannot convert address %s to ibv_gid type", maddr.c_str()));
         return 0;
      }

      std::string buf = ConvertGidToStr(multi_gid);
      if (buf!=maddr) {
         EOUT(("Addresses not the same: %s - %s", maddr.c_str(), buf.c_str()));
         return 0;
      }

      ComplQueue* port_cq(0);
      QueuePair*  port_qp(0);
      ThreadRef   thrd;

      if (CreatePortQP(thrdname.c_str(), port, IBV_QPT_UD, thrd, port_cq, port_qp))
         return new Transport(fIbContext, port_cq, port_qp, portref, false, &multi_gid);
   }

   return 0;
}

int verbs::Device::HandleManagerConnectionRequest(dabc::Command cmd)
{
   std::string reqitem = cmd.GetStdStr(dabc::ConnectionManagerHandleCmd::ReqArg());

   dabc::ConnectionRequestFull req = dabc::mgr.FindPar(reqitem);

   if (req.null()) return dabc::cmd_false;

   switch (req.progress()) {

      // here on initializes connection
      case dabc::ConnectionManager::progrDoingInit: {

         dabc::PortRef port = req.GetPort();
         if (port.null()) {
            EOUT(("No port is available for the request"));
            return dabc::cmd_false;
         }

         ProtocolWorker* proc = new ProtocolWorker;

         // FIXME: ConnectionRequest should be used
         if (!CreatePortQP(req.GetConnThread().c_str(), port(), 0,
                           proc->fPortThrd, proc->fPortCQ, proc->fPortQP)) {
            delete proc;
            return dabc::cmd_false;
         }

         std::string portid;

         dabc::formats(portid,"%04X:%08X:%08X", (unsigned) fIbContext.lid(), (unsigned) proc->fPortQP->qp_num(), (unsigned) proc->fPortQP->local_psn());

         DOUT0(("CREATE CONNECTION %s", portid.c_str()));

         if (req.IsServerSide()) {
            req.SetServerId(portid);
         } else
            req.SetClientId(portid);

         // make backpointers, fCustomData is reference, automatically cleaned up by the connection manager
         req.SetCustomData(dabc::Reference(proc, true));

         proc->fReqItem = reqitem;

         proc->fDevice = this;

         return dabc::cmd_true;
      }

      case dabc::ConnectionManager::progrDoingConnect: {
         // one should register request and start connection here

         DOUT2(("****** VERBS START: %s %s CONN: %s *******", (req.IsServerSide() ? "SERVER" : "CLIENT"), req.GetConnId().c_str(), req.GetConnInfo().c_str()));

         // once connection is started, custom data is no longer necessary by connection record
         // protocol worker will be cleaned up automatically either when connection is done or when connection is timedout

         // by coping of the reference source reference will be cleaned
         ProtocolWorkerRef proc = req.TakeCustomData();

         if (proc.null()) {
            EOUT(("SOMETHING WRONG - NO PROTOCOL PROCESSOR for the connection request"));
            return dabc::cmd_false;
         }

         std::string remoteid;
         bool res = true;

         if (req.IsServerSide())
            remoteid = req.GetClientId();
         else
            remoteid = req.GetServerId();

         if (sscanf(remoteid.c_str(),"%X:%X:%X", &proc()->fRemoteLID, &proc()->fRemoteQPN, &proc()->fRemotePSN)!=3) {
            EOUT(("Cannot decode remote id string %s", remoteid.c_str()));
            res = false;
         }

         // reply remote command that one other side can start connection
         req.ReplyRemoteCommand(res);

         if (res == dabc::cmd_false) {
            proc.Release();

            return dabc::cmd_false;
         }

         DOUT0(("CONNECT TO REMOTE %04x:%08x:%08x - %s", proc()->fRemoteLID, proc()->fRemoteQPN, proc()->fRemotePSN, remoteid.c_str()));

         // FIXME: remote port should be handled correctly
         if (proc()->fPortQP->Connect(proc()->fRemoteLID, proc()->fRemoteQPN, proc()->fRemotePSN)) {

            proc()->fPool = new MemoryPool(fIbContext, "HandshakePool", 1, 1024, false);

            proc()->fLocalCmd = cmd;

            proc()->SetQP(proc()->fPortQP);

            // we need to preserve thread reference until transport itself will be created
            proc()->AssignToThread(proc()->fPortThrd.Ref());

            proc.SetOwner(false);

            return dabc::cmd_postponed;
         }

         proc.Release();

         return dabc::cmd_false;

         break;
      }


      default:
         EOUT(("Request from connection manager in undefined situation progress = %d ???", req.progress()));
         return dabc::cmd_false;
         break;
   }

   return dabc::cmd_true;
}


int verbs::Device::ExecuteCommand(dabc::Command cmd)
{
   int cmd_res = dabc::cmd_true;

   DOUT5(("Execute command %s", cmd.GetName()));


   if (cmd.IsName(dabc::ConnectionManagerHandleCmd::CmdName())) {

      cmd_res = HandleManagerConnectionRequest(cmd);

   } else

     cmd_res = dabc::Device::ExecuteCommand(cmd);

   return cmd_res;
}

double verbs::Device::ProcessTimeout(double last_diff)
{
   return -1;
}

bool verbs::ConvertStrToGid(const std::string& s, ibv_gid &gid)
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
