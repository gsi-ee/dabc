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

#include "roc/UdpTransport.h"

#include "roc/UdpDevice.h"
#include "roc/Commands.h"
#include "roc/defines.h"
#include "roc/udpdefines.h"
#include "nxyter/Data.h"

#include "dabc/timing.h"
#include "dabc/Port.h"

#include <math.h>

const unsigned UDP_DATA_SIZE = ((sizeof(roc::UdpDataPacketFull) - sizeof(roc::UdpDataPacket)) / 6) * 6;

roc::UdpDataSocket::UdpDataSocket(UdpDevice* dev, int fd) :
   dabc::SocketIOProcessor(fd),
   dabc::Transport(dev),
   dabc::MemoryPoolRequester(),
   dabc::CommandClientBase(),
   fDev(dev),
   fQueueMutex(),
   fQueue(1),
   fReadyBuffers(0),
   fTransportBuffers(0),
   fResend()
{
   // we will react on all input packets
   SetDoingInput(true);

   fTgtBuf = 0;
   fTgtBufIndx = 0;
   fTgtShift = 0;
   fTgtPtr = 0;
   fTgtCheckGap = false;

   fTgtNextId = 0;
   fTgtTailId = 0;

   fBufferSize = 0;
   fTransferWindow = 10;

   rocNumber = 0;
   daqActive_ = false;
   daqState = daqInit;

   fTotalRecvPacket = 0;
   fTotalResubmPacket = 0;

   fFlashTimeout = .5;
   fLastDelivery = dabc::NullTimeStamp;
}

roc::UdpDataSocket::~UdpDataSocket()
{
   ResetDaq();

   if (fDev) fDev->fDataCh = 0;
   fDev = 0;
}

void roc::UdpDataSocket::ConfigureFor(dabc::Port* port)
{
   fQueue.Allocate(port->InputQueueCapacity());
   fBufferSize = port->GetCfgInt(dabc::xmlBufferSize, 0);
   fTransferWindow = port->GetCfgInt(roc::xmlTransferWindow, 40);
   fPool = port->GetMemoryPool();
   fReadyBuffers = 0;

   fTgtCheckGap = false;

   // one cannot have too much resend requests
   fResend.Allocate(port->InputQueueCapacity() * fBufferSize / UDP_DATA_SIZE);

   DOUT2(("Pool = %p buffer size = %u", fPool, fBufferSize));
}

void roc::UdpDataSocket::ProcessEvent(dabc::EventId evnt)
{
   switch (dabc::GetEventCode(evnt)) {
      case evntSocketRead: {
         void *tgt = fTgtPtr;
         if (tgt==0) tgt = fTempBuf;

         ssize_t len = DoRecvBufferHdr(&fTgtHdr, sizeof(UdpDataPacket),
                                        tgt, UDP_DATA_SIZE);
         if (len>0) {
            fTotalRecvPacket++;
//            DOUT0(("READ Packet %d len %d", ntohl(fTgtHdr.pktid), len));
            AddDataPacket(len, tgt);
         }

         break;
      }

      case evntStartTransport: {

         // no need to do anything when daq is already running
         if (daqActive_) return;

         // if start daq was performed by device,
         // one only need to start data taking

         if (daqState == daqPrepared) {
            daqState = daqStarting;
            daqActive_ = true;
            fLastDelivery = TimeStamp();
            AddBuffersToQueue();
            ActivateTimeout(0.0001);
            return;
         }

         ResetDaq();

         daqState = daqStarting;
         dabc::Command* cmd = new CmdPut(ROC_START_DAQ , 1);
         fDevice->Submit(Assign(cmd));
         break;
      }

      case evntStopTransport: {
         if (daqState == daqStopping) {
            ResetDaq();
            DOUT1(("Normal finish of daq"));
            break;
         }

         daqState = daqSuspending;

         fTgtBuf = 0;
         fTgtBufIndx = 0;
         fTgtShift = 0;
         fTgtPtr = 0;

         fLastDelivery = dabc::NullTimeStamp;


         dabc::Command* cmd = new CmdPut(ROC_STOP_DAQ, 1);
         fDevice->Submit(Assign(cmd));
         break;
      }

      case evntConfirmCmd: {
         if (dabc::GetEventArg(evnt) == 0) {
            ResetDaq();
            daqState = daqFails;
            ActivateTimeout(-1.);
         } else
         if (daqState == daqStarting) {
            daqActive_ = true;
            fLastDelivery = TimeStamp();
            AddBuffersToQueue();
            ActivateTimeout(0.0001);
         } else
         if (daqState == daqSuspending) {
           daqState = daqStopped;
           ActivateTimeout(-1.);
         }
         break;
      }

      case evntFillBuffer:
         AddBuffersToQueue();
         break;

      default:
         dabc::SocketIOProcessor::ProcessEvent(evnt);
   }
}

