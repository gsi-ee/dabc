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

#include "mbs/ClientTransport.h"

#include "dabc/Port.h"
#include "dabc/Manager.h"
#include "dabc/MemoryPool.h"
#include "dabc/Device.h"
#include "dabc/Pointer.h"


mbs::ClientTransport::ClientTransport(dabc::Reference port, int kind, int fd) :
   dabc::SocketIOWorker(fd),
   dabc::Transport(port),
   dabc::MemoryPoolRequester(),
   fState(ioInit),
   fSwapping(false),
   fRecvBuffer(),
   fKind(kind),
   fMutex(),
   fInpQueue(GetPort()->InputQueueCapacity())
{
   fServInfo.iStreams = 0; // by default, new format

   RegisterTransportDependencies(this);
}

mbs::ClientTransport::~ClientTransport()
{
   // FIXME: cleanup should be done much earlier

   DOUT3(("Destroy mbs::ClientTransport::~ClientTransport()"));

   fRecvBuffer.Release();
}

void mbs::ClientTransport::CleanupTransport()
{
   fInpQueue.Cleanup(&fMutex);
   fRecvBuffer.Release();

   // FIXME: check that close is really send to the server
   FireEvent(evSendClose);

   dabc::Transport::CleanupTransport();
}

void mbs::ClientTransport::CleanupFromTransport(dabc::Object* obj)
{
   if (obj == GetPool()) {
      fInpQueue.Cleanup(&fMutex);
   }

   dabc::Transport::CleanupFromTransport(obj);
}

bool mbs::ClientTransport::IsDabcEnabledOnMbsSide()
{
   return fServInfo.iStreams==0;
}

unsigned mbs::ClientTransport::ReadBufferSize()
{
   uint32_t sz = fHeader.BufferLength();
   if (sz < sizeof(fHeader)) {
      EOUT(("Wrong buffer length %u", sz));
      return 0;
   }

   return sz - sizeof(mbs::BufferHeader);
}


void mbs::ClientTransport::ProcessEvent(const dabc::EventId& evnt)
{
//   DOUT0(("ClientTransport::ProcessEvent event %d", evnt.GetCode()));

   switch (evnt.GetCode()) {

      case evReactivate:
         // event appears from transport when it sees first empty place in the queue
         if (fState != ioReady) return;

      case evDataInput:
         if (fState == ioReady) {
            if (!HasPlaceInQueue()) return;
            if (Kind() == mbs::StreamServer) {
               strcpy(fSendBuf, "GETEVT");
               StartSend(fSendBuf, 12);
            }

            StartRecv(&fHeader, sizeof(fHeader));
            fState = ioRecvHeder;
         }

         if (fState == ioWaitBuffer){
            if (!RequestBuffer(ReadBufferSize(), fRecvBuffer)) {
               fState = ioError;
               EOUT(("Cannot request buffer %u from memory pool", ReadBufferSize()));
            } else
               if (!fRecvBuffer.null()) {
                  //         DOUT1(("Start receiving of buffer %p %u %p len = %d",
                  //               fRecvBuffer, fRecvBuffer->GetTotalSize(), fRecvBuffer->GetDataLocation(), ReadBufferSize()));

                  fRecvBuffer.SetTypeId(mbs::mbt_MbsEvents);

                  DOUT5(("MBS transport start recv %u", ReadBufferSize()));

                  StartRecv(fRecvBuffer, ReadBufferSize());
                  fState = ioRecvBuffer;
               }
         }
         if (fState == ioError) {
            EOUT(("Process error"));
            CloseTransport(true);
         }
         break;

      case evRecvInfo:
         if (fState != ioInit) {
            EOUT(("Not an initial state to receive info from server"));
         } else {
            StartRecv(&fServInfo, sizeof(fServInfo));
            fState = ioRecvInfo;
         }
         break;

      case evSendClose:
         DOUT3(("Send close"));
         strcpy(fSendBuf, "CLOSE");
         StartSend(fSendBuf, 12);
         fState = ioClosing;
         break;

      default:
         dabc::SocketIOWorker::ProcessEvent(evnt);
   }
}

void mbs::ClientTransport::OnSendCompleted()
{
   if ((fState != ioRecvHeder) && (fState != ioClosing)) {
      EOUT(("Complete sending at wrong moment"));
      fState = ioError;
      FireEvent(evDataInput);
   }
}

