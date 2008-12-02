#include "dabc/NetworkTransport.h"

#include "dabc/logging.h"
#include "dabc/MemoryPool.h"
#include "dabc/Port.h"
#include "dabc/PoolHandle.h"
#include "dabc/Manager.h"
#include "dabc/Device.h"

dabc::NetworkTransport::NetworkTransport(Device* device) :
   Transport(device),
   MemoryPoolRequester(),
   fTransportId(0),
   fIsInput(false),
   fIsOutput(false),
   fPool(0),
   fUseAckn(false),
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
   fInputBufferSize(0),
   fInputQueueSize(0),
   fInputQueue(),
   fFirstAckn(true),
   fAcknReadyCounter(0),
   fFullHeaderSize(0),
   fUserHeaderSize(0)

{
}

dabc::NetworkTransport::~NetworkTransport()
{
   DOUT5((" NetworkTransport::~NetworkTransport() %p", this));
}

void dabc::NetworkTransport::Init(Port *port, bool useackn)
{
   // This method must be called from constructor of inherited class or
   // immediately afterwards.
   //
   // This method was implemented separately from constructor, while
   // we want to call here some virtual methods and therefore,
   // constructor of inherited class must be active already.

   fPool = port->Pool() ? port->Pool()->getPool() : 0;
   fUseAckn = useackn;

   DOUT3(("Port %s use ackn %s", port->GetName(), DBOOL(fUseAckn)));

   fInputQueueLength = port->InputQueueCapacity();
   fIsInput = fInputQueueLength > 0;

   fOutputQueueLength = port->OutputQueueCapacity();
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

   fUserHeaderSize = port->UserHeaderSize();
   fFullHeaderSize = fUserHeaderSize + sizeof(NetworkHeader);

   fNumRecs = port->NumInputBuffersRequired() + port->NumOutputBuffersRequired();
   fRecsCounter = 0;
   fNumUsedRecs = 0;
   if (fNumRecs>0) {
      fRecs = new NetIORec[fNumRecs];
      for (uint32_t n=0;n<fNumRecs;n++) {
         fRecs[n].used = false;
         fRecs[n].kind = 0;
         fRecs[n].buf = 0;
         fRecs[n].header = 0;
         fRecs[n].usrheader = 0;
      }
   }

   if (fIsOutput && fUseAckn)
      fAcknSendQueue.Allocate(fOutputQueueLength);

   if (fInputQueueLength>0) {
      if (fPool==0) {
         EOUT(("Pool required for input transport"));
         return;
      }

      fInputBufferSize = port->Pool()->GetRequiredBufferSize();

      fInputQueue.Allocate(fInputQueueLength);

      fAcknSendBufBusy = false;
   }
}

void dabc::NetworkTransport::SetRecHeader(uint32_t recid, void* header)
{
   fRecs[recid].header = header;
   if (fUserHeaderSize > 0)
      fRecs[recid].usrheader = (char*) header + sizeof(NetworkHeader);
}

void dabc::NetworkTransport::PortChanged()
{
   if (IsPortAssigned())
      FillRecvQueue();
}

void dabc::NetworkTransport::Cleanup()
{
   // first, exclude possibility to get callback from pool
   // anyhow, we should lock access to all queues, but
   // release buffers without involving of lock
   // therefore we use intermediate queue, which we release at very end

   DOUT5(("Calling NetworkTransport::Cleanup() %p id = %d", this, GetId()));

   dabc::BuffersQueue relqueue(fNumRecs);

   {
      dabc::LockGuard guard(fMutex);

      if (fInputQueueLength>0)
         if (fPool) fPool->RemoveRequester(this);

      fPool = 0;

      for (uint32_t n=0;n<fNumRecs;n++)
         if (fRecs[n].used) {
            fRecs[n].used = false;
            if (fRecs[n].buf) relqueue.Push(fRecs[n].buf);
            fRecs[n].buf = 0;
         }

      delete[] fRecs; fRecs = 0; fNumRecs = 0;
   }

   DOUT5(("Calling NetworkTransport::Cleanup() %p  relqueue %u", this, relqueue.Size()));

   relqueue.Cleanup();

   DOUT5(("Calling NetworkTransport::Cleanup() done %p", this));
}

