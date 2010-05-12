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
#include "dabc/SocketTransport.h"

#include "dabc/SocketDevice.h"

dabc::SocketTransport::SocketTransport(SocketDevice* dev, Port* port, bool useackn, int fd, bool isdatagram) :
   NetworkTransport(dev),
   SocketIOProcessor(fd),
   fIsDatagram(isdatagram),
   fHeaders(0),
   fSendQueue(),
   fRecvQueue(),
   fRecvStatus(0),
   fRecvRecid(0),
   fSendStatus(0),
   fSendRecid(0)
{
   InitNetworkTransport(port, useackn);

   DOUT3(("Transport %p for port %p created", this, port));

   fHeaders = new char[fFullHeaderSize*fNumRecs];
   for (uint32_t n=0;n<fNumRecs;n++)
      SetRecHeader(n, fHeaders + n*fFullHeaderSize);

   // 16 is default value, if required, it will be dynamically increased
   AllocateSendIOV(16);
   AllocateRecvIOV(16);

   fSendQueue.Allocate(fOutputQueueLength+2); // +2 for sending and recv ackn
   fRecvQueue.Allocate(fInputQueueLength+2);

   DOUT5(("Create queues inp: %d out: %d", fInputQueueLength, fOutputQueueLength));
}

dabc::SocketTransport::~SocketTransport()
{
   CleanupNetworkTransport();

   delete [] fHeaders; fHeaders = 0;
}

void dabc::SocketTransport::_SubmitSend(uint32_t recid)
{
   DOUT5(("_SubmitSend %u", recid));

   fSendQueue.Push(recid);
   if (fSendQueue.Size()==1) FireEvent(evntActivateSend);
}

void dabc::SocketTransport::_SubmitRecv(uint32_t recid)
{
   DOUT5(("_SubmitRecv %u", recid));

   fRecvQueue.Push(recid);
   if (fRecvQueue.Size()==1) FireEvent(evntActivateRecv);
}

void dabc::SocketTransport::OnConnectionClosed()
{
   DOUT2(("Connection closed - close transport"));

   ErrorCloseTransport();
}

void dabc::SocketTransport::OnSocketError(int errnum, const char* info)
{
   EOUT(("tr %p Connection error socket %d errnum %d info %s", this, fSocket, errnum, info ));

   ErrorCloseTransport();
}


void dabc::SocketTransport::HaltTransport()
{
   fSendStatus = 0;
   fRecvStatus = 0;

   HaltProcessor();

   RemoveProcessorFromThread(true);
}


bool dabc::SocketTransport::ProcessPoolRequest()
{
   FireEvent(evntActiavtePool);

   return true;
}


void dabc::SocketTransport::ProcessEvent(dabc::EventId evnt)
{
   switch (GetEventCode(evnt)) {
       case evntActivateSend: {
          if (fSendStatus==0) OnSendCompleted();
          break;
       }

       case evntActivateRecv: {
          if (fRecvStatus==0) OnRecvCompleted();
          break;
       }

       case evntActiavtePool:
          FillRecvQueue();
          break;

       default:
          dabc::SocketIOProcessor::ProcessEvent(evnt);
   }
}

void dabc::SocketTransport::OnSendCompleted()
{
   if (fSendStatus==1) {
      ProcessSendCompl(fSendRecid);
      fSendRecid = 0;
      fSendStatus = 0;
   }

   {
      LockGuard lock(fMutex);
      // nothing to do, just wait for new submitted recv operation
      if (fSendQueue.Size() == 0) return;

      fSendRecid = fSendQueue.Pop();
   }

   fSendStatus = 1;

   int hdrsize = PackHeader(fSendRecid);

//   DOUT1(("Start sending of rec %u", fSendRecid));

   if (fRecs[fSendRecid].buf!=0)
      StartNetSend(fRecs[fSendRecid].header, hdrsize, fRecs[fSendRecid].buf);
   else
      StartSend(fRecs[fSendRecid].header, hdrsize, false);
}

void dabc::SocketTransport::OnRecvCompleted()
{

do_compl:

   if (fRecvStatus==2) {
      // if we complete receiving of the buffer
      ProcessRecvCompl(fRecvRecid);
      fRecvRecid = 0;
      fRecvStatus = 0;
   }

   if (fRecvStatus==1) {
      // analyze header, set new recv operation and so on

      fRecvStatus = 2;

      if ((fRecs==0) || (fRecvRecid>fNumRecs)) {
         EOUT(("Completely wrong recs = %p num = %u id = %u", fRecs, fNumRecs, fRecvRecid));
         exit(75);
      }

      NetworkHeader* nethdr = (NetworkHeader*) fRecs[fRecvRecid].header;

      if (nethdr->typid == dabc::mbt_EOL) {
         DOUT1(("Receive buffer with EOL bufsize = %u resthdr = %u",
                 nethdr->size, fFullHeaderSize - sizeof(NetworkHeader)));
      }

      if (nethdr->kind & netot_HdrSend)
         goto do_compl;

      dabc::Buffer* buf = fRecs[fRecvRecid].buf;

      if ((buf==0) || (nethdr->size > buf->GetTotalSize())) {
         EOUT(("Fatal - no buffer to receive data rec %d  buf %p sz1:%d sz2:%d pool:%s",
                 fRecvRecid, buf, nethdr->size, buf->GetTotalSize(), fPool->GetName()));

         ErrorCloseTransport();
         return;
         // fPool->Print();
         // exit(110);
      }

      void* hdr = 0;
      if (fFullHeaderSize > sizeof(NetworkHeader))
        hdr = (char*) fRecs[fRecvRecid].header + sizeof(NetworkHeader);

      if (!StartNetRecv(hdr, fFullHeaderSize - sizeof(NetworkHeader), buf, nethdr->size)) {
         EOUT(("Cannot start recv - fatal error"));
         ErrorCloseTransport();
         return;
      }
   } else {
      {
         LockGuard lock(fMutex);
         // nothing to do, just wait for new submitted recv operation
         if (fRecvQueue.Size() == 0) return;

         fRecvRecid = fRecvQueue.Pop();
      }

      if (fIsDatagram) {
         fRecvStatus = 2;

         dabc::Buffer* buf = fRecs[fRecvRecid].buf;

         StartNetRecv(fRecs[fRecvRecid].header, fFullHeaderSize, buf, buf->GetTotalSize(), true, true);

      } else {
         fRecvStatus = 1;
         // do normal recv of the header data without evolving messages
         StartRecv(fRecs[fRecvRecid].header, sizeof(NetworkHeader), false);
      }
   }
}