bool roc::UdpDataSocket::_ProcessReply(dabc::Command* cmd)
{
   bool res = cmd->GetResult();

   DOUT0(("Replyed cmd %s res = %s ", cmd->GetName(), DBOOL(res)));

   FireEvent(evntConfirmCmd, res ? 1 : 0);

   return false; // we do not need command object any longer
}

bool roc::UdpDataSocket::Recv(dabc::Buffer* &buf)
{
   buf = 0;
   {
      dabc::LockGuard lock(fQueueMutex);
      if (fReadyBuffers==0) return false;
      buf = fQueue.Pop();
      fReadyBuffers--;
   }
   FireEvent(evntFillBuffer);

   return buf!=0;
}

unsigned  roc::UdpDataSocket::RecvQueueSize() const
{
   dabc::LockGuard guard(fQueueMutex);

   return fReadyBuffers;
}

dabc::Buffer* roc::UdpDataSocket::RecvBuffer(unsigned indx) const
{
   dabc::LockGuard lock(fQueueMutex);

   if (indx>=fReadyBuffers) return 0;

   return fQueue.Item(indx);
}

bool roc::UdpDataSocket::ProcessPoolRequest()
{
   FireEvent(evntFillBuffer);
   return true;
}

void roc::UdpDataSocket::StartTransport()
{
   DOUT2(("Starting UDP transport "));

   FireEvent(evntStartTransport);
}

void roc::UdpDataSocket::StopTransport()
{
   DOUT2(("Stopping udp transport %ld", fTotalRecvPacket));

   FireEvent(evntStopTransport);
}

void roc::UdpDataSocket::AddBuffersToQueue(bool checkanyway)
{
   unsigned cnt = 0;

   {
      dabc::LockGuard lock(fQueueMutex);
      cnt = fQueue.Capacity() - fQueue.Size();
   }

   bool isanynew = false;

   while (cnt) {
      dabc::Buffer* buf = fPool->TakeBufferReq(this, fBufferSize);
      if (buf==0) break;

      fTransportBuffers++;
      if (fTgtBuf==0) {
         fTgtBuf = buf;
         fTgtShift = 0;
         fTgtPtr = (char*) buf->GetDataLocation();
      }

      isanynew = true;
      cnt--;
      dabc::LockGuard lock(fQueueMutex);
      fQueue.Push(buf);
   }

   if (isanynew || checkanyway) CheckNextRequest();
}

