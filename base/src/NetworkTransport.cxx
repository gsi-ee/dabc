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

#include "dabc/NetworkTransport.h"

#include "dabc/logging.h"
#include "dabc/MemoryPool.h"
#include "dabc/Manager.h"
#include "dabc/Device.h"
#include "dabc/Pointer.h"


dabc::NetworkTransport::NetworkTransport(dabc::Command cmd, const PortRef& inpport, const PortRef& outport, bool useackn, WorkerAddon* addon) :
    dabc::Transport(cmd, inpport, outport),
    fNet(0),
    fTransportId(0),
    fUseAckn(useackn),
    fInputQueueCapacity(0),
    fOutputQueueCapacity(0),
    fNumRecs(0),
    fRecsCounter(0),
    fRecs(0),
    fOutputQueueSize(0),
    fAcknAllowedOper(0),
    fAcknSendQueue(),
    fAcknSendBufBusy(false),
    fInputQueueSize(0),
    fFirstAckn(true),
    fAcknReadyCounter(0),
    fFullHeaderSize(0),
    fInlineDataSize(0),
    fStartBufReq(false)
{
   AssignAddon(addon);

   fNet = (NetworkInetrface*) fAddon.Notify("GetNetworkTransportInetrface");

   if (fNet==0) {
      EOUT("Cannot obtain network addon for the NetworkTransport");
      exit(345);
   }

   if (IsInputTransport())
      fInputQueueCapacity = inpport.QueueCapacity();

   if (IsOutputTransport())
      fOutputQueueCapacity = outport.QueueCapacity();

   DOUT2("Create new net transport inp %s out %s ackn %s", DBOOL(IsInputTransport()), DBOOL(IsOutputTransport()), DBOOL(fUseAckn));

   if (fUseAckn) {
      if (fInputQueueCapacity<AcknoledgeQueueLength)
         fInputQueueCapacity = AcknoledgeQueueLength;
      // TODO: do we need here increment output queue size???
   }


   if (inpport.QueueCapacity() > 0) {

      if (NumPools()==0) {
         EOUT("No memory pool specified to provided buffers for network transport");
         exit(444);
      }

      // use time to request buffer again
      if (!IsAutoPool()) CreateTimer("SysTimer");
   }


   fInputQueueSize = 0;
   fFirstAckn = true;
   fAcknReadyCounter = 0;

   fInlineDataSize = 32; // TODO: configure via port properties
   fFullHeaderSize = sizeof(NetworkHeader) + fInlineDataSize;

   fNumRecs = fInputQueueCapacity + fOutputQueueCapacity + 1;
   fRecsCounter = 0;
   fNumUsedRecs = 0;
   if (fNumRecs>0) {
      fRecs = new NetIORec[fNumRecs];
      for (uint32_t n=0;n<fNumRecs;n++) {
         fRecs[n].used = false;
         fRecs[n].kind = 0;
         fRecs[n].extras = 0;
         // fRecs[n].buf.Release();
         fRecs[n].header = 0;
         fRecs[n].inlinebuf = 0;
      }
   }

   if (IsInputTransport() && fUseAckn)
      fAcknSendQueue.Allocate(fOutputQueueCapacity);

   if (IsInputTransport() && (NumPools()==0)) {
      EOUT("Pool required for input transport or for the acknowledge queue");
      return;
   }

   fNet->AllocateNet(fOutputQueueCapacity + AcknoledgeQueueLength,
                     fInputQueueCapacity + AcknoledgeQueueLength);
}

dabc::NetworkTransport::~NetworkTransport()
{
   DOUT2("#### ~NetworkTransport fRecs %p", fRecs);
}

void dabc::NetworkTransport::TransportCleanup()
{
   dabc::Transport::TransportCleanup();

   DOUT3("NetworkTransport::TransportCleanup");

   // at this moment net should be destroyed by the addon cleanup
   fNet = 0;

   fAcknSendQueue.Reset();

   if (fNumRecs>0) {
      for (uint32_t n=0;n<fNumRecs;n++) {
         fRecs[n].used = false;
         fRecs[n].buf.Release();
      }

      delete[] fRecs;
   }
   fRecs = 0;
   fNumRecs = 0;
   fNumUsedRecs = 0;
}


