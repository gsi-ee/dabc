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

#include "mbs/ServerTransport.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"
#include "dabc/Device.h"
#include "dabc/MemoryPool.h"

// ________________________________________________________________________

mbs::ServerIOProcessor::ServerIOProcessor(ServerTransport* tr, int fd) :
   dabc::SocketIOWorker(fd),
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
         DOUT0(("MBS Server send info block"));

         break;
      case ioSendingBuffer:
         fSendBuffers++;
         fState = ioReady;
         fSendQueue.Pop().Release();
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

      DeleteThis();

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

   dabc::SocketIOWorker::OnConnectionClosed();
}

void mbs::ServerIOProcessor::OnSocketError(int errnum, const char* info)
{

   DOUT0(("Connection to MBS client error"));

   if (fTransport) fTransport->SocketIOClosed(this);

   dabc::SocketIOWorker::OnSocketError(errnum, info);
}

double mbs::ServerIOProcessor::ProcessTimeout(double)
{
   return -1.;
}

void mbs::ServerIOProcessor::ProcessEvent(const dabc::EventId& evnt)
{
    if (evnt.GetCode() != evMbsDataOutput) {
       dabc::SocketIOWorker::ProcessEvent(evnt);
       return;
    }

    if (fTransport==0) return;

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

       dabc::Buffer& buf = fSendQueue.ItemRef(0);

       if (buf.GetTypeId()==mbt_MbsEvents) {
          fHeader.Init(true);
          fHeader.SetUsedBufferSize(buf.GetTotalSize());

          // error in evapi, must be + sizeof(mbs::BufferHeader)
          fHeader.SetFullSize(buf.GetTotalSize() - sizeof(mbs::BufferHeader));
          fState = ioSendingBuffer;

          StartNetSend(&fHeader, sizeof(fHeader), buf);
       } else {
          EOUT(("Buffer type %u not supported !!!", buf.GetTypeId()));
          fSendQueue.Pop().Release();
          fState = ioWaitingBuffer;
          FireDataOutput();
       }
    }

}

// _____________________________________________________________________

mbs::ServerTransport::ServerTransport(dabc::Reference port, int kind, int serversocket, int portnum,
                                      uint32_t maxbufsize, int scale, int maxclients) :
   dabc::SocketServerWorker(serversocket, portnum),
   dabc::Transport(port),
   fKind(kind),
   fMutex(),
   fOutQueue(GetPort()->OutputQueueCapacity()),
   fIOSockets(),
   fMaxBufferSize(1024),
   fScale(scale),
   fScaleCounter(0),
   fClientLimit(maxclients)
{
   RegisterTransportDependencies(this);

   while (fMaxBufferSize < maxbufsize) fMaxBufferSize*=2;
   if ((fScale<1) || (kind != mbs::StreamServer)) fScale = 1;

   if (fClientLimit>0) DOUT0(("Set client limit for MBS server to %d", fClientLimit));
}

mbs::ServerTransport::~ServerTransport()
{
   DOUT3(("mbs::ServerTransport::~ServerTransport queue:%d", fOutQueue.Size()));

   if (fOutQueue.Size()>0) {
      EOUT(("OUTPUT QUEUE WAS NOT EMPTIED"));
      fOutQueue.Cleanup();
   }

   if (fIOSockets.size()>0)
      EOUT(("CONNECTION SOCKETS ARE NOT DESTROYED"));

   DOUT3(("mbs::ServerTransport::~ServerTransport done queue:%d", fOutQueue.Size()));
}

void mbs::ServerTransport::OnClientConnected(int connfd)
{
   if (fIOSockets.size()>0) {
      if (Kind()== mbs::TransportServer)
         DOUT5(("The only connection meaningful for transport server, but lets try"));
      DOUT5(("There are %u connections opened, add one more", fIOSockets.size()));
   }

   if (fClientLimit>0) {
      int cnt=0;
      for (unsigned n=0;n<fIOSockets.size();n++)
         if (fIOSockets[n]!=0) cnt++;

      if (cnt>=fClientLimit) {
         DOUT0(("Size limit %d of clients number is achieved - reject connection", fClientLimit));
         close(connfd);
         return;
      }
   }


   ServerIOProcessor* io = new ServerIOProcessor(this, connfd);

   io->AssignToThread(thread());

   io->SendInfo(fMaxBufferSize + sizeof(mbs::BufferHeader), true);

   // replace empty place to avoid increasing of vector size
   for (unsigned n=0;n<fIOSockets.size();n++)
      if (fIOSockets[n]==0) {
         fIOSockets[n] = io;
         io = 0;
         break;
      }

   if (io) fIOSockets.push_back(io);

   DOUT1(("New client for fd:%d total %u", connfd, fIOSockets.size()));
}

