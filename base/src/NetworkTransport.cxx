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
#include "dabc/PoolHandle.h"
#include "dabc/Port.h"
#include "dabc/Manager.h"
#include "dabc/Device.h"
#include "dabc/BuffersQueue.h"

dabc::NetworkTransport::NetworkTransport(Reference port, bool useakn) :
   Transport(port),
   MemoryPoolRequester(),
   fTransportId(0),
   fIsInput(false),
   fIsOutput(false),
   fUseAckn(useakn),
   fInputQueueLength(0),
   fOutputQueueLength(0),
   fMutex(),
   fNumRecs(0),
   fRecsCounter(0),
   fRecs(0),
   fOutputQueueSize(0),
   fAcknAllowedOper(0),
   fAcknSendQueue(),
   fAcknSendBufBusy(false),
   fInputQueueSize(0),
   fInputQueue(),
   fFirstAckn(true),
   fAcknReadyCounter(0),
   fFullHeaderSize(0),
   fInlineDataSize(0)
{
}

dabc::NetworkTransport::~NetworkTransport()
{
   DOUT5((" NetworkTransport::~NetworkTransport() %p", this));
}

void dabc::NetworkTransport::InitNetworkTransport(Object* this_transport)
{
   // This method must be called from constructor of inherited class or
   // immediately afterwards.
   //
   // This method was implemented separately from constructor, while
   // we want to call here some virtual methods and therefore,
   // constructor of inherited class must be active already.


   RegisterTransportDependencies(this_transport);

   if (GetPort()==0) {
      EOUT(("Already here port is 0 - wrong!!!"));
      exit(876);
   }

   DOUT3(("Port %s use ackn %s", GetPort()->GetName(), DBOOL(fUseAckn)));

   fInputQueueLength = GetPort()->InputQueueCapacity();
   fIsInput = fInputQueueLength > 0;

   fOutputQueueLength = GetPort()->OutputQueueCapacity();
   fIsOutput = fOutputQueueLength > 0;

   if (fUseAckn) {
      if (fInputQueueLength<AcknoledgeQueueLength)
         fInputQueueLength = AcknoledgeQueueLength;
   }

   fInputQueueSize = 0;
   fFirstAckn = true;
   fAcknReadyCounter = 0;

   fOutputQueueSize = 0;
   fAcknAllowedOper = 0;

   fInlineDataSize = GetPort()->InlineDataSize();
   fFullHeaderSize = sizeof(NetworkHeader) + fInlineDataSize;

   fNumRecs = GetPort()->NumInputBuffersRequired() + GetPort()->NumOutputBuffersRequired();
   fRecsCounter = 0;
   fNumUsedRecs = 0;
   if (fNumRecs>0) {
      fRecs = new NetIORec[fNumRecs];
      for (uint32_t n=0;n<fNumRecs;n++) {
         fRecs[n].used = false;
         fRecs[n].kind = 0;
         fRecs[n].buf.Release();
         fRecs[n].header = 0;
         fRecs[n].inlinebuf = 0;
      }
   }

   if (fIsOutput && fUseAckn)
      fAcknSendQueue.Allocate(fOutputQueueLength);

   if (fInputQueueLength>0) {
      if (GetPool()==0) {
         EOUT(("Pool required for input transport"));
         return;
      }

      fInputQueue.Allocate(fInputQueueLength);

      fAcknSendBufBusy = false;
   }
}

void dabc::NetworkTransport::SetRecHeader(uint32_t recid, void* header)
{
   fRecs[recid].header = header;
   if (fInlineDataSize > 0)
      fRecs[recid].inlinebuf = (char*) header + sizeof(NetworkHeader);
}

void dabc::NetworkTransport::PortAssigned()
{
   DOUT2(("NetworkTransport:: port %s assigned", fPort.GetName()));

   FillRecvQueue();
}

void dabc::NetworkTransport::CleanupFromTransport(Object* obj)
{
   if (obj == GetPool())
      CleanupNetTransportQueue();

   dabc::Transport::CleanupFromTransport(obj);
}

void dabc::NetworkTransport::CleanupTransport()
{
   // first, exclude possibility to get callback from pool
   // anyhow, we should lock access to all queues, but
   // release buffers without involving of lock
   // therefore we use intermediate queue, which we release at very end

   DOUT3(("Calling NetworkTransport::Cleanup() %p id = %d pool %s", this, GetId(), fPool.GetName()));

   CleanupNetTransportQueue();

   DOUT3(("Calling NetworkTransport::Cleanup() done %p", this));

   dabc::Transport::CleanupTransport();

   DOUT3(("Calling Transport::Cleanup() done %p", this));

}