bool roc::UdpDataSocket::CheckNextRequest(bool check_retrans)
{
   UdpDataRequestFull req;
   double curr_tm = TimeStamp() * 1e-6;

   if (!daqActive_) return false;

   // send request each 0.2 sec,
   // if there is no replies on last request send it much faster - every 0.01 sec.
   bool dosend =
      fabs(curr_tm - lastRequestTm) > (lastRequestSeen ? 0.2 : 0.01);

   int can_send = 0;
   if (fTgtBuf) {
      can_send += (fBufferSize - fTgtShift) / UDP_DATA_SIZE;
      can_send += (fTransportBuffers - fTgtBufIndx - 1) * fBufferSize / UDP_DATA_SIZE;
   }

//   DOUT0(("TgtIndx %u Can_send %d ", fTgtBufIndx, can_send));

   if (can_send > (int) fTransferWindow) can_send = fTransferWindow;

   if (fResend.Size() >= fTransferWindow) can_send = 0; else
   if (can_send + fResend.Size() > fTransferWindow)
      can_send = fTransferWindow - fResend.Size();

   req.frontpktid = fTgtNextId + can_send;

   // if newly calculated front id bigger than last
   if ((req.frontpktid - lastSendFrontId) < 0x80000000) {

     if ((req.frontpktid - lastSendFrontId) >= fTransferWindow / 3) dosend = true;

   } else
      req.frontpktid = lastSendFrontId;

   req.tailpktid = fTgtTailId;

   req.numresend = 0;

   if (can_send==0) dosend = false;

   if (!check_retrans && !dosend) return false;

   for (unsigned n=0; n<fResend.Size(); n++) {
      ResendInfo* entry = fResend.ItemPtr(n);

      if ((entry->numtry>0) && (fabs(curr_tm - entry->lasttm)) < 0.1) continue;

      entry->lasttm = curr_tm;
      entry->numtry++;
      if (entry->numtry < 8) {
         req.resend[req.numresend++] = entry->pktid;

         dosend = true;

         if (req.numresend >= sizeof(req.resend) / 4) {
            EOUT(("Number of resends more than one can pack in the retransmit packet"));
            break;
         }

      } else {
         EOUT(("Roc:%u Drop pkt %u\n", rocNumber, entry->pktid));

         fTgtCheckGap = true;

         memset(entry->ptr, 0, UDP_DATA_SIZE);

         nxyter::Data* data = (nxyter::Data*) entry->ptr;
         data->setRocNumber(rocNumber);
         data->setMessageType(ROC_MSG_SYS);
         data->setSysMesType(5); // this is lost packet mark

         fResend.RemoveItem(n);
         n--;
      }

   }

   if (!dosend) return false;

   uint32_t pkt_size = sizeof(UdpDataRequest) + req.numresend * sizeof(uint32_t);

   // make request always 4 byte aligned
   while ((pkt_size < MAX_UDP_PAYLOAD) &&
          (pkt_size + UDP_PAYLOAD_OFFSET) % 4) pkt_size++;

   lastRequestTm = curr_tm;
   lastRequestSeen = false;
   lastSendFrontId = req.frontpktid;
   lastRequestId++;

//   DOUT1(("Send request id:%u  Range: 0x%04x - 0x%04x nresend:%d resend[0] = 0x%04x tgtbuf %p ptr %p tgtsize %u",
//         lastRequestId, req.tailpktid, req.frontpktid, req.numresend,
//         req.numresend > 0 ? req.resend[0] : 0, fTgtBuf, fTgtPtr, fTransportBuffers));

   req.password = htonl(ROC_PASSWORD);
   req.reqpktid = htonl(lastRequestId);
   req.frontpktid = htonl(req.frontpktid);
   req.tailpktid = htonl(req.tailpktid);
   for (uint32_t n=0; n < req.numresend; n++)
      req.resend[n] = htonl(req.resend[n]);
   req.numresend = htonl(req.numresend);

   DoSendBuffer(&req, pkt_size);

   return true;
}

double roc::UdpDataSocket::ProcessTimeout(double)
{
   if (!daqActive_) return -1;

   if (fTgtBuf == 0)
      AddBuffersToQueue(true);
   else
      CheckNextRequest();

   // check if we should flush current buffer
   if (!dabc::IsNullTime(fLastDelivery) &&
       (dabc::TimeDistance(fLastDelivery, TimeStamp()) > fFlashTimeout)) {
          DOUT0(("Doing flush"));
          CheckReadyBuffers(true);
   }

   return 0.01;
}


void roc::UdpDataSocket::ResetDaq()
{
   daqActive_ = false;
   daqState = daqInit;

   fTransportBuffers = 0;

   fTgtBuf = 0;
   fTgtBufIndx = 0;
   fTgtShift = 0;
   fTgtPtr = 0;

   fTgtNextId = 0;
   fTgtTailId = 0;
   fTgtCheckGap = false;

   lastRequestId = 0;
   lastSendFrontId = 0;
   lastRequestTm = 0.;
   lastRequestSeen = true;

   fResend.Reset();

   dabc::LockGuard lock(fQueueMutex);
   fQueue.Cleanup();
   fReadyBuffers = 0;

   fLastDelivery = dabc::NullTimeStamp;
}



