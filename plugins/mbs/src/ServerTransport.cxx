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
#include "mbs/ServerTransport.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"
#include "dabc/Device.h"

mbs::ServerConnectProcessor::ServerConnectProcessor(ServerTransport* tr, int serversocket, int portnum) :
   dabc::SocketServerProcessor(serversocket, portnum),
   fTransport(tr)
{
}

void mbs::ServerConnectProcessor::OnClientConnected(int connfd)
{
   if (fTransport)
      fTransport->ProcessConnectionRequest(connfd);
   else {
      EOUT(("Nobody waits for requested connection"));
      close(connfd);
   }
}

// ________________________________________________________________________

mbs::ServerIOProcessor::ServerIOProcessor(ServerTransport* tr, int fd) :
   dabc::SocketIOProcessor(fd),
   fTransport(tr),
   fState(ioInit),
   fSendQueue(2),
   fSendBuffers(0),
   fDroppedBuffers(0)
{
}

mbs::ServerIOProcessor::~ServerIOProcessor()
{
   DOUT1(("MBS server processor fd:%d send:%ld dropped:%ld", Socket(), fSendBuffers, fDroppedBuffers));
}

void mbs::ServerIOProcessor::SendInfo(int32_t maxbytes, bool isnewformat)
{
   memset(&fServInfo, 0, sizeof(fServInfo));

   fServInfo.iEndian = 1;      // byte order. Set to 1 by sender
   fServInfo.iMaxBytes = maxbytes;    // maximum buffer size
   fServInfo.iBuffers = 1;     // buffers per stream
   fServInfo.iStreams = isnewformat ? 0 : 1;     // number of streams (could be set to -1 to indicate variable length buffers, size l_free[1])

   fState = ioInit;

   StartSend(&fServInfo, sizeof(fServInfo));
}

void mbs::ServerIOProcessor::OnSendCompleted()
{
   DOUT3(("Send completed status:%d tr:%p", fState, fTransport));

   switch (fState) {
      case ioInit:
         fState = ioReady;
         DOUT2(("MBS Server send info block"));

         break;
      case ioSendingBuffer:
         fSendBuffers++;
         fState = ioReady;
         dabc::Buffer::Release(fSendQueue.Pop());
         break;
      default:
         EOUT(("One should not complete send in such status %d", fState));
         return;
   }

   FireDataOutput();
}

void mbs::ServerIOProcessor::OnRecvCompleted()
{
   if (fState != ioWaitingReq) {
      EOUT(("No recv completion is expected up to now"));
      return;
   }

   if (strcmp(f_sbuf, "CLOSE")==0) {
      DOUT1(("Client want to close connection, do it immediately"));
      // keep state as it is, just wait until connection is cut

      if (fTransport) fTransport->SocketIOClosed(this);

      fState = ioDoingClose;

      DestroyProcessor();

      return;
   }

   if (strcmp(f_sbuf, "GETEVT")!=0)
     EOUT(("Wrong request string %s", f_sbuf));

   fState = ioWaitingBuffer;

   FireDataOutput();
}

void mbs::ServerIOProcessor::OnConnectionClosed()
{
   DOUT0(("Connection to MBS client closed"));

   if (fTransport) fTransport->SocketIOClosed(this);

   dabc::SocketIOProcessor::OnConnectionClosed();
}

void mbs::ServerIOProcessor::OnSocketError(int errnum, const char* info)
{

   DOUT0(("Connection to MBS client error"));

   if (fTransport) fTransport->SocketIOClosed(this);

   dabc::SocketIOProcessor::OnSocketError(errnum, info);
}

double mbs::ServerIOProcessor::ProcessTimeout(double)
{
   return -1.;
}

void mbs::ServerIOProcessor::ProcessEvent(dabc::EventId evnt)
{
    if (dabc::GetEventCode(evnt) != evMbsDataOutput) {
       dabc::SocketIOProcessor::ProcessEvent(evnt);
       return;
    }

    if (fTransport==0) return;

    if (dabc::GetEventArg(evnt)==1)
       fTransport->MoveFrontBuffer(this);

    int kind = fTransport->Kind();

    if (fState == ioDoingClose) {
       DOUT5(("Doing close"));
       return;
    }

    if (fState == ioReady) {
        if (kind == TransportServer)
           fState = ioWaitingBuffer;
        else
        if (kind == StreamServer) {
           strcpy(f_sbuf, "");
           StartRecv(f_sbuf, 12);
           fState = ioWaitingReq;
        }
    }

    if (fState == ioWaitingReq);

    if (fState == ioWaitingBuffer) {

       if (fSendQueue.Size()==0) {
          fTransport->MoveFrontBuffer(this);
          if (fSendQueue.Size()==0) return;
       }

       dabc::Buffer* buf = fSendQueue.Front();

       if (buf->GetTypeId()==mbt_MbsEvents) {
          fHeader.Init(true);
          fHeader.SetUsedBufferSize(buf->GetTotalSize());

          // error in evapi, must be + sizeof(mbs::BufferHeader)
          fHeader.SetFullSize(buf->GetTotalSize() - sizeof(mbs::BufferHeader));
          fState = ioSendingBuffer;

          StartNetSend(&fHeader, sizeof(fHeader), buf, false);
       } else {
          EOUT(("Buffer type %u not supported !!!", buf->GetTypeId()));
          dabc::Buffer::Release(buf);
          fSendQueue.Pop();
          fState = ioWaitingBuffer;
          FireDataOutput();
       }
    }

}