void dabc::NetworkTransport::SetRecHeader(uint32_t recid, void* header)
{
   fRecs[recid].header = header;
   if (fInlineDataSize > 0)
      fRecs[recid].inlinebuf = (char*) header + sizeof(NetworkHeader);
}

uint32_t dabc::NetworkTransport::TakeRec(Buffer& buf, uint32_t kind, uint32_t extras)
{
   if (fNumRecs == 0) return 0;

   uint32_t cnt = fNumRecs;
   uint32_t recid = 0;
   while (cnt-->0) {
      recid = fRecsCounter;
      fRecsCounter = (fRecsCounter+1) % fNumRecs;
      if (!fRecs[recid].used) {
         fRecs[recid].used = true;
         fRecs[recid].kind = kind;
         fRecs[recid].buf << buf;
         fRecs[recid].extras = extras;
         fNumUsedRecs++;
         return recid;
      }
   }

   EOUT("Cannot allocate NetIORec. Halt");
   EOUT("SendQueue %u RecvQueue %u NumRecs %u used %u", fOutputQueueSize, fInputQueueSize, fNumRecs, fNumUsedRecs);
   return fNumRecs;
}

void dabc::NetworkTransport::ReleaseRec(uint32_t recid)
{
   if (recid<fNumRecs) {
      if (!fRecs[recid].buf.null()) EOUT("Buffer is not empty when record is released !!!!");
      fRecs[recid].used = false;
      fNumUsedRecs--;
   } else {
      EOUT("Error recid %u", recid);
   }
}


int dabc::NetworkTransport::PackHeader(uint32_t recid)
{
   // Returns 0 - failure, 1 - only header should be send, 2 - header and buffer should be send */

   NetworkHeader* hdr = (NetworkHeader*) fRecs[recid].header;

   if (hdr==0) return 0;

   hdr->chkword = 123;
   hdr->kind = fRecs[recid].kind;
   if (fRecs[recid].buf.null()) {
      hdr->size = 0;
      hdr->typid = 0;

      if (hdr->kind & netot_HdrSend) {
         hdr->typid = mbt_AcknCounter;
         hdr->size = fRecs[recid].extras;
      }

      return 1;
   }

   hdr->typid = fRecs[recid].buf.GetTypeId();
   hdr->size = fRecs[recid].buf.GetTotalSize();

   // copy content of the buffer in the inline buffer
   if ((hdr->size>0) && fRecs[recid].inlinebuf && (hdr->size<=fInlineDataSize)) {
      fRecs[recid].buf.CopyTo(fRecs[recid].inlinebuf, hdr->size);
      return 1;
   }

   return 2;
}

void dabc::NetworkTransport::FillRecvQueue(Buffer* freebuf, bool onlyfreebuf)
{
   // method used to keep receive queue filled
   // Sometime one need to reinject buffer, which was received as "fast",
   // therefore its processing finished in transport thread and we can
   // use it again in receive queue.

//   DOUT0("FillRecvQueue  isinp:%s port:%p pool:%p", DBOOL(IsInputTransport()), Output(), Pool());

   if (isTransportError()) return;

   unsigned newitems(0), numcansubmit(0);

   if (IsInputTransport()) {
      if (NumPools()==0) {
         EOUT("No memory pool in input transport");
         CloseTransport(true);
         return;
      }
      if (NumOutputs()==0) {
         EOUT("No output port for input transport");
         CloseTransport(true);
         return;
      }
      numcansubmit = NumCanSend();

//      DOUT0("FillRecvQueue submitlimit %u acutalsize %u", numcansubmit, fInputQueueSize));

   } else {
      // if no input is specified, one only need queue for ackn
      numcansubmit = fInputQueueCapacity;
   }

   while (fInputQueueSize < numcansubmit) {
      Buffer buf;

      if (IsInputTransport()) {
         // only with input transport we need buffers
         if (freebuf) buf << *freebuf;

         if (buf.null()) buf = TakeBuffer();

         if (buf.null()) {
            if (IsAutoPool()) ShootTimer("SysTimer", 0.001);
            break;
         }
      }

      uint32_t recvrec = TakeRec(buf, netot_Recv);
      fInputQueueSize++;
      newitems++;
      fNet->SubmitRecv(recvrec);

      // if we want to reuse only free buffer, just break and do not try to submit any new requests
      if (freebuf && onlyfreebuf) break;
   }

   // no need to release additional buffer if it was not used, it will be done in upper method
   // if (freebuf) freebuf->Release();

   CheckAcknReadyCounter(newitems);
}

