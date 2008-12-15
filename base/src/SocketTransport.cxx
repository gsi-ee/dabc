#include "dabc/SocketTransport.h"

#include "dabc/SocketDevice.h"

dabc::SocketTransport::SocketTransport(SocketDevice* dev, Port* port, bool useackn, int fd) :
   NetworkTransport(dev),
   SocketIOProcessor(fd),
   fHeaders(0),
   fSendQueue(),
   fRecvQueue(),
   fRecvStatus(0),
   fRecvRecid(0),
   fSendStatus(0),
   fSendRecid(0)
{
   Init(port, useackn);

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
   Cleanup();

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
   DOUT2(("Connection closed"));

   ErrorCloseTransport();
}

void dabc::SocketTransport::OnSocketError(int errnum, const char* info)
{
   EOUT((" Connection error"));

   ErrorCloseTransport();
}

void dabc::SocketTransport::ErrorCloseTransport()
{
   fSendStatus = 0;
   fRecvStatus = 0;
   SetDoingNothing();

   dabc::NetworkTransport::ErrorCloseTransport();
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

      NetworkHeader* nethdr = (NetworkHeader*) fRecs[fRecvRecid].header;

      if (nethdr->typid == dabc::mbt_EOL) {
         DOUT1(("Receive buffer with EOL bufsize = %u resthdr = %u",
                 nethdr->size, fFullHeaderSize - sizeof(NetworkHeader)));
      }

      if (nethdr->kind & netot_HdrSend)
         goto do_compl;

      dabc::Buffer* buf = fRecs[fRecvRecid].buf;

      if ((buf==0) || (nethdr->size > buf->GetTotalSize())) {
         EOUT(("Fatal - no buffer to receive data rec %d  buf %p sz1:%d sz2:%d",
                 fRecvRecid, buf, nethdr->size, buf->GetTotalSize()));
         fPool->Print();
         exit(1);
      }

      void* hdr = 0;
      if (fFullHeaderSize > sizeof(NetworkHeader))
        hdr = (char*) fRecs[fRecvRecid].header + sizeof(NetworkHeader);

      if (!StartNetRecv(hdr, fFullHeaderSize - sizeof(NetworkHeader), buf, nethdr->size)) {
         EOUT(("Cannot start recv - fatal error"));
         exit(1);
      }
   } else {
      {
         LockGuard lock(fMutex);
         // nothing to do, just wait for new submitted recv operation
         if (fRecvQueue.Size() == 0) return;

         fRecvRecid = fRecvQueue.Pop();
      }

      fRecvStatus = 1;

      // do normal recv of the header data without evolving messages
      StartRecv(fRecs[fRecvRecid].header, sizeof(NetworkHeader), false);
   }
}

