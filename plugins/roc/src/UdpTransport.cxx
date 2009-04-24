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
#include "roc/Defines.h"
#include "nxyter/Data.h"

#include "dabc/timing.h"
#include "dabc/Port.h"

#include <math.h>

#define UDP_DATA_SIZE (sizeof(UdpDataPacketFull) - sizeof(UdpDataPacket))

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

   DOUT0(("Pool = %p buffer size = %u", fPool, fBufferSize));
}

void roc::UdpDataSocket::ProcessEvent(dabc::EventId evnt)
{
   switch (dabc::GetEventCode(evnt)) {
      case evntSocketRead: {
         ssize_t len = DoRecvBufferHdr(&fTgtHdr, sizeof(UdpDataPacket),
                                        fTgtPtr, UDP_DATA_SIZE);
         if (len>0) {
            fTotalRecvPacket++;
            DOUT5(("READ Packet %d len %d", ntohl(fTgtHdr.pktid), len));
            AddDataPacket(len);
         }

         break;
      }

      case evntStartDaq: {
         ResetDaq();

         daqState = daqStarting;
         dabc::Command* cmd = new CmdPoke(ROC_START_DAQ , 1);
         fDevice->Submit(Assign(cmd));
         break;
      }

      case evntStopDaq: {
         daqState = daqStopping;

         fTgtBuf = 0;
         fTgtBufIndx = 0;
         fTgtShift = 0;
         fTgtPtr = 0;

         dabc::Command* cmd = new CmdPoke(ROC_STOP_DAQ, 1);
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

            AddBuffersToQueue();

            ActivateTimeout(0.0001);
         } else
         if (daqState == daqStopping) {
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
   DOUT0(("Starting UDP transport "));

   FireEvent(evntStartDaq);
}

void roc::UdpDataSocket::StopTransport()
{
   FireEvent(evntStopDaq);
}

void roc::UdpDataSocket::AddBuffersToQueue()
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

   if (isanynew) CheckNextRequest();
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
      if (fTransportBuffers > 0)
         can_send += (fTransportBuffers - 1) * fBufferSize / UDP_DATA_SIZE;
   }

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

//   if (pkt->numresend)
//      DOUT(("Request:%u request front:%u tail:%u resubm:%u\n", lastRequestId, pkt->frontpktid, pkt->tailpktid, pkt->numresend));

   DOUT1(("Send request id:%u  Range: %u - %u nresend:%d resend[0] = %d tgtbuf %p ptr %p tgtsize %u",
         lastRequestId, req.tailpktid, req.frontpktid, req.numresend,
         req.numresend > 0 ? req.resend[0] : -1, fTgtBuf, fTgtPtr, fTransportBuffers));

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
      AddBuffersToQueue();
   else
      CheckNextRequest();

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

}



void roc::UdpDataSocket::AddDataPacket(int len)
{
   if (fTgtPtr==0) {
      EOUT(("Packet received without having place for it!"));
      return;
   }

   if (len <= (int) sizeof(UdpDataPacket)) {
      EOUT(("Too few data received %d", len));
      return;
   }

   uint32_t src_pktid = ntohl(fTgtHdr.pktid);

   DOUT0(("Packet id:%05u Head:%05u", src_pktid, fTgtNextId));

   if (ntohl(fTgtHdr.lastreqid) == lastRequestId) lastRequestSeen = true;

   uint32_t gap = src_pktid - fTgtNextId;

   int data_len = len - sizeof(UdpDataPacket);

   bool checkready = false;

   if (gap < fBufferSize / UDP_DATA_SIZE * fTransportBuffers) {

      if (gap>0) {
         // some packets are lost on the way, move pointer forward and
         // remember packets which should be resubmit
         void* src = fTgtPtr;

         while (fTgtNextId != src_pktid) {

            ResendInfo* info = fResend.PushEmpty();

            info->pktid = fTgtNextId;
            info->addtm = 0.;
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
         checkready = true;
      }
   } else {
      // this is retransmitted packet
      for (unsigned n=0; n<fResend.Size(); n++) {
         ResendInfo* entry = fResend.ItemPtr(n);
         if (entry->pktid != src_pktid) continue;

         fTotalResubmPacket++;

         memcpy(entry->ptr, fTgtPtr, data_len);
         if (data_len < (int) UDP_DATA_SIZE) {
            void* restptr = (char*) entry->ptr + data_len;
            memset(restptr, 0, UDP_DATA_SIZE - data_len);
            fTgtCheckGap = true;
         }

         fResend.RemoveItem(n);

         checkready = true;

         break;
      }
   }

   if (checkready && (fTgtBufIndx>0)) {
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
      }
   }

   if ((fTgtBuf==0) && (fTgtBufIndx<fTransportBuffers)) {
      dabc::LockGuard lock(fQueueMutex);
      fTgtBuf = fQueue.Item(fReadyBuffers + fTgtBufIndx);
      fTgtPtr = (char*) fTgtBuf->GetDataLocation();
      fTgtShift = 0;
      // one can disable checks once we have no data in queues at all
      if ((fTgtBufIndx==0) && (fResend.Size()==0)) {
         if (fTgtCheckGap) DOUT0(("!!! DISABLE COMPRESS !!!"));
         fTgtCheckGap = false;
      }
   }

   if (fTgtBuf == 0)
      AddBuffersToQueue();
   else
      CheckNextRequest();
}