void roc::UdpDataSocket::AddDataPacket(int len, void* tgt)
{
   uint32_t src_pktid = ntohl(fTgtHdr.pktid);

   if (tgt==0) {
      DOUT0(("Packet 0x%04x has no place buf %p bufindx %u queue %u ready %u", src_pktid, fTgtBuf, fTgtBufIndx, fQueue.Size(), fReadyBuffers));
      for (unsigned n=0;n < fResend.Size(); n++)
         DOUT0(("   Need resend 0x%04x retry %d", fResend.ItemPtr(n)->pktid, fResend.ItemPtr(n)->numtry));

      CheckNextRequest();

      return;
   }

   if (len <= (int) sizeof(UdpDataPacket)) {
      EOUT(("Too few data received %d", len));
      return;
   }

   if (ntohl(fTgtHdr.lastreqid) == lastRequestId) lastRequestSeen = true;

   int nummsgs = ntohl(fTgtHdr.nummsg);

   uint32_t gap = src_pktid - fTgtNextId;

   int data_len = nummsgs * 6;

//   DOUT0(("Packet id:0x%04x Head:0x%04x NumMsgs:%d ", src_pktid, fTgtNextId, nummsgs));

   bool packetaccepted = false;
   bool doflush = false;

   if ((fTgtPtr==tgt) && (gap < fBufferSize / UDP_DATA_SIZE * fTransportBuffers)) {

      if (gap>0) {
         // some packets are lost on the way, move pointer forward and
         // remember packets which should be resubmit
         void* src = fTgtPtr;

         while (fTgtNextId != src_pktid) {

            ResendInfo* info = fResend.PushEmpty();

            info->pktid = fTgtNextId;
            info->lasttm = 0.;
            info->numtry = 0;
            info->buf = fTgtBuf;
            info->bufindx = fTgtBufIndx;
            info->ptr = fTgtPtr;

            fTgtNextId++;
            fTgtShift += UDP_DATA_SIZE;
            fTgtPtr += UDP_DATA_SIZE;

            if (fTgtBuf->GetDataSize() - fTgtShift < UDP_DATA_SIZE) {
               fTgtBufIndx++;
               if (fTgtBufIndx >= fTransportBuffers) {
                  EOUT(("One get packet out of the available buffer spaces !!!!"));
                  return;
               }

               {
                  dabc::LockGuard lock(fQueueMutex);
                  fTgtBuf = fQueue.Item(fReadyBuffers + fTgtBufIndx);
               }

               fTgtPtr = (char*) fTgtBuf->GetDataLocation();
               fTgtShift = 0;
            }
         }

         // copy data which was received into the wrong place of the buffers
         memcpy(fTgtPtr, src, data_len);

//         DOUT1(("Copy pkt 0x%04x to buffer %p shift %u", src_pktid, fTgtBuf, fTgtShift));
      }

      // from here just normal situation when next packet is arrived

      if (fResend.Size()==0) fTgtTailId = fTgtNextId;

      fTgtNextId++;

      fTgtShift += data_len;
      fTgtPtr += data_len;

      if (fTgtBuf->GetDataSize() - fTgtShift < UDP_DATA_SIZE) {
         fTgtPtr = 0;
         fTgtBuf->SetDataSize(fTgtShift);
         fTgtShift = 0;
         fTgtBuf = 0;
         fTgtBufIndx++;
      }

      packetaccepted = true;

   } else {
      // this is retransmitted packet, may be received in temporary place
      for (unsigned n=0; n<fResend.Size(); n++) {
         ResendInfo* entry = fResend.ItemPtr(n);
         if (entry->pktid != src_pktid) continue;

         DOUT0(("Get retransmitted packet 0x%04x", src_pktid));

         fTotalResubmPacket++;

         memcpy(entry->ptr, tgt, data_len);
         if (data_len < (int) UDP_DATA_SIZE) {
            void* restptr = (char*) entry->ptr + data_len;
            memset(restptr, 0, UDP_DATA_SIZE - data_len);
            fTgtCheckGap = true;
         }

         fResend.RemoveItem(n);

         packetaccepted = true;

         break;
      }

   }

   if (!packetaccepted)
      EOUT(("Packet %p was not accepted, FLUSH?", src_pktid));

   // check incoming data for stop/start messages
   if (packetaccepted && (data_len>0) && (tgt!=0) && ((daqState == daqStarting) || (daqState == daqSuspending))) {
//      DOUT0(("Search special kind of message !!!"));

      nxyter::Data* data = (nxyter::Data*) tgt;
      int cnt = data_len / sizeof(nxyter::Data);
      for (;cnt; cnt--, data++) {
         // data->printData(7);
         if (data->isStartDaqMsg()) {
            // when we get first message, one should start counting delivery
            DOUT2(("Find start message"));
            if (daqState == daqStarting)
               daqState = daqRuns;
            else
               EOUT(("Start DAQ when not expected"));
         } else
         if (data->isStopDaqMsg()) {
            DOUT2(("Find stop message"));
            if (daqState == daqSuspending) {
               daqState = daqStopping;
            } else
               EOUT(("Stop DAQ when not expected"));
            // at that place one can close current buffer that it be delivered to user

            doflush = true;
          }
      }
   }

   CheckReadyBuffers(doflush);
}