bool dabc::NetworkTransport::CheckAcknReadyCounter(unsigned newitems)
{
   // check if count of newly submitted recv buffers exceed limit
   // after which one should send acknowledge packet to receiver

   DOUT5("CheckAcknReadyCounter ackn:%s pool:%s inp:%s", DBOOL(fUseAckn), PoolName().c_str(), DBOOL(IsInputTransport()));

   if (!fUseAckn || (NumPools()==0) || !IsInputTransport()) return false;

   fAcknReadyCounter+=newitems;

   if (fAcknSendBufBusy) return false;

   unsigned ackn_limit = fFirstAckn ? fInputQueueCapacity : fInputQueueCapacity/2;
   if (ackn_limit<1) ackn_limit = 1;

   DOUT5("fAcknReadyCounter = %d limit = %d", fAcknReadyCounter, ackn_limit);

   // check if we need to send ackn packet
   if (fAcknReadyCounter<ackn_limit) return false;

   fAcknSendBufBusy = true;

   fAcknReadyCounter -= ackn_limit;

   fFirstAckn = false;

   dabc::Buffer buf;

   uint32_t recid = TakeRec(buf, netot_HdrSend, ackn_limit);

   fNet->SubmitSend(recid);

   return true;
}

void dabc::NetworkTransport::SubmitAllowedSendOperations()
{
   while ((fAcknAllowedOper>0) && (fAcknSendQueue.Size()>0)) {
      uint32_t recid = fAcknSendQueue.Pop();
      fAcknAllowedOper--;
      fNet->SubmitSend(recid);
   }
}

void dabc::NetworkTransport::ProcessSendCompl(uint32_t recid)
{
   if (recid>=fNumRecs) { EOUT("Recid fail %u %u", recid, fNumRecs); return; }

   bool checkackn(false);

   fRecs[recid].buf.Release();

   if (fRecs[recid].kind & netot_Send) {
      // normal send
      fOutputQueueSize--;

      if (!CanRecv()) {
         EOUT("One cannot recieve buffer!!!!");
         exit(333);
      }

      Recv().Release();

   } else
   if (fRecs[recid].kind & netot_HdrSend) {
      fAcknSendBufBusy = false;
      checkackn = true;
   } else {
      EOUT("Wrong kind=%u in ProcessSendCompl", fRecs[recid].kind);
   }

   ReleaseRec(recid);

   // we releasing buffer out of locked area, while it can make indirect call
   // back to transport instance via memory pool event handling

   if (checkackn) CheckAcknReadyCounter(0);
}

void dabc::NetworkTransport::ProcessRecvCompl(uint32_t recid)
{
   // method return true, if fast packet was received and transport
   // should try to speed up its threads and probably, for some time switch
   // in polling mode

   if (recid>=fNumRecs) {
      EOUT("Recid fail tr %p %u %u", this, recid, fNumRecs);
//      return;
      exit(107);
   }

   NetworkHeader* hdr = (NetworkHeader*) fRecs[recid].header;

   if (hdr->chkword != 123) {
      EOUT("Error in network header magic number");
      ReleaseRec(recid);
      CloseTransport(true);
      return;
   }

   // check special case when we send only network header and nothing else
   // for the moment this is only work with AcknCounter, later can be extend for other applications
   if (hdr->kind & netot_HdrSend) {
      uint32_t extras = hdr->size;

      fAcknAllowedOper += extras;
      SubmitAllowedSendOperations();

      fInputQueueSize--;

      Buffer buf;

      buf << fRecs[recid].buf;

      ReleaseRec(recid);

      FillRecvQueue(&buf);

   } else {

      fInputQueueSize--;

      Buffer buf;

      buf << fRecs[recid].buf;

      buf.SetTotalSize(hdr->size);
      buf.SetTypeId(hdr->typid);

      if ((hdr->size>0) && (hdr->size <= fInlineDataSize))
         Pointer(buf).copyfrom(fRecs[recid].inlinebuf, hdr->size);

      ReleaseRec(recid);

      Send(buf);
   }

}