// _____________________________________________________________________

mbs::ServerTransport::ServerTransport(dabc::Device* dev, dabc::Port* port,
                                      int kind,
                                      int serversocket, const std::string& thrdname,
                                      int portnum,
                                      uint32_t maxbufsize) :
   dabc::Transport(dev),
   fKind(kind),
   fMutex(),
   fOutQueue(port->OutputQueueCapacity()),
   fServerPort(0),
   fIOSockets(),
   fMaxBufferSize(maxbufsize)
{
   fServerPort = new ServerConnectProcessor(this, serversocket, portnum);

   dabc::Manager::Instance()->MakeThreadFor(fServerPort, thrdname.c_str());
}

mbs::ServerTransport::~ServerTransport()
{
   DOUT3(("mbs::ServerTransport::~ServerTransport queue:%d", fOutQueue.Size()));

   fOutQueue.Cleanup();

   for (unsigned n=0; n<fIOSockets.size();n++) {
      DOUT1(("Close I/O socket %p", fIOSockets[n]));
      delete fIOSockets[n];
   }

   if (fServerPort) {
      DOUT3(("mbs::ServerTransport Close server port socket"));
      delete fServerPort;
      fServerPort = 0;
   }

   DOUT3(("mbs::ServerTransport::~ServerTransport done queue:%d", fOutQueue.Size()));
}

void mbs::ServerTransport::PortChanged()
{
   if (IsPortAssigned()) return;

   // release buffers as soon as possible, using mutex
   fOutQueue.Cleanup(&fMutex);
}


void mbs::ServerTransport::ProcessConnectionRequest(int fd)
{
   if (fIOSockets.size()>0) {
      if (Kind()== mbs::TransportServer) {
         EOUT(("The only connection allowed for transport server"));
         close(fd);
         return;
      }

      DOUT1(("There are %u connections opened, but we will try to manage new one", fIOSockets.size()));
   }

   ServerIOProcessor* io = new ServerIOProcessor(this, fd);

   dabc::Manager::Instance()->MakeThreadFor(io, fServerPort->ProcessorThreadName());

   DOUT1(("Create IO for client fd:%d", fd));
   io->SendInfo(fMaxBufferSize + sizeof(mbs::BufferHeader), true);

   dabc::LockGuard guard(fMutex);
   fIOSockets.push_back(io);
}

void mbs::ServerTransport::SocketIOClosed(ServerIOProcessor* proc)
{
   bool testdrop = false;

   {
      dabc::LockGuard guard(fMutex);

      for (unsigned n=0;n<fIOSockets.size();n++)
         if (fIOSockets[n]==proc) {
            fIOSockets.erase(fIOSockets.begin()+n);
            testdrop = (Kind() == mbs::StreamServer);
            DOUT1(("Transport socket %p is closed", proc->Socket()));
            break;
         }

      testdrop = (Kind() == mbs::StreamServer);
   }


   if (testdrop) DropFrontBufferIfQueueFull();
}

bool mbs::ServerTransport::MoveFrontBuffer(ServerIOProcessor* callproc)
{
   // method executed in IO sockets thread context, therefore no lock needed to access them

   if (fIOSockets.size()==0) return false;

   bool isanyempty = false;

   for (unsigned n=0;n<fIOSockets.size();n++)
      if (fIOSockets[n]->fSendQueue.Empty()) isanyempty = true;

   if (!isanyempty && (Kind() == mbs::TransportServer)) return false;

   dabc::Buffer* buf = 0;

   {
      dabc::LockGuard guard(fMutex);
      if (fOutQueue.Size()==0) return false;
      if (!isanyempty && !fOutQueue.Full()) return false;
      buf = fOutQueue.Pop();
   }

   unsigned refcnt = 0;

   for (unsigned n=0;n<fIOSockets.size();n++)
      if (fIOSockets[n]->fSendQueue.Full()) {
         fIOSockets[n]->fDroppedBuffers++;
      } else {
         dabc::Buffer* ref = refcnt==0 ? buf : buf->MakeReference();
         if (ref!=0) fIOSockets[n]->fSendQueue.Push(ref);
         if (callproc!=fIOSockets[n]) fIOSockets[n]->FireDataOutput();

         refcnt++;
      }

   if (refcnt==0) dabc::Buffer::Release(buf);

   FireOutput();

   return refcnt>0;
}

void mbs::ServerTransport::DropFrontBufferIfQueueFull()
{
   dabc::Buffer* buf = 0;

   {
      dabc::LockGuard guard(fMutex);
      buf = fOutQueue.Full() ? fOutQueue.Pop() : 0;
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
         if (fIOSockets.size()>0)
            fIOSockets[0]->FireNewBuffer();
         else
            testdrop = (Kind() == mbs::StreamServer);
      }
   }

   if (!res) EOUT(("Not able put buffer in the queue"));

   if (testdrop) DropFrontBufferIfQueueFull();

   return res;
}

unsigned mbs::ServerTransport::SendQueueSize()
{
   dabc::LockGuard guard(fMutex);

   return fOutQueue.Size();
}