void dabc::NetworkTransport::CleanupNetTransportQueue()
{
   unsigned maxsz = 32;
   {
      dabc::LockGuard guard(fMutex);
      maxsz = fNumRecs;
   }

   dabc::BuffersQueue relqueue(maxsz);

   {
      dabc::LockGuard guard(fMutex);

      if (fInputQueueLength>0)
         if (GetPool()) GetPool()->RemoveRequester(this);

      for (uint32_t n=0;n<fNumRecs;n++)
         if (fRecs[n].used) {
            fRecs[n].used = false;
            relqueue.Push(fRecs[n].buf);
         }

      delete[] fRecs; fRecs = 0; fNumRecs = 0;

      fInputQueue.Reset();

      fAcknSendQueue.Reset();
   }

   DOUT3(("Calling NetworkTransport::Cleanup() %p  relqueue %u", this, relqueue.Size()));

  relqueue.Cleanup();
}


uint32_t dabc::NetworkTransport::_TakeRec(const Buffer& buf, uint32_t kind, uint32_t extras)
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

   EOUT(("Cannot allocate NetIORec. Halt"));
   EOUT(("SendQueue %u RecvQueue %u NumRecs %u used %u", fOutputQueueSize, fInputQueueSize, fNumRecs, fNumUsedRecs));
   return fNumRecs;
}

void dabc::NetworkTransport::_ReleaseRec(uint32_t recid)
{
   if (recid<fNumRecs) {
      if (!fRecs[recid].buf.null()) EOUT(("Buffer is not empty when record is released !!!!"));
      fRecs[recid].used = false;
      fNumUsedRecs--;
   } else {
      EOUT(("Error recid %u", recid));
   }
}

bool dabc::NetworkTransport::Recv(Buffer &buf)
{
   {
      dabc::LockGuard guard(fMutex);

      if (fInputQueue.Size()>0) {
         uint32_t recid = fInputQueue.Pop();

         buf << fRecs[recid].buf;
         _ReleaseRec(recid);

         fInputQueueSize--;
      }
   }

   if (!buf.null()) FillRecvQueue();

   return !buf.null();
}

unsigned dabc::NetworkTransport::RecvQueueSize() const
{
   dabc::LockGuard guard(fMutex);
   return fInputQueue.Size();
}

dabc::Buffer& dabc::NetworkTransport::RecvBuffer(unsigned indx) const
{
   dabc::LockGuard guard(fMutex);
   if (indx>=fInputQueue.Size()) throw dabc::Exception("wrong argument in NetworkTransport::RecvBuffer call");

   return fRecs[fInputQueue.Item(indx)].buf;
}

bool dabc::NetworkTransport::Send(const Buffer& buf)
{
   if (buf.null()) return false;

   dabc::LockGuard guard(fMutex);

   fOutputQueueSize++;
   uint32_t recid = _TakeRec(buf, netot_Send);
   if (recid==fNumRecs) return false;

   // from this moment buf should be used from record directly

   if (fAcknSendQueue.Capacity() > 0) {
      fAcknSendQueue.Push(recid);
      _SubmitAllowedSendOperations();
   } else {
      _SubmitSend(recid);
   }

   return true;
}

unsigned dabc::NetworkTransport::SendQueueSize()
{
   dabc::LockGuard guard(fMutex);
   return fOutputQueueSize;
}

void dabc::NetworkTransport::FillRecvQueue(Buffer* freebuf)
{
   // method used to keep receive queue filled
   // Sometime one need to reinject buffer, which was received as "fast",
   // therefore its processing finished in transport thread and we can
   // use it again in receive queue.

   unsigned newitems = 0;

   if (GetPool() && !IsTransportErrorFlag()) {
      dabc::LockGuard guard(fMutex);

      while (fInputQueueSize<fInputQueueLength) {
         Buffer buf;
         if (freebuf) buf << *freebuf;

         if (buf.null()) {

            buf = GetPool()->TakeBufferReq(this);

            if (buf.null()) break;
         }

         uint32_t recvrec = _TakeRec(buf, netot_Recv);
         fInputQueueSize++;
         newitems++;
         _SubmitRecv(recvrec);

         // if we get free buffer, just exit and do not try any new requests
         if (freebuf) break;
      }
   }

   // no need to release additional buffer if it was not used, it will be done in upper method
   // if (freebuf) freebuf->Release();

   CheckAcknReadyCounter(newitems);
}

