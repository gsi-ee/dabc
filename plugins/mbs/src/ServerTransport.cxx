#include "mbs/ServerTransport.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"

#include "mbs/Device.h"

mbs::MbsServerPort::MbsServerPort(ServerTransport* tr, int serversocket, int portnum) :
   dabc::SocketServerProcessor(serversocket, portnum),
   fTransport(tr)
{
}

void mbs::MbsServerPort::OnClientConnected(int connfd)
{
   if (fTransport)
      fTransport->ProcessConnectionRequest(connfd);
   else {
      EOUT(("Nobody waits for requested connection"));
      close(connfd);
   }
}

// ________________________________________________________________________

mbs::MbsServerIOProcessor::MbsServerIOProcessor(ServerTransport* tr, int fd) :
   dabc::SocketIOProcessor(fd),
   fTransport(tr),
   fStatus(ioInit),
   fSendBuf(0)
{
}

mbs::MbsServerIOProcessor::~MbsServerIOProcessor()
{
   dabc::Buffer::Release(fSendBuf);
   fSendBuf = 0;
}


void mbs::MbsServerIOProcessor::SendInfo(int32_t maxbytes, bool isnewformat)
{
   memset(&fServInfo, 0, sizeof(fServInfo));

   fServInfo.iEndian = 1;      // byte order. Set to 1 by sender
   fServInfo.iMaxBytes = maxbytes;    // maximum buffer size
   fServInfo.iBuffers = 1;     // buffers per stream
   fServInfo.iStreams = isnewformat ? 0 : 1;     // number of streams (could be set to -1 to indicate var length buffers, size l_free[1])

   fStatus = ioInit;

   StartSend(&fServInfo, sizeof(fServInfo));
}

void mbs::MbsServerIOProcessor::OnSendCompleted()
{
   DOUT3(("Send completed status:%d tr:%p", fStatus, fTransport));

   switch (fStatus) {
      case ioInit:
         fStatus = ioReady;
         DOUT0(("Send of initial information completed"));

         break;
      case ioSendingBuffer:
         fStatus = ioReady;
         dabc::Buffer::Release(fSendBuf);
         fSendBuf = 0;
         break;
      default:
         EOUT(("One should not complete send in such status %d", fStatus));
         return;
   }

   FireDataOutput();
}

void mbs::MbsServerIOProcessor::OnRecvCompleted()
{
   if (fStatus != ioWaitingReq) {
      EOUT(("No recv completion is expected up to now"));
      return;
   }

   if (strcmp(f_sbuf, "CLOSE")==0) {
      DOUT1(("Client want to close connection, do it immidiately"));
      // keep state as it is, just wait until connection is cut

      if (fTransport) fTransport->SocketIOClosed(this);

      fStatus = ioDoingClose;

      DestroyProcessor();

      return;
   }

   if (strcmp(f_sbuf, "GETEVT")!=0)
     EOUT(("Wrong request string %s", f_sbuf));
   fStatus = ioWaitingBuffer;

   FireDataOutput();
}

void mbs::MbsServerIOProcessor::OnConnectionClosed()
{
   DOUT0(("Connection to MBS client closed"));

   if (fTransport) fTransport->SocketIOClosed(this);

   dabc::SocketIOProcessor::OnConnectionClosed();
}

void mbs::MbsServerIOProcessor::OnSocketError(int errnum, const char* info)
{

   DOUT0(("Connection to MBS client error"));

   if (fTransport) fTransport->SocketIOClosed(this);

   dabc::SocketIOProcessor::OnSocketError(errnum, info);
}

double mbs::MbsServerIOProcessor::ProcessTimeout(double)
{
   return -1.;
}


void mbs::MbsServerIOProcessor::ProcessEvent(dabc::EventId evnt)
{
    if (dabc::GetEventCode(evnt) != evMbsDataOutput) {
       dabc::SocketIOProcessor::ProcessEvent(evnt);
       return;
    }

    if (fTransport==0) return;

    int kind = fTransport->Kind();

    if (fStatus == ioDoingClose) {
       DOUT5(("Doing close"));
       return;
    }

    if (fStatus == ioReady) {
        if (kind == TransportServer)
           fStatus = ioWaitingBuffer;
        else
        if (kind == StreamServer) {
           strcpy(f_sbuf, "");
           StartRecv(f_sbuf, 12);
           fStatus = ioWaitingReq;
        }
    }

    if (fStatus == ioWaitingReq)
       fTransport->DropFrontBufferIfQueueFull();

    if (fStatus == ioWaitingBuffer) {
       fSendBuf = fTransport->TakeFrontBuffer();
       if (fSendBuf!=0) {
          fStatus = ioSendingBuffer;

//        mbs::sMbsBufferHeader* bufhdr = (mbs::sMbsBufferHeader*) fSendBuf->GetDataLocation();
//        DOUT1(("Send MBS buffer type %x len %d used %d",
//        bufhdr->iType,  bufhdr->BufferLength(), bufhdr->UsedBufferLength()));

          if (fSendBuf->GetTypeId()==mbt_Mbs100_1)
             StartSend(fSendBuf);
          else
          if (fSendBuf->GetTypeId()==mbt_MbsEvs10_1) {
             fHeader.Init(true);
             fHeader.SetUsedBufferSize(fSendBuf->GetTotalSize());
             // error in evapi, must be + sizeof(mbs::BufferHeader)
             fHeader.SetFullSize(fSendBuf->GetTotalSize() - sizeof(mbs::BufferHeader));
             StartNetSend(&fHeader, sizeof(fHeader), fSendBuf, false);
          } else {
              EOUT(("Buffer type %u not supported !!!", fSendBuf->GetTypeId()));
              dabc::Buffer::Release(fSendBuf);
              fSendBuf = 0;
              fStatus = ioWaitingBuffer;
              FireDataOutput();
          }
       }
    }

}