void dabc::NetworkTransport::OnThreadAssigned()
{
   dabc::Transport::OnThreadAssigned();

   FillRecvQueue();
}


void dabc::NetworkTransport::ProcessInputEvent(unsigned port)
{
   unsigned numbufs = NumCanRecv(port);

   while (fOutputQueueSize < numbufs) {
      // we create copy of the buffer, which will be used in the transport
      // original reference will remain in the port queue until send operation is completed
      Buffer buf = RecvQueueItem(port, fOutputQueueSize);

      uint32_t recid = TakeRec(buf, netot_Send);
      if (recid==fNumRecs) {
         EOUT("No available recs!!!");
         exit(543);
      }

      fOutputQueueSize++;

      // from this moment buf should be used from record directly
      if (fAcknSendQueue.Capacity() > 0) {
         fAcknSendQueue.Push(recid);
      } else {
         fNet->SubmitSend(recid);
      }
   }

   SubmitAllowedSendOperations();
}

void dabc::NetworkTransport::ProcessOutputEvent(unsigned indx)
{
   // when consumer take buffers from the queue, one can try to submit more recv operations
   FillRecvQueue();
}

void dabc::NetworkTransport::ProcessPoolEvent(unsigned pool)
{
   FillRecvQueue();
}

void dabc::NetworkTransport::ProcessTimerEvent(unsigned timer)
{
   FillRecvQueue();
}


bool dabc::NetworkTransport::StartTransport()
{
   dabc::Transport::StartTransport();
   FillRecvQueue();
   return true;
}

bool dabc::NetworkTransport::StopTransport()
{
   return dabc::Transport::StopTransport();
}

void dabc::NetworkTransport::GetRequiredQueuesSizes(const PortRef& port, unsigned& input_size, unsigned& output_size)
{
   PortRef inpport, outport;

   if (port.IsInput()) {
      inpport = port;
      outport = inpport.GetBindPort();
   } else {
      outport = port;
      inpport = outport.GetBindPort();
   }

   input_size = inpport.QueueCapacity() + AcknoledgeQueueLength;
   output_size = outport.QueueCapacity() + AcknoledgeQueueLength;
}


bool dabc::NetworkTransport::Make(const ConnectionRequest& req, WorkerAddon* addon, const std::string &devthrdname)
{
   PortRef port = req.GetPort();

   if (req.null() || port.null()) {
      EOUT("Port or request disappear while connection is ready");
      delete addon;
      return false;
   }

   PortRef inpport, outport;

   if (port.IsInput()) {
      inpport << port;
      outport = inpport.GetBindPort();
   } else {
      outport << port;
      inpport = outport.GetBindPort();
   }

   std::string newthrdname = req.GetConnThread();
   if (newthrdname.empty()) newthrdname = devthrdname;

   dabc::CmdCreateTransport cmd;
   cmd.SetPoolName(req.GetPoolName());

   TransportRef tr = new NetworkTransport(cmd, inpport, outport, req.GetUseAckn(), addon);

   if (tr.MakeThreadForWorker(newthrdname)) {
      tr.ConnectPoolHandles();
      if (!inpport.null())
         dabc::LocalTransport::ConnectPorts(tr.OutputPort(), inpport);
      if (!outport.null())
         dabc::LocalTransport::ConnectPorts(outport, tr.InputPort());

      DOUT0("!!!!!! NETWORK TRANSPORT IS CREATED !!!!");
      return true;

   }

   EOUT("No thread for transport");
   tr.Destroy();
   return false;
}