void mbs::ClientTransport::OnRecvCompleted()
{
   switch (fState) {
      case ioRecvInfo:
         fState = ioReady;
         if (fServInfo.iEndian != 1) {
            mbs::SwapData(&fServInfo, sizeof(fServInfo));
            fSwapping = true;
         }

         if (fServInfo.iEndian != 1) {
            EOUT(("Cannot correctly define server endian"));
            fState = ioError;
         }

         if ((fState != ioError) && (fServInfo.iStreams != 0) && (fServInfo.iBuffers != 1)) {
            EOUT(("Number of buffers %u per stream bigger than 1", fServInfo.iBuffers));
            EOUT(("This will lead to event spanning which is not supported by DABC"));
            EOUT(("Set buffers number to 1 or call \"enable dabc\" on mbs side"));
            fState=ioError;
         }

         if (fState!=ioError) {
            std::string info = "new format";
            if (fServInfo.iStreams > 0) dabc::formats(info, "streams = %u", fServInfo.iStreams);

            DOUT1(("Get MBS server info: %s, buf_per_stream = %u, swap = %s ",
                  info.c_str(), fServInfo.iBuffers, DBOOL(fSwapping)));
         }

         FireEvent(evDataInput);

         break;
      case ioRecvHeder:

         if (fSwapping) mbs::SwapData(&fHeader, sizeof(fHeader));

         DOUT5(("MbsClient:: Header received, size %u, rest size = %u used %u", fHeader.BufferLength(), ReadBufferSize(), fHeader.UsedBufferSize()));

         if (ReadBufferSize() > (unsigned) fServInfo.iMaxBytes) {
            EOUT(("Buffer size %u bigger than allowed by info record %d", ReadBufferSize(), fServInfo.iMaxBytes));
            fState = ioError;
         } else
         if (ReadBufferSize() == 0) {
            fState = ioReady;
            DOUT3(("Keep alive buffer from MBS side"));
         } else {
            fState = ioWaitBuffer;
         }

         FireEvent(evDataInput);

         break;

      case ioRecvBuffer:

//         DOUT1(("Provide recv buffer %p to transport", fRecvBuffer));

         if (fHeader.UsedBufferSize()>0) {
            fRecvBuffer.SetTotalSize(fHeader.UsedBufferSize());
            if (fSwapping) {
               dabc::Pointer ptr(fRecvBuffer);
               while (!ptr.null()) {
                  mbs::SwapData(ptr(), ptr.rawsize());
                  ptr.shift(ptr.rawsize());
               }
            }

            // DOUT5(("Recv data into SEG:%u size %u", fRecvBuffer.SegmentId(0), fRecvBuffer.GetTotalSize()));
            fInpQueue.Push(fRecvBuffer, &fMutex);
            // DOUT0(("After push buffer %u", fRecvBuffer.GetTotalSize()));
            FirePortInput();
         } else {
            if (IsDabcEnabledOnMbsSide()) EOUT(("Empty buffer from mbs when dabc enabled?"));
                                     else DOUT3(("Keep alive buffer from MBS"));
         }

         // ensure that buffer is released
         fRecvBuffer.Release();

         fState = ioReady;

         FireEvent(evDataInput);

         break;
      default:
         EOUT(("One should not complete recv in such state %d", fState));
         return;
   }
}

void mbs::ClientTransport::OnConnectionClosed()
{
   DOUT0(("Close client connection"));

   // from this moment we have nothing to do and can close transport
   CloseTransport();

   dabc::SocketIOWorker::OnConnectionClosed();
}


bool mbs::ClientTransport::ProcessPoolRequest()
{
   FireEvent(evDataInput);

   return true;
}

void mbs::ClientTransport::PortAssigned()
{
   FireEvent(evRecvInfo);
}

void mbs::ClientTransport::StartTransport()
{
   // call performed from port thread, one should protect our data
   {
      dabc::LockGuard lock(fMutex);
      fRunning = true;
   }
   FireEvent(evReactivate);
}

void mbs::ClientTransport::StopTransport()
{
   dabc::LockGuard lock(fMutex);
   fRunning = false;
}

bool mbs::ClientTransport::Recv(dabc::Buffer &buf)
{
   bool dofire = false;

   {
      dabc::LockGuard lock(fMutex);
      dofire = fInpQueue.Full();
      fInpQueue.PopBuffer(buf);
   }

   if (dofire) FireEvent(evReactivate);

   return !buf.null();
}

unsigned mbs::ClientTransport::RecvQueueSize() const
{
   dabc::LockGuard guard(fMutex);

   return fInpQueue.Size();
}

dabc::Buffer& mbs::ClientTransport::RecvBuffer(unsigned indx) const
{
   dabc::LockGuard lock(fMutex);

   return fInpQueue.ItemRef(indx);
}

bool mbs::ClientTransport::HasPlaceInQueue()
{
   dabc::LockGuard lock(fMutex);
   return fRunning && !fInpQueue.Full();
}

bool mbs::ClientTransport::RequestBuffer(uint32_t sz, dabc::Buffer &buf)
{
   if (GetPool()==0) {
      DOUT0(("Client port pool = null port = %p", GetPort()));
      return false;
   }

   buf = GetPool()->TakeBufferReq(this, sz);

   return true;
}