void roc::UdpDataSocket::CompressBuffer(dabc::Buffer* buf)
{
   EOUT(("Implement buffer compress !!!!!!!"));
}


/*


bool roc::UdpDataSocket::KnutstartDaq(unsigned trWin)
{
   ResetDaq();
   daqActive_ = true;
   daqState = daqStarting;

   bool res = true;

   {
      daqActive_ = true;
      daqState = daqStarting;
      transferWindow = trWin < 4 ? 4 : trWin;

      ringCapacity = trWin * 10;
      if (ringCapacity<100) ringCapacity = 100;

      if (ringCapacity * MESSAGES_PER_PACKET * 6 < fBufferSize * 2)
         ringCapacity = 2 * fBufferSize / (MESSAGES_PER_PACKET * 6);

      dataRequired_ = fBufferSize / 6;

      ringBuffer = new UdpDataPacketFull[ringCapacity];
      ringHead = 0;
      ringHeadId = 0;
      ringTail = 0;
      ringSize = 0;

      if (ringBuffer==0) {
         EOUT(("Cannot allocate ring buffer\n"));
         res = false;
      } else
      for (unsigned n=0;n<ringCapacity;n++)
         ringBuffer[n].lastreqid = 0; // indicate that entry is invalid
   }

//   if (res)
//      res = poke(ROC_START_DAQ , 1) == 0;

   if (!res) {
      EOUT(("Fail to start DAQ\n"));
//      dabc::LockGuard guard(dataMutex_);
      daqActive_ = false;
      daqState = daqFails;
   }

   return res;
}

void roc::UdpDataSocket::KnutsendDataRequest(UdpDataRequestFull* pkt)
{
   uint32_t pkt_size = sizeof(UdpDataRequest) + pkt->numresend * sizeof(uint32_t);

   // make request always 4 byte aligned
   while ((pkt_size < MAX_UDP_PAYLOAD) &&
          (pkt_size + UDP_PAYLOAD_OFFSET) % 4) pkt_size++;

   lastRequestTm = TimeStamp() * 1e-6;
   lastRequestSeen = false;
   lastSendFrontId = pkt->frontpktid;
   lastRequestId++;

//   if (pkt->numresend)
//      DOUT(("Request:%u request front:%u tail:%u resubm:%u\n", lastRequestId, pkt->frontpktid, pkt->tailpktid, pkt->numresend));

   DOUT1(("Send request id:%u  Range: %u - %u nresend:%d resend[0] = %d", lastRequestId, pkt->tailpktid, pkt->frontpktid, pkt->numresend,
         pkt->numresend > 0 ? pkt->resend[0] : -1));

   pkt->password = htonl(ROC_PASSWORD);
   pkt->reqpktid = htonl(lastRequestId);
   pkt->frontpktid = htonl(pkt->frontpktid);
   pkt->tailpktid = htonl(pkt->tailpktid);
   for (uint32_t n=0; n < pkt->numresend; n++)
      pkt->resend[n] = htonl(pkt->resend[n]);
   pkt->numresend = htonl(pkt->numresend);

   DoSendBuffer(pkt, pkt_size);
}

bool roc::UdpDataSocket::Knut_checkDataRequest(UdpDataRequestFull* req, double curr_tm, bool check_retrans)
{
   // try to send credits increment any time when 1/2 of maximum is available

   if (!daqActive_) return false;


   // send request each 0.2 sec, if there is no replies on last request
   // send it much faster - every 0.01 sec.
   bool dosend =
      fabs(curr_tm - lastRequestTm) > (lastRequestSeen ? 0.2 : 0.01);

   uint32_t frontid = ringHeadId;
   if ((packetsToResend.size() < transferWindow) &&
       (ringSize < ringCapacity - transferWindow))
         frontid += (transferWindow - packetsToResend.size());

   // if newly calculated front id bigger than last
   if ((frontid - lastSendFrontId) < 0x80000000) {

     if ((frontid - lastSendFrontId) >= transferWindow / 3) dosend = true;

   } else
      frontid = lastSendFrontId;

   req->frontpktid = frontid;

   req->tailpktid = ringHeadId - ringSize;

   req->numresend = 0;

   if (!check_retrans && !dosend) return false;

   std::list<ResendInfo>::iterator iter = packetsToResend.begin();
   while (iter!=packetsToResend.end()) {
     if ((iter->numtry==0) || (fabs(curr_tm - iter->lasttm)) > 0.1) {
        iter->lasttm = curr_tm;
        iter->numtry++;
        if (iter->numtry < 8) {
            req->resend[req->numresend++] = iter->pktid;

            iter++;
            dosend = true;

            if (req->numresend >= sizeof(req->resend) / 4) {
               EOUT(("Number of resends more than one can pack in the retransmit packet\n"));
               break;
            }

        } else {

            std::list<ResendInfo>::iterator deliter = iter;

            unsigned target = ringHead + iter->pktid - ringHeadId;
            while (target >= ringCapacity) target += ringCapacity;

            EOUT(("Roc:%u Drop pkt %u tgt %u\n", rocNumber, iter->pktid, target));

            UdpDataPacketFull* tgt = ringBuffer + target;

            tgt->pktid = iter->pktid;
            tgt->nummsg = 1; // only one message
            tgt->lastreqid = 1;

            nxyter::Data* data = (nxyter::Data*) tgt->msgs;
            data->setRocNumber(rocNumber);
            data->setMessageType(ROC_MSG_SYS);
            data->setSysMesType(5); // this is lost packet mark

            iter++;
            packetsToResend.erase(deliter);
        }
     } else
       iter++;
   }

   return dosend;
}


void roc::UdpDataSocket::KnutaddDataPacket(UdpDataPacketFull* src, unsigned l)
{
   fTotalRecvPacket++;

   bool dosendreq = false;
   bool docheckretr = false;
   UdpDataRequestFull req;

   UdpDataPacketFull* tgt = 0;

   DOUT1(("Packet id:%u size %u reqid:%u", ntohl(src->pktid), ntohl(src->nummsg), ntohl(src->lastreqid)));

   double curr_tm = TimeStamp() * 1e-6;

//   bool data_call_back = false;

   {
//      dabc::LockGuard guard(dataMutex_);

      if (!daqActive_) return;

      uint32_t src_pktid = ntohl(src->pktid);

//      DOUT0(("Brd:%u  Packet id:%05u Tm:%7.5f Head:%u Need:%u\n", rocNumber, src_pktid, curr_tm, ringHeadId, dataRequired_ / MESSAGES_PER_PACKET));

      if (ntohl(src->lastreqid) == lastRequestId) lastRequestSeen = true;

      uint32_t gap = src_pktid - ringHeadId;

      if (gap < ringCapacity - ringSize) {
         // normal when gap == 0, meaning we get that we expecting

         while (ringHeadId != src_pktid) {
            // this is indicator of lost packets
            docheckretr = true;
            packetsToResend.push_back(ResendInfo(ringHeadId));

            ringHeadId++;
            ringHead = (ringHead+1) % ringCapacity;
            ringSize++;
         }

         tgt = ringBuffer + ringHead;
         ringHead = (ringHead+1) % ringCapacity;
         ringHeadId++;
         ringSize++;
      } else
      if (ringSize + gap < ringSize) {
         // this is normal resubmitted packet

         fTotalResubmPacket++;

         unsigned resubm = ringTail + ringSize + gap;
         while (resubm>=ringCapacity) resubm-=ringCapacity;
         tgt = ringBuffer + resubm;

         DOUT1(("Get resubm pkt %u on position %u headid = %u", src_pktid, resubm, ringHeadId));

         if (tgt->lastreqid != 0) {
            DOUT0(("Roc:%u Packet %u already seen  Resubm:%u Waitsz:%u ringSize:%u mode:%u",
                rocNumber, src_pktid, packetsToResend.size(), dataRequired_, ringSize, consumerMode_));
            if (src_pktid != tgt->pktid)
               EOUT(("!!!!!!!! Roc:%u Missmtach %u %u\n",rocNumber, src_pktid , tgt->pktid));
            tgt = 0;
         } else {
            // remove id from resend list
            std::list<ResendInfo>::iterator iter = packetsToResend.begin();
            while (iter!=packetsToResend.end()) {
               if (iter->pktid == src_pktid) {
                  packetsToResend.erase(iter);
                  break;
               }
               iter++;
            }
         }
      } else {
         EOUT(("Packet with id %u far away from expected range %u - %u",
                 src_pktid, ringHeadId - ringSize, ringHeadId));
      }

      if (tgt!=0) {
         tgt->lastreqid = 1;
         tgt->pktid = src_pktid;
         tgt->nummsg = ntohl(src->nummsg);
         memcpy(tgt->msgs, src->msgs, tgt->nummsg*6);
      }

      if (ringSize >= ringCapacity)
         EOUT(("RING BUFFER DANGER Roc:%u size:%u capacity:%u", rocNumber, ringSize, ringCapacity));

      // check if we found start/stop messages
      if ((tgt!=0) && ((daqState == daqStarting) || (daqState == daqStopping)))
         for (unsigned n=0;n<tgt->nummsg;n++) {
            nxyter::Data* data = (nxyter::Data*) (tgt->msgs + n*6);
            if (data->isStartDaqMsg()) {
               DOUT1(("Find start message"));
               if (daqState == daqStarting)
                  daqState = daqRuns;
               else
                  EOUT(("Start DAQ when not expected"));
            } else
            if (data->isStopDaqMsg()) {
               DOUT1(("Find stop message"));
               if (daqState == daqStopping)
                  daqState = daqStopped;
               else
                 EOUT(("Stop DAQ when not expected"));
            }
         }

      if (daqState == daqStopped)
         if ((packetsToResend.size()==0) && !docheckretr)
            daqActive_ = false;

      dosendreq = Knut_checkDataRequest(&req, curr_tm, docheckretr);

      // one should have definite amount of valid packets in data queue

   }

   AddBufferToQueue();

   if (dosendreq)
      KnutsendDataRequest(&req);
}

bool roc::UdpDataSocket::Knut_checkAvailData(unsigned num_msg)
{
   unsigned min_num_pkts = (num_msg < MESSAGES_PER_PACKET) ? 1 : num_msg / MESSAGES_PER_PACKET;

   // no chance to get required number of packets
   if ((ringSize - packetsToResend.size()) < min_num_pkts) return false;

   unsigned ptr = ringTail;
   unsigned total_sz = 0;
   while (ptr != ringHead) {
      // check, if there is lost packets
      if (ringBuffer[ptr].lastreqid == 0) return false;
      total_sz += ringBuffer[ptr].nummsg;
      if (total_sz >= num_msg) return true;
      ptr = (ptr+1) % ringCapacity;
   }

   return false;
}

bool roc::UdpDataSocket::KnutfillData(void* buf, unsigned& sz)
{
   char* tgt = (char*) buf;
   unsigned fullsz = sz;
   sz = 0;

   unsigned start(0), stop(0), count(0);


   // in first lock take region, where we can copy data from
   {
//      dabc::LockGuard guard(dataMutex_);

      if (fillingData_) {
          EOUT(("Other consumer fills data from buffer !!!\n"));
          return false;
      }

      // no data to fill at all
      if (ringSize == 0) return false;

      fillingData_ = true;

      start = ringTail; stop = ringTail;
      while (stop!=ringHead) {
         if (ringBuffer[stop].lastreqid == 0) break;
         stop = (stop + 1) % ringCapacity;
      }
   }

   // now out of locked region start data filling

   if (sorter_) sorter_->startFill(buf, fullsz);

   while (start != stop) {
      UdpDataPacketFull* pkt = ringBuffer + start;

      if (sorter_) {
         // -1 while sorter want to put epoch, but did not do this
         if (sorter_->sizeFilled() >= (fullsz/6 - 1)) break;

         sorter_->addData((nxyter::Data*) pkt->msgs, pkt->nummsg);
      } else {
         unsigned pktsize = pkt->nummsg * 6;
         if (pktsize>fullsz) {
            if (sz==0)
               EOUT(("Buffer size %u is too small for complete packet size %u\n", fullsz, pktsize));
            break;
         }

         memcpy(tgt, pkt->msgs, pktsize);

         sz+=pktsize;
         tgt+=pktsize;
         fullsz-=pktsize;
      }

      pkt->lastreqid = 0; // mark as no longer used
      start = (start+1) % ringCapacity;
      count++; // how many packets taken from buffer
   }

   if (sorter_) {
      sz = sorter_->sizeFilled() * 6;
      sorter_->stopFill();
   }

   {
//      dabc::LockGuard guard(dataMutex_);

      ringTail = start;
      fillingData_ = false;
      ringSize -= count;
   }

   if (sz>0) FireEvent(evntCheckRequest);

   return sz>0;
}
*/
