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
#include "mbs/ClientTransport.h"

#include "dabc/Port.h"
#include "dabc/Manager.h"
#include "dabc/MemoryPool.h"
#include "dabc/Device.h"


mbs::ClientIOProcessor::ClientIOProcessor(ClientTransport* cl, int fd) :
   dabc::SocketIOProcessor(fd),
   fTransport(cl),
   fState(ioInit),
   fSwapping(false),
   fRecvBuffer(0)
{
   fServInfo.iStreams = 0; // by default, new format
}

mbs::ClientIOProcessor::~ClientIOProcessor()
{
   dabc::Buffer::Release(fRecvBuffer);
}

bool mbs::ClientIOProcessor::IsDabcEnabledOnMbsSide()
{
   return fServInfo.iStreams==0;
}

unsigned mbs::ClientIOProcessor::ReadBufferSize()
{
   uint32_t sz = fHeader.BufferLength();
   if (sz < sizeof(fHeader)) {
      EOUT(("Wrong buffer length %u", sz));
      return 0;
   }

   return sz - sizeof(mbs::BufferHeader);
}


void mbs::ClientIOProcessor::ProcessEvent(dabc::EventId evnt)
{
//   DOUT0(("ClientIOProcessor::Process event %d", dabc::GetEventCode(evnt)));

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

         if (fState == ioWaitBuffer){
            if (!fTransport->RequestBuffer(ReadBufferSize(), fRecvBuffer)) {
               fState = ioError;
               EOUT(("Cannot request buffer %u from memory pool", ReadBufferSize()));
            } else
            if (fRecvBuffer!=0) {
      //         DOUT1(("Start receiving of buffer %p %u %p len = %d",
      //               fRecvBuffer, fRecvBuffer->GetTotalSize(), fRecvBuffer->GetDataLocation(), ReadBufferSize()));

               fRecvBuffer->SetTypeId(mbs::mbt_MbsEvents);
	       
	       DOUT5(("MBS transport start recv %u", ReadBufferSize()));
	       
               StartRecv(fRecvBuffer, ReadBufferSize());
               fState = ioRecvBuffer;
            }
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
            fRecvBuffer->SetDataSize(fHeader.UsedBufferSize());
            if (fSwapping) mbs::SwapData(fRecvBuffer->GetDataLocation(), fRecvBuffer->GetDataSize());
	    fTransport->GetReadyBuffer(fRecvBuffer);
	 } else {
	    if (IsDabcEnabledOnMbsSide()) EOUT(("Empty buffer from mbs when dabc enabled?"));
	                             else DOUT3(("Keep alive buffer from MBS"));
	    dabc::Buffer::Release(fRecvBuffer);
	 }
	 
         
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


mbs::ClientTransport::ClientTransport(dabc::Port* port, int kind, int fd, const std::string& thrdname) :
   dabc::Transport(dabc::mgr()->FindLocalDevice()),
   dabc::MemoryPoolRequester(),
   fKind(kind),
   fIOProcessor(0),
   fMutex(),
   fInpQueue(port->InputQueueCapacity())
{
   fIOProcessor = new ClientIOProcessor(this, fd);

   dabc::mgr()->MakeThreadFor(fIOProcessor, thrdname.c_str());
}

mbs::ClientTransport::~ClientTransport()
{
   if (fIOProcessor!=0) {
      fIOProcessor->fTransport = 0;
      fIOProcessor->DestroyProcessor();
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

void mbs::ClientTransport::PortChanged()
{
   if (IsPortAssigned()) {
      dabc::LockGuard lock(fMutex);
      if (fIOProcessor) fIOProcessor->FireEvent(ClientIOProcessor::evRecvInfo);
   } else {
      fInpQueue.Cleanup(&fMutex);
      dabc::LockGuard lock(fMutex);
      if (fIOProcessor) fIOProcessor->FireEvent(ClientIOProcessor::evSendClose);
   }
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
   if (pool==0) {
      DOUT0(("Client port pool = null port = %p", fPort));
      return false;
   }

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