void roc::UdpDataSocket::CompressBuffer(dabc::Buffer* buf)
{
   nxyter::Data* src = (nxyter::Data*) buf->GetDataLocation();
   unsigned numdata = buf->GetDataSize() / sizeof(nxyter::Data);
   unsigned numtgt(0);

   if (numdata==0) return;

   nxyter::Data* tgt = src;

//    int ddd(0), nops(0);

   while (numdata--) {
//      printf("%2d %5d %p ", ddd/243, ddd, src); src->printData(7); ddd++;
      if (src->isNopMsg()) {
         src++;
//         nops++;
      } else {
         if (tgt!=src) memcpy(tgt, src, sizeof(nxyter::Data));
         src++;
         tgt++;
         numtgt++;
      }
   }

   if (numtgt==0)
      EOUT(("Zero size after compress !!!"));

   buf->SetDataSize(numtgt*sizeof(nxyter::Data));

//   DOUT1(("Compress buffer %p numsgs %u nops %u", buf, numtgt, nops));
}

void roc::UdpDataSocket::CheckReadyBuffers(bool doflush)
{
   if (doflush && (fTgtBuf!=0) && (fTgtShift>0) && (fResend.Size()==0)) {
      DOUT2(("Flash buffer when recv of size %u", fTgtShift));
      fTgtPtr = 0;
      fTgtBuf->SetDataSize(fTgtShift);
      fTgtShift = 0;
      fTgtBuf = 0;
      fTgtBufIndx++;
   }

   if (fTgtBufIndx>0) {
      unsigned minindx = fTgtBufIndx;

      for (unsigned n=0; n<fResend.Size(); n++) {
         unsigned indx = fResend.ItemPtr(n)->bufindx;
         if (indx < minindx) minindx = indx;
      }

      if (minindx>0) {

         fTransportBuffers -= minindx;
         fTgtBufIndx -= minindx;
         for (unsigned n=0; n<fResend.Size(); n++)
            fResend.ItemPtr(n)->bufindx -= minindx;

         {
            dabc::LockGuard lock(fQueueMutex);

            // check all buffers on gaps, if necessary
            if (fTgtCheckGap)
               for (unsigned n=0;n<minindx;n++) {
                  dabc::Buffer* buf = fQueue.Item(fReadyBuffers + n);
                  CompressBuffer(buf);
               }

            fReadyBuffers += minindx;
         }

         while (minindx--) FireInput();

         fLastDelivery = TimeStamp();
      }
   }

   if ((fTgtBuf==0) && (fTgtBufIndx<fTransportBuffers)) {
      dabc::LockGuard lock(fQueueMutex);
      fTgtBuf = fQueue.Item(fReadyBuffers + fTgtBufIndx);
      fTgtPtr = (char*) fTgtBuf->GetDataLocation();
      fTgtShift = 0;
      // one can disable checks once we have no data in queues at all
      if ((fTgtBufIndx==0) && (fResend.Size()==0)) {
//         if (fTgtCheckGap) DOUT0(("!!! DISABLE COMPRESS !!!"));
         fTgtCheckGap = false;
      }
   }

   if (fTgtBuf == 0)
      AddBuffersToQueue();
   else
      CheckNextRequest();
}

int roc::UdpDataSocket::prepareForDaq()
{
   // if daq running, one should do nothing
   if (daqActive_) return 2;

   if ((daqState != daqInit) && (daqState != daqFails) && (daqState != daqPrepared)) {
      EOUT(("Cannot prepare daq at state %d", daqState));
      return 0;
   }

   if (daqState != daqPrepared) {
      ResetDaq();
      daqState = daqPrepared;
   }

   return 1;
}

bool roc::UdpDataSocket::prepareForSuspend()
{
   if (!daqActive_) return false;
   daqState = daqSuspending;
   return true;
}

bool roc::UdpDataSocket::prepareForStop()
{
   daqState = daqStopping;
   return true;
}