bool dabc::NetworkTransport::CheckAcknReadyCounter(unsigned newitems)
{
   // check if count of newly submitted recv buffers exceed limit
   // after which one should send acknowledge packet to receiver

   DOUT5(("CheckAcknReadyCounter ackn:%s pool:%p inp:%s", DBOOL(fUseAckn), GetPool(), DBOOL(fIsInput)));

   if (!fUseAckn || (GetPool()==0) || !fIsInput) return false;

   dabc::LockGuard guard(fMutex);

   fAcknReadyCounter+=newitems;

   if (fAcknSendBufBusy) return false;

   unsigned ackn_limit = fFirstAckn ? fInputQueueLength : fInputQueueLength/2;
   if (ackn_limit<1) ackn_limit = 1;

   DOUT5(("fAcknReadyCounter = %d limit = %d", fAcknReadyCounter, ackn_limit));

   // check if we need to send ackn packet
   if (fAcknReadyCounter<ackn_limit) return false;

   fAcknSendBufBusy = true;

   fAcknReadyCounter -= ackn_limit;

   fFirstAckn = false;

   uint32_t recid = _TakeRec(Buffer(), netot_HdrSend, ackn_limit);

   _SubmitSend(recid);

   return true;
}

void dabc::NetworkTransport::_SubmitAllowedSendOperations()
{
   while ((fAcknAllowedOper>0) && (fAcknSendQueue.Size()>0)) {
      uint32_t recid = fAcknSendQueue.Pop();
      fAcknAllowedOper--;
      _SubmitSend(recid);
   }
}

void dabc::NetworkTransport::ProcessSendCompl(uint32_t recid)
{
   if (recid>=fNumRecs) { EOUT(("Recid fail %u %u", recid, fNumRecs)); return; }

   Buffer buf;
   bool dofire = false;
   bool checkackn = false;

   {
      dabc::LockGuard guard(fMutex);

      buf << fRecs[recid].buf;

      if (fRecs[recid].kind & netot_Send) {
         // normal send
         fOutputQueueSize--;
         dofire = true;
      } else
      if (fRecs[recid].kind & netot_HdrSend) {
         if (!buf.null()) EOUT(("Non-zero buffer with sending netot_HdrSend"));
         fAcknSendBufBusy = false;
         checkackn = true;
      } else {
         EOUT(("Wrong kind=%u in ProcessSendCompl", fRecs[recid].kind));
      }

      _ReleaseRec(recid);
   }

   // we releasing buffer out of locked area, while it can make indirect call
   // back to tranport instance via memory pool event handling
   buf.Release();

   if (dofire) FirePortOutput();

   if (checkackn) CheckAcknReadyCounter(0);
}

void dabc::NetworkTransport::ProcessRecvCompl(uint32_t recid)
{
   // method return true, if fast packet was received and transport
   // should try to speed up its threads and probably, for some time switch
   // in polling mode

   if (recid>=fNumRecs) {
      EOUT(("Recid fail tr %p %u %u", this, recid, fNumRecs));
      return;
//      exit(107);
   }

   Buffer buf;
   uint32_t kind = 0;
   bool dofire = false;
   bool doerrclose = false;

   {
      dabc::LockGuard guard(fMutex);

      NetworkHeader* hdr = (NetworkHeader*) fRecs[recid].header;

      if (hdr->chkword != 123) {
         EOUT(("Error in network header magic number"));
         doerrclose = true;
      }

      kind = hdr->kind;

      // check special case when we send only network header and nothing else
      // for the moment this is only work with AcknCounter, later can be extend for other applications
      if (kind & netot_HdrSend) {
         uint32_t extras = hdr->size;

//         DOUT1(("Get ackn counter = %llu", extras));

         fAcknAllowedOper += extras;
         _SubmitAllowedSendOperations();

         fInputQueueSize--;

         buf << fRecs[recid].buf;

         _ReleaseRec(recid);
      } else {
//         DOUT0(("ProcessRecvCompl recid %u totalsize %u bufsize %u", recid, hdr->size, fRecs[recid].buf.GetTotalSize()));

         fRecs[recid].buf.SetTotalSize(hdr->size);
         fRecs[recid].buf.SetTypeId(hdr->typid);

         if ((hdr->size>0) && (hdr->size <= fInlineDataSize))
            fRecs[recid].buf.CopyFrom(fRecs[recid].inlinebuf, hdr->size);

         fInputQueue.Push(recid);
         dofire = true;
      }
   }

//   DOUT0(("Network transport complete fire: %s buf %p", DBOOL(dofire), buf));

   if (dofire)
      FirePortInput();
   else
   if (doerrclose)
      CloseTransport(true);
   else
      FillRecvQueue(&buf);
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
