#include "mbs/ClientTransport.h"

#include "dabc/Port.h"
#include "dabc/Manager.h"
#include "dabc/MemoryPool.h"

#include "mbs/Device.h"

mbs::ClientIOProcessor::ClientIOProcessor(ClientTransport* cl, int fd) :
   dabc::SocketIOProcessor(fd),
   fTransport(cl),
   fState(ioInit),
   fSwapping(false),
   fRecvBuffer(0)
{

}

mbs::ClientIOProcessor::~ClientIOProcessor()
{
   dabc::Buffer::Release(fRecvBuffer);
}

unsigned mbs::ClientIOProcessor::ReadBufferSize()
{
   uint32_t sz = fHeader.BufferLength();
   if (sz<sizeof(fHeader)) {
      EOUT(("Wrong buffer length %u", sz));
      return 0;
   }

   return sz - sizeof(mbs::BufferHeader);
}


void mbs::ClientIOProcessor::ProcessEvent(dabc::EventId evnt)
{
   switch (dabc::GetEventCode(evnt)) {

      case evReactivate:
         // event appears from transport when it sees first empty place in the queue
         if (fState != ioReady) return;

      case evDataInput:
         if (fTransport==0) return;

         if (fState == ioReady) {
            if (!fTransport->HasPlaceInQueue()) return;
            if (fTransport->Kind() == mbs::StreamServer) {
               strcpy(fSendBuf, "GETVT");
               StartSend(fSendBuf, 12);
            }

            StartRecv(&fHeader, sizeof(fHeader));
            fState = ioRecvHeder;
         }

         if (fState == ioWaitBuffer)
            if (!fTransport->RequestBuffer(ReadBufferSize(), fRecvBuffer)) {
               fState = ioError;
               EOUT(("Cannot request buffer from memory pool"));
            } else
            if (fRecvBuffer!=0) {
      //         DOUT1(("Start receiving of buffer %p %u %p len = %d",
      //               fRecvBuffer, fRecvBuffer->GetTotalSize(), fRecvBuffer->GetDataLocation(), ReadBufferSize()));

               fRecvBuffer->SetTypeId(mbs::mbt_MbsEvs10_1);
               StartRecv(fRecvBuffer, ReadBufferSize());
               fState = ioRecvBuffer;
            }

          if (fState == ioError) {
             EOUT(("Process error"));
             fTransport->CloseTransport();
          }
          break;

      case evRecvInfo:
         if (fState != ioInit) {
            EOUT(("Not an initial state to receive info from server"));
         } else {
            StartRecv(&fServInfo, sizeof(fServInfo), false);
            fState = ioRecvInfo;
         }
         break;

      case evSendClose:
         if (!IsDoingOutput()) {
            DOUT1(("Send close"));
            strcpy(fSendBuf, "CLOSE");
            StartSend(fSendBuf, 12);
            fState = ioClosing;
         }
         break;

      default:
         dabc::SocketIOProcessor::ProcessEvent(evnt);
   }
}

void mbs::ClientIOProcessor::OnSendCompleted()
{
   if ((fState != ioRecvHeder) && (fState != ioClosing)) {
      EOUT(("Complete sending at wrong moment"));
      fState = ioError;
      FireEvent(evDataInput);
   }
}

void mbs::ClientIOProcessor::OnRecvCompleted()
{
   switch (fState) {
      case ioRecvInfo:
         fState = ioReady;
         DOUT1(("Receive server info completed"));
         if (fServInfo.iEndian != 1) {
            SwapMbsData(&fServInfo, sizeof(fServInfo));
            fSwapping = true;
         }

         if (fServInfo.iEndian != 1) {
            EOUT(("Cannot correctly define server endian"));
            fState = ioError;
         }

         FireEvent(evDataInput);

         break;
      case ioRecvHeder:

         DOUT1(("Header received, rest size = %u used %u", ReadBufferSize(), fHeader.UsedBufferSize()));

         if (fSwapping) SwapMbsData(&fHeader, sizeof(fHeader));

         if (ReadBufferSize() > (unsigned) fServInfo.iMaxBytes) {
            EOUT(("Buffer size bigger than allowed by info record"));
            fState = ioError;
         } else
         if (ReadBufferSize() == 0) {
            fState = ioReady;
            DOUT1(("Keep alive buffer from MBS side"));
         } else {
            fState = ioWaitBuffer;
         }

         FireEvent(evDataInput);

         break;

      case ioRecvBuffer:

//         DOUT1(("Provide recv buffer %p to transport", fRecvBuffer));
         fRecvBuffer->SetDataSize(fHeader.UsedBufferSize());
         fTransport->GetReadyBuffer(fRecvBuffer);
         fRecvBuffer = 0;
         fState = ioReady;

         FireEvent(evDataInput);

         break;
      default:
         EOUT(("One should not complete recv in such state %d", fState));
         return;
   }
}

