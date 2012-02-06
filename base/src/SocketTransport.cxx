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

#include "dabc/SocketTransport.h"

#include "dabc/SocketDevice.h"

dabc::SocketTransport::SocketTransport(Reference port, bool useackn, int fd, bool isdatagram) :
   SocketIOWorker(fd, isdatagram, true),
   NetworkTransport(port, useackn),
   fHeaders(0),
   fSendQueue(),
   fRecvQueue(),
   fRecvStatus(0),
   fRecvRecid(0),
   fSendStatus(0),
   fSendRecid(0)
{
   InitNetworkTransport(this);

   DOUT3(("Transport %p for port %p created", this, GetPort()));

   fHeaders = new char[fFullHeaderSize*fNumRecs];
   for (uint32_t n=0;n<fNumRecs;n++)
      SetRecHeader(n, fHeaders + n*fFullHeaderSize);

   // 16 is default value, if required, it will be dynamically increased
   AllocateSendIOV(16);
   AllocateRecvIOV(16);

   fSendQueue.Allocate(fOutputQueueLength+2); // +2 for sending and recv ackn
   fRecvQueue.Allocate(fInputQueueLength+2);

   DOUT5(("Create queues inp: %d out: %d", fInputQueueLength, fOutputQueueLength));

//   SetLogging(true);
}

dabc::SocketTransport::~SocketTransport()
{
   // FIXME: should it go in cleanup ???

   DOUT2(("@@@@@@@@@@@@@@@@@@@ SOCKET TRANSPORT: %p %s destructor @@@@@@@@@@@@@@@@@@@@@@@@@", this, GetName()));

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
   fRecvQueue.Push(recid);

//   if (IsLogging())
//      DOUT0(("_SubmitRecv %u queue %u", recid, fRecvQueue.Size()));

   if (fRecvQueue.Size()==1) FireEvent(evntActivateRecv);
}

void dabc::SocketTransport::CloseTransport(bool witherr)
{
   DOUT2(("SOCKET TRANSPORT: %p %s WorkerId=%u CloseTransport witherr %s", this, GetName(), fWorkerId, DBOOL(witherr)));

   CancelIOOperations();

   dabc::NetworkTransport::CloseTransport(witherr);
}

void dabc::SocketTransport::CleanupTransport()
{
   DOUT2(("SOCKET TRANSPORT: %p %s CleanupTransport WorkerId=%u", this, GetName(), fWorkerId));

   CancelIOOperations();

   dabc::NetworkTransport::CleanupTransport();
}


void dabc::SocketTransport::OnConnectionClosed()
{
   DOUT2(("Connection closed - close transport"));

   CloseTransport(true);
}

void dabc::SocketTransport::OnSocketError(int errnum, const char* info)
{
   EOUT(("tr %p Connection error socket %d errnum %d info %s", this, fSocket, errnum, info ));

   CloseTransport(true);
}

bool dabc::SocketTransport::ProcessPoolRequest()
{
   FireEvent(evntActiavtePool);

   return true;
}


void dabc::SocketTransport::ProcessEvent(const EventId& evnt)
{
   switch (evnt.GetCode()) {
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
          dabc::SocketIOWorker::ProcessEvent(evnt);
   }
}

void dabc::SocketTransport::OnSendCompleted()
{
   if (!IsTransportWorking()) {
      EOUT(("!!!!!!!!!!! SendCompleted EVENT when transport no longer working !!!!!!!!!!!! "));
      return;
   }

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

   int sendtyp = PackHeader(fSendRecid);

   if (sendtyp==0) {
      EOUT(("record failed ", fSendRecid));
      throw dabc::Exception(("send record failed - should never happen"));
   }

   if (sendtyp == 1)
      StartSend(fRecs[fSendRecid].header, fFullHeaderSize);
   else
      StartNetSend(fRecs[fSendRecid].header, fFullHeaderSize, fRecs[fSendRecid].buf);
}

void dabc::SocketTransport::OnRecvCompleted()
{
   if (!IsTransportWorking()) {
      EOUT(("!!!!!!!!!!! RecvCompleted EVENT when transport no longer working !!!!!!!!!!!! "));
      return;
   }

//   if (IsLogging())
//      DOUT0(("Socket transport complete rec %u status %d ", fRecvRecid, fRecvStatus));

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

      if (nethdr->kind & netot_HdrSend) {
         DOUT0(("REDO"));
         goto do_compl;
      }

      if (nethdr->size > fRecs[fRecvRecid].buf.GetTotalSize()) {
         EOUT(("Fatal - no buffer to receive data rec %d  sz1:%d sz2:%d pool:%s",
                 fRecvRecid, nethdr->size, fRecs[fRecvRecid].buf.GetTotalSize(), GetPool()->GetName()));

         CloseTransport(true);
         return;
      }

      if (!StartRecv(fRecs[fRecvRecid].buf, nethdr->size)) {
         EOUT(("Cannot start recv - fatal error"));
         CloseTransport(true);
         return;
      }
   } else {
      {
         LockGuard lock(fMutex);
         // nothing to do, just wait for new submitted recv operation
         if (fRecvQueue.Size() == 0) return;

         fRecvRecid = fRecvQueue.Pop();
      }

//      if (IsLogging())
//         DOUT0(("SOCKET TRANSPORT: Start receiving socket %d of rec %u datagramm %s ", Socket(), fRecvRecid, DBOOL(IsDatagramSocket())));

      if (IsDatagramSocket()) {
         fRecvStatus = 2;

         StartNetRecv(fRecs[fRecvRecid].header, fFullHeaderSize, fRecs[fRecvRecid].buf, fRecs[fRecvRecid].buf.GetTotalSize());

      } else {
         fRecvStatus = 1;
         // do normal recv of the header data without evolving messages
         StartRecv(fRecs[fRecvRecid].header, fFullHeaderSize);
      }
   }
}