void mbs::ServerTransport::ProcessEvent(const dabc::EventId& evnt)
{
   if (evnt.GetCode() == evntNewBuffer)
      MoveFrontBuffer(0);
   else
      dabc::SocketServerWorker::ProcessEvent(evnt);
}

void mbs::ServerTransport::CleanupFromTransport(dabc::Object* obj)
{
   if (obj == GetPool()) {
      fOutQueue.Cleanup(&fMutex);
   }

   dabc::Transport::CleanupFromTransport(obj);
}


void mbs::ServerTransport::CleanupTransport()
{
   fOutQueue.Cleanup(&fMutex);

   // FIXME: cleanup should be done much earlier
   for (unsigned n=0; n<fIOSockets.size();n++)
      if (fIOSockets[n]) {
         fIOSockets[n]->fTransport = 0;
         DOUT3(("Close I/O socket %p", fIOSockets[n]));
         // FIXME: should much earlier
         dabc::Object::Destroy(fIOSockets[n]);
         fIOSockets[n] = 0;
      }

   fIOSockets.clear();

   dabc::Transport::CleanupTransport();
}


void mbs::ServerTransport::SocketIOClosed(ServerIOProcessor* proc)
{
   for (unsigned n=0;n<fIOSockets.size();n++)
      if (fIOSockets[n]==proc) {
         fIOSockets.erase(fIOSockets.begin()+n);
         DOUT1(("Transport socket %p is closed", proc->Socket()));
         break;
      }

   FireEvent(evntNewBuffer);
}

void mbs::ServerTransport::MoveFrontBuffer(ServerIOProcessor* callproc)
{
   // method executed in IO sockets thread context, therefore no lock needed to access them

   do {

      bool isanyfull(false);

      for (unsigned n=0;n<fIOSockets.size();n++)
         if (fIOSockets[n]->fSendQueue.Full()) isanyfull = true;

      dabc::Buffer buf;
      bool fireout(false);

      {
         dabc::LockGuard guard(fMutex);
         // first check that we have buffers at all
         if (fOutQueue.Size()==0) return;

         // second, if some I/O socket has full output queue,
         // buffer will be taken only for stream server and when queue is full
         if (isanyfull && (!fOutQueue.Full() || (Kind() == mbs::TransportServer))) return;

         buf << fOutQueue.Pop();
         fireout = (Kind() == mbs::TransportServer);
      }

      // first count how may buffers we need
      unsigned refcnt(0);

      for (unsigned n=0;n<fIOSockets.size();n++)
         if (!fIOSockets[n]->fSendQueue.Full()) refcnt++;

      unsigned cnt(0);

      for (unsigned n=0;n<fIOSockets.size();n++)
         if (fIOSockets[n]->fSendQueue.Full()) {
            fIOSockets[n]->fDroppedBuffers++;
         } else {
            dabc::Buffer ref;
            if (++cnt == refcnt) ref << buf;
                           else  ref = buf.Duplicate();
            if (!ref.null()) fIOSockets[n]->fSendQueue.Push(ref);
            if (callproc!=fIOSockets[n]) fIOSockets[n]->FireDataOutput();
         }

      buf.Release();

      // we do not need port mutex while call is performed inside transport thread
      if (fireout) FirePortOutput();

     // if there are no any client connected, continue until output queue is cleaned up
   } while (fIOSockets.size()==0);
}



bool mbs::ServerTransport::Send(const dabc::Buffer& buf)
{
   bool res(false), fireout(false), firetransport(false);
   dabc::Buffer dropbuf;

   {
      dabc::LockGuard guard(fMutex);

      if (Kind() == mbs::StreamServer) {
         // in case of stream server accept any new buffer
         // if necessary, release oldest buffer immediately
         fScaleCounter = (fScaleCounter+1) % fScale;

         if (fScaleCounter!=0) {
            // buffer is just skipped
            dropbuf << buf;
         } else {
            if (fOutQueue.Full()) dropbuf << fOutQueue.Pop();
            fOutQueue.Push(buf);
            firetransport = true;
         }
         res = true;
         fireout = true;
      } else {
         if (!fOutQueue.Full()) {
            fOutQueue.Push(buf);
            res = true;
            firetransport = true;
         }
      }
   }

   // first release dropped buffer - it may be useful immediately
   dropbuf.Release();

   // in case of stream server say that we ready to get new data
   // we use mutex to ensure that transport thread will not clean port pointer before we complete
   if (fireout) FirePortOutput(true);

   // if buffer was placed into the queue, try move it inside the
   if (firetransport) FireEvent(evntNewBuffer);

   return res;
}

unsigned mbs::ServerTransport::SendQueueSize()
{
   dabc::LockGuard guard(fMutex);

   return fOutQueue.Size();
}

dabc::Buffer& mbs::ServerTransport::RecvBuffer(unsigned) const
{
   throw dabc::Exception("ServerTransport::RecvBuffer cannot be implemented");
}