uint32_t dabc::NetworkTransport::_TakeRec(Buffer* buf, uint32_t kind, uint64_t extras)
{
   uint32_t cnt = fNumRecs;
   uint32_t recid = 0;
   while (cnt-->0) {
      recid = fRecsCounter;
      fRecsCounter = (fRecsCounter+1) % fNumRecs;
      if (!fRecs[recid].used) {
         fRecs[recid].used = true;
         fRecs[recid].kind = kind;
         fRecs[recid].buf = buf;
         fRecs[recid].extras = extras;
         fNumUsedRecs++;
         return recid;
      }
   }

   EOUT(("Cannot allocate NetIORec. Halt"));
   EOUT(("SendQueue %u RecvQueue %u NumRecs %u used %u", fOutputQueueSize, fInputQueueSize, fNumRecs, fNumUsedRecs));
   exit(1);
   return 0;
}

void dabc::NetworkTransport::_ReleaseRec(uint32_t recid)
{
   if (recid<fNumRecs) {
      fRecs[recid].buf = 0;
      fRecs[recid].used = false;
      fNumUsedRecs--;
   } else {
      EOUT(("Error recid %u", recid));
   }
}

bool dabc::NetworkTransport::Recv(Buffer* &buf)
{
   buf = 0;

   {
      dabc::LockGuard guard(fMutex);

      if (fInputQueue.Size()>0) {
         uint32_t recid = fInputQueue.Pop();

         buf = fRecs[recid].buf;
         _ReleaseRec(recid);

         fInputQueueSize--;
      }
   }

   if (buf) FillRecvQueue();

   return buf!=0;
}

unsigned dabc::NetworkTransport::RecvQueueSize() const
{
   dabc::LockGuard guard(fMutex);
   return fInputQueue.Size();
}

dabc::Buffer* dabc::NetworkTransport::RecvBuffer(unsigned indx) const
{
   dabc::LockGuard guard(fMutex);
   return indx < fInputQueue.Size() ? fRecs[fInputQueue.Item(indx)].buf : 0;
}

bool dabc::NetworkTransport::Send(Buffer* buf)
{
   if (buf==0) return false;

   dabc::LockGuard guard(fMutex);

   fOutputQueueSize++;
   uint32_t recid = _TakeRec(buf, netot_Send);

   BufferSize_t hdrsize = buf->GetHeaderSize();
   if (hdrsize>fUserHeaderSize) hdrsize = fUserHeaderSize;
   if ((hdrsize>0) && fRecs[recid].usrheader)
      memcpy(fRecs[recid].usrheader, buf->GetHeader(), hdrsize);

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
   // method used to keep recieve queue filled
   // Sometime one need to reinject buffer, which was received as "fast",
   // therefore its processing finished in transport thread and we can
   // use it again in recieve queue.

   unsigned newitems = 0;

   if (fPool && !IsErrorState()) {
      dabc::LockGuard guard(fMutex);

      while (fInputQueueSize<fInputQueueLength) {
         Buffer* buf = freebuf;

         if (buf==0) {

            buf = fPool->TakeBufferReq(this, fInputBufferSize, fUserHeaderSize);

//            if (fPool->IsName("RecvPool"))
//               DOUT1(("Request RecvPool buf %u %u res:%p", fInputBufferSize, fUserHeaderSize, buf));

            if (buf==0) break;
         }

         uint32_t recvrec = _TakeRec(buf, netot_Recv);
         fInputQueueSize++;
         newitems++;
         _SubmitRecv(recvrec);

         // if we get free buffer, just exit and do not try any new requests
         if (freebuf) {
            freebuf = 0;

            break;
//            if (isfastbuf) break;
         }
      }
   }

   if (freebuf!=0) {
      EOUT(("Not used explicitely requested buffer %d %d", fInputQueueSize, fInputQueueLength));
   }

   // we must release buffer if we cannot use it
   dabc::Buffer::Release(freebuf);

   CheckAcknReadyCounter(newitems);
}

