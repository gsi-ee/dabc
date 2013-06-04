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

dabc::SocketNetworkInetrface::SocketNetworkInetrface(int fd) :
   SocketIOAddon(fd, false, true),
   NetworkInetrface(),
   fHeaders(0),
   fSendQueue(),
   fRecvQueue(),
   fRecvStatus(0),
   fRecvRecid(0),
   fSendStatus(0),
   fSendRecid(0)
{
}

dabc::SocketNetworkInetrface::~SocketNetworkInetrface()
{
   delete [] fHeaders; fHeaders = 0;
   DOUT0("############ ~SocketNetworkInetrface() #####################");
}

void dabc::SocketNetworkInetrface::AllocateNet(unsigned fulloutputqueue, unsigned fullinputqueue)
{
   NetworkTransport* tr = (NetworkTransport*) fWorker();

   fHeaders = new char[tr->NumRecs() * tr->GetFullHeaderSize()];
   for (uint32_t n=0;n<tr->NumRecs();n++)
      tr->SetRecHeader(n, fHeaders + n * tr->GetFullHeaderSize());

   // 16 is default value, if required, it will be dynamically increased
   AllocateSendIOV(16);
   AllocateRecvIOV(16);

   fSendQueue.Allocate(fulloutputqueue); // +2 for sending and recv ackn
   fRecvQueue.Allocate(fullinputqueue);

   DOUT5("Create queues inp: %d out: %d", fullinputqueue, fulloutputqueue);
}


long dabc::SocketNetworkInetrface::Notify(const std::string& cmd, int arg)
{
   if (cmd == "GetNetworkTransportInetrface") return (long) ((NetworkInetrface*) this);

   return dabc::SocketIOAddon::Notify(cmd, arg);
}


void dabc::SocketNetworkInetrface::SubmitSend(uint32_t recid)
{
   fSendQueue.Push(recid);

   // we are in transport thread and can call event-processing methods directly
   if ((fSendQueue.Size()==1) && (fSendStatus==0)) OnSendCompleted();
}

void dabc::SocketNetworkInetrface::SubmitRecv(uint32_t recid)
{
   fRecvQueue.Push(recid);

   // we are in transport thread and can call event-processing methods directly
   if ((fRecvQueue.Size()==1) && (fRecvStatus==0)) OnRecvCompleted();
}


void dabc::SocketNetworkInetrface::OnSendCompleted()
{
   NetworkTransport* tr = (NetworkTransport*) fWorker();

   if (tr==0) {
      EOUT("Transport not available!!!");
      return;
   }

//   DOUT0("SocketNetworkInetrface::OnSendCompleted status %d ", fSendStatus);

   if (fSendStatus==1) {
      tr->ProcessSendCompl(fSendRecid);
      fSendRecid = 0;
      fSendStatus = 0;
   }

   // nothing to do, just wait for new submitted recv operation
   if (fSendQueue.Size() == 0) return;

   fSendRecid = fSendQueue.Pop();

   fSendStatus = 1;

   int sendtyp = tr->PackHeader(fSendRecid);

   if (sendtyp==0) {
      EOUT("record %u failed", fSendRecid);
      throw dabc::Exception("send record failed - should never happen");
   }

   NetworkTransport::NetIORec* rec = tr->GetRec(fSendRecid);

   if (rec==0) {
      EOUT("Completely wrong send recid %u", fSendRecid);
      exit(432);
   }

//   DOUT0("Start sending rec %u typ %d buf %u", fSendRecid, sendtyp, rec->buf.GetTotalSize());

   if (sendtyp == 1)
      StartSend(rec->header, tr->GetFullHeaderSize());
   else
      StartNetSend(rec->header, tr->GetFullHeaderSize(), rec->buf);

}

void dabc::SocketNetworkInetrface::OnRecvCompleted()
{
   NetworkTransport* tr = (NetworkTransport*) fWorker();

   if (tr==0) {
      EOUT("Transport not available!!!");
      return;
   }

do_compl:

   if (fRecvStatus==2) {
      // if we complete receiving of the buffer

      tr->ProcessRecvCompl(fRecvRecid);
      fRecvRecid = 0;
      fRecvStatus = 0;
   }

   if (fRecvStatus==1) {
      // analyze header, set new recv operation and so on

      fRecvStatus = 2;

      NetworkTransport::NetIORec* rec = tr->GetRec(fRecvRecid);

      if (rec==0) {
         EOUT("Completely wrong recv rec id %u", fRecvRecid);
         exit(75);
      }

      NetworkTransport::NetworkHeader* nethdr = (NetworkTransport::NetworkHeader*) rec->header;

      if (nethdr->typid == dabc::mbt_EOL) {
         DOUT1("Receive buffer with EOL bufsize = %u resthdr = %u",
                 nethdr->size, tr->GetFullHeaderSize() - sizeof(NetworkTransport::NetworkHeader));
      }

      if (nethdr->kind & NetworkTransport::netot_HdrSend) {
         goto do_compl;
      }

      if (nethdr->size > rec->buf.GetTotalSize()) {
         EOUT("Fatal - no buffer to receive data rec %d  sz1:%d sz2:%d",
                 fRecvRecid, nethdr->size, rec->buf.GetTotalSize());

         tr->CloseTransport(true);
         return;
      }

      if (!StartRecv(rec->buf, nethdr->size)) {
         EOUT("Cannot start recv - fatal error");
         tr->CloseTransport(true);
         return;
      }

      // DOUT0("Start receiving of buffer size %u", nethdr->size);
   } else {
      // nothing to do, just wait for new submitted recv operation
      if (fRecvQueue.Size() == 0) return;
      fRecvRecid = fRecvQueue.Pop();

      NetworkTransport::NetIORec* rec = tr->GetRec(fRecvRecid);

      if (rec==0) {
         EOUT("Completely wrong recv recid %u", fRecvRecid);
         exit(432);
      }

//      DOUT0("SocketNetworkInetrface::OnRecvCompleted start receiving socket %d thrd %s", Socket(), tr->ThreadName().c_str());

      if (IsDatagramSocket()) {
         fRecvStatus = 2;
         StartNetRecv(rec->header, tr->GetFullHeaderSize(), rec->buf, rec->buf.GetTotalSize());
      } else {
         fRecvStatus = 1;
         // do normal recv of the header data without evolving messages
         StartRecv(rec->header, tr->GetFullHeaderSize());
      }
   }
}