void mbs::ClientIOProcessor::OnConnectionClosed()
{
   if (fTransport) fTransport->SocketClosed(this);

   dabc::SocketIOProcessor::OnConnectionClosed();
}

// _________________________________________________________________________________


mbs::ClientTransport::ClientTransport(Device* dev, dabc::Port* port, int kind) :
   dabc::Transport(dev),
   dabc::MemoryPoolRequester(),
   fKind(kind),
   fIOProcessor(0),
   fMutex(),
   fInpQueue(port->InputQueueCapacity())
{
}

mbs::ClientTransport::~ClientTransport()
{
   if (fIOProcessor!=0) {
      delete fIOProcessor;
      fIOProcessor = 0;
   }
}

bool mbs::ClientTransport::ProcessPoolRequest()
{
   dabc::LockGuard lock(fMutex);

   if (fIOProcessor==0) return false;

   fIOProcessor->FireEvent(ClientIOProcessor::evDataInput);

   return true;
}

bool mbs::ClientTransport::OpenSocket(const char* hostname, int port)
{
   int fd = dabc::SocketThread::StartClient(hostname, port);
   if (fd<=0) return false;
   return SetSocket(fd);
}

bool mbs::ClientTransport::SetSocket(int fd)
{
   if (fIOProcessor!=0) {
      EOUT(("There is connection already exists"));
      close(fd);
      return false;
   }

   if (fd<=0) {
      EOUT(("Wrong socket descriptor %d", fd));
      return false;
   }

   fIOProcessor = new ClientIOProcessor(this, fd);

   dabc::mgr()->MakeThreadFor(fIOProcessor, GetDevice()->ProcessorThreadName());

   fIOProcessor->FireEvent(ClientIOProcessor::evRecvInfo);

   return true;
}

void mbs::ClientTransport::PortChanged()
{
   if (IsPortAssigned()) return;
   fInpQueue.Cleanup(&fMutex);

   dabc::LockGuard lock(fMutex);

   if (fIOProcessor) fIOProcessor->FireEvent(ClientIOProcessor::evSendClose);
}

void mbs::ClientTransport::StartTransport()
{
   dabc::LockGuard lock(fMutex);
   fRunning = true;
   if (fIOProcessor) fIOProcessor->FireEvent(ClientIOProcessor::evReactivate);
}

void mbs::ClientTransport::StopTransport()
{
   dabc::LockGuard lock(fMutex);
   fRunning = false;
}

bool mbs::ClientTransport::Recv(dabc::Buffer* &buf)
{
   buf = 0;

   bool dofire = false;

   {
      dabc::LockGuard lock(fMutex);
      dofire = fInpQueue.Full();
      buf = fInpQueue.Pop();
   }

   if (dofire && fIOProcessor) fIOProcessor->FireEvent(ClientIOProcessor::evReactivate);

   return buf!=0;
}

unsigned mbs::ClientTransport::RecvQueueSize() const
{
   dabc::LockGuard guard(fMutex);

   return fInpQueue.Size();
}

dabc::Buffer* mbs::ClientTransport::RecvBuffer(unsigned indx) const
{
   dabc::LockGuard lock(fMutex);

   return fInpQueue.Item(indx);
}

bool mbs::ClientTransport::HasPlaceInQueue()
{
   dabc::LockGuard lock(fMutex);
   return fRunning && !fInpQueue.Full();
}

bool mbs::ClientTransport::RequestBuffer(uint32_t sz, dabc::Buffer* &buf)
{
   dabc::MemoryPool* pool = GetPortPool();
   if (pool==0) return false;

   buf = pool->TakeBufferReq(this, sz);

   return true;
}

void mbs::ClientTransport::GetReadyBuffer(dabc::Buffer* buf)
{

   {
      dabc::LockGuard lock(fMutex);
      fInpQueue.Push(buf);
   }

   FireInput();
}

void mbs::ClientTransport::SocketClosed(ClientIOProcessor* proc)
{

   {
      dabc::LockGuard lock(fMutex);
      if (fIOProcessor!=proc) return;
      fIOProcessor = 0;
   }

   // from this moment we have nothing to do and can close transport
   CloseTransport();
}