bool dabc::NetworkTransport::CheckAcknReadyCounter(unsigned newitems)
{
   // check if count of newly submitted recv buffers exceed limit
   // after which one should send acknowledge packet to receiver

   DOUT5(("CheckAcknReadyCounter ackn:%s pool:%p inp:%s", DBOOL(fUseAckn), fPool, DBOOL(fIsInput)));

   if (!fUseAckn || (fPool==0) || !fIsInput) return false;

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

   uint32_t recid = _TakeRec(0, netot_HdrSend, ackn_limit);

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
   if (recid>=fNumRecs) { EOUT(("Recid fail %u %u", recid, fNumRecs)); exit(1); }

   Buffer* buf = 0;
   bool dofire = false;
   bool checkackn = false;

   {
      dabc::LockGuard guard(fMutex);

      buf = fRecs[recid].buf;

      if (fRecs[recid].kind & netot_Send) {
         // normal send
         fOutputQueueSize--;
         dofire = true;
      } else
      if (fRecs[recid].kind & netot_HdrSend) {
         if (buf!=0) EOUT(("Non-zero buffer with sending netot_HdrSend"));
         fAcknSendBufBusy = false;
         checkackn = true;
      } else {
         EOUT(("Wrong kind=%u in ProcessSendCompl", fRecs[recid].kind));
      }

      _ReleaseRec(recid);
   }

   // we releasing buffer out of locked area, while it can make indirect call
   // back to tranport instance via memory pool event handling
   dabc::Buffer::Release(buf);

   if (dofire) FireOutput();

   if (checkackn) CheckAcknReadyCounter(0);
}

void dabc::NetworkTransport::ProcessRecvCompl(uint32_t recid)
{
   // method return true, if fast packet was received and transport
   // should try to speed up its threads and probably, for some time switch
   // in polling mode

   if (recid>=fNumRecs) { EOUT(("Recid fail %u %u", recid, fNumRecs)); exit(1); }

   Buffer* buf = 0;
   uint32_t kind = 0;
   bool dofire = false;

   {
      dabc::LockGuard guard(fMutex);

      NetworkHeader* hdr = (NetworkHeader*) fRecs[recid].header;

      if (hdr->chkword != 123) {
         EOUT(("Error in network header magic number"));
      }

      kind = hdr->kind;

      buf = fRecs[recid].buf;

      // check special case when we send only network header and nothing else
      // for the moment this is only work with AcknCounter, later can be extend for other applications
      if (kind & netot_HdrSend) {
         uint64_t extras = hdr->size;
         extras = extras*0x100000000LLU + hdr->hdrsize;

//         DOUT1(("Get ackn counter = %llu", extras));

         fAcknAllowedOper += extras;
         _SubmitAllowedSendOperations();

         fInputQueueSize--;
         _ReleaseRec(recid);
      } else {
         if (buf->NumSegments() > 0) buf->SetDataSize(hdr->size);
         buf->SetTypeId(hdr->typid);

         buf->SetHeaderSize(hdr->hdrsize);

         if (hdr->hdrsize>0)
            memcpy(buf->GetHeader(), fRecs[recid].usrheader, hdr->hdrsize);

         fInputQueue.Push(recid);
         dofire = true;
      }
   }

   if (dofire)
      FireInput();
   else
      FillRecvQueue(buf);
}

int dabc::NetworkTransport::PackHeader(uint32_t recid)
{
   // function packs netwrok header and return size which must be transported

   NetworkHeader* hdr = (NetworkHeader*) fRecs[recid].header;
   Buffer* buf = fRecs[recid].buf;

   if (hdr==0) return 0;

   hdr->chkword = 123;
   hdr->kind = fRecs[recid].kind;
   if (buf==0) {
      hdr->size = 0;
      hdr->typid = 0;
      hdr->hdrsize = 0;

      if (hdr->kind & netot_HdrSend) {
         hdr->typid = mbt_AcknCounter;
         hdr->hdrsize = fRecs[recid].extras % 0x100000000LLU;
         hdr->size = fRecs[recid].extras / 0x100000000LLU;
         return sizeof(NetworkHeader);
      }

   } else {
      hdr->size = buf->GetTotalSize();
      hdr->typid = buf->GetTypeId();
      hdr->hdrsize = buf->GetHeaderSize();
   }

   return fFullHeaderSize;
}