// _____________________________________________________________________

mbs::ServerTransport::ServerTransport(Device* dev, dabc::Port* port,
                                            int kind,
                                            int serversocket,
                                            int portnum,
                                            uint32_t maxbufsize) :
   dabc::Transport(dev),
   fKind(kind),
   fMutex(),
   fOutQueue(port->OutputQueueCapacity()),
   fServerPort(0),
   fIOSocket(0),
   fMaxBufferSize(maxbufsize),
   fSendBuffers(0),
   fDroppedBuffers(0)
{
   fServerPort = new MbsServerPort(this, serversocket, portnum);

   dabc::Manager::Instance()->MakeThreadFor(fServerPort, dev->ProcessorThreadName());
}

mbs::ServerTransport::~ServerTransport()
{
   DOUT1(("mbs::ServerTransport::~ServerTransport queue:%d", fOutQueue.Size()));

   fOutQueue.Cleanup();

   if (fIOSocket) {
      DOUT1(("Close I/O socket"));
      delete fIOSocket;
      fIOSocket = 0;
   }

   if (fServerPort) {
      DOUT1(("Close server port socket"));
      delete fServerPort;
      fServerPort = 0;
   }

   DOUT1(("mbs::ServerTransport::~ServerTransport done queue:%d", fOutQueue.Size()));
}

void mbs::ServerTransport::PortChanged()
{
   if (IsPortAssigned()) return;

   // release buffers as soon as possible, using mutex
   fOutQueue.Cleanup(&fMutex);
}


void mbs::ServerTransport::ProcessConnectionRequest(int fd)
{
   if (fIOSocket!=0) {
      EOUT(("Client is connected, refuse any other connections"));
      close(fd);
      return;
   }

   MbsServerIOProcessor* io = new MbsServerIOProcessor(this, fd);

   dabc::Manager::Instance()->MakeThreadFor(io, GetDevice()->ProcessorThreadName());

   DOUT1(("Create IO for client fd:%d", fd));
   io->SendInfo(fMaxBufferSize + sizeof(mbs::BufferHeader), true);

   dabc::LockGuard guard(fMutex);
   fIOSocket = io;
}

void mbs::ServerTransport::SocketIOClosed(MbsServerIOProcessor* proc)
{
   bool testdrop = false;

   {
      dabc::LockGuard guard(fMutex);

      if (fIOSocket!=proc) return;

      DOUT1(("Transport socket is closed, can accept new connections"));
      fIOSocket = 0;

      DOUT1(("ServerTransport send:%ld dropped:%ld", fSendBuffers, fDroppedBuffers));

      fSendBuffers = 0;
      fDroppedBuffers = 0;

      testdrop = (Kind() == mbs::StreamServer);
   }


   if (testdrop) DropFrontBufferIfQueueFull();
}

dabc::Buffer* mbs::ServerTransport::TakeFrontBuffer()
{
   dabc::Buffer* buf = 0;

   {
      dabc::LockGuard guard(fMutex);
      buf = fOutQueue.Size()>0 ? fOutQueue.Pop() : 0;
      if (buf) fSendBuffers++;
   }

   if (buf!=0) FireOutput();

   return buf;
}

void mbs::ServerTransport::DropFrontBufferIfQueueFull()
{
   dabc::Buffer* buf = 0;

   {
      dabc::LockGuard guard(fMutex);
      buf = fOutQueue.Full() ? fOutQueue.Pop() : 0;
      if (buf) fDroppedBuffers++;
   }

   if (buf!=0) {
      dabc::Buffer::Release(buf);
      FireOutput();
   }
}

bool mbs::ServerTransport::Send(dabc::Buffer* buf)
{
   bool res = false;

   bool testdrop = false;

   {
      dabc::LockGuard guard(fMutex);

      res = fOutQueue.MakePlaceForNext();

      if (res) {
         fOutQueue.Push(buf);
         if (fIOSocket)
            fIOSocket->FireDataOutput();
         else
            testdrop = (Kind() == mbs::StreamServer);
      }
   }

   if (!res) {
      dabc::Buffer::Release(buf);
      EOUT(("Not able put buffer in the queue"));
   }

   if (testdrop) DropFrontBufferIfQueueFull();

   return res;
}

unsigned mbs::ServerTransport::SendQueueSize()
{
   dabc::LockGuard guard(fMutex);

   return fOutQueue.Size();
}
