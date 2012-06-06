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

#include "hadaq/UdpTransport.h"


#include "dabc/timing.h"
#include "dabc/Port.h"
#include "dabc/version.h"

#include <math.h>

// FIXME for hadtu
const unsigned UDP_DATA_SIZE = ((sizeof(hadaq::UdpDataPacketFull) - sizeof(hadaq::UdpDataPacket)) / 6) * 6;

hadaq::UdpDataSocket::UdpDataSocket(dabc::Reference port, int fd) :
   dabc::SocketWorker(fd),
   dabc::Transport(port.Ref()),
   dabc::MemoryPoolRequester(),
   //fDev(dev),
   fQueueMutex(),
   fQueue(((dabc::Port*) port())->InputQueueCapacity()),
   fReadyBuffers(0),
   fTransportBuffers(0),
   fTgtBuf(0)
{
   // we will react on all input packets
   SetDoingInput(true);

   fTgtBufIndx = 0;
   fTgtShift = 0;
   fTgtPtr = 0;
   fTgtCheckGap = false;

   fTgtNextId = 0;
   fTgtTailId = 0;

   fBufferSize = 0;
   fTransferWindow = 40;

   //rocNumber = 0;
   daqState = daqInit;
   daqCheckStop = false;

   fTotalRecvPacket = 0;
   fTotalResubmPacket = 0;

   //lastRequestTm.Reset();

   fFlushTimeout = .1;
   //fLastDelivery.Reset();

   ConfigureFor((dabc::Port*) port());
}

hadaq::UdpDataSocket::~UdpDataSocket()
{
   ResetDaq();

}

void hadaq::UdpDataSocket::ConfigureFor(dabc::Port* port)
{
   fBufferSize = port->Cfg(dabc::xmlBufferSize).AsInt(16384);
   //fTransferWindow = port->Cfg(roc::xmlTransferWindow).AsInt(fTransferWindow);
   fFlushTimeout = port->Cfg(dabc::xmlFlushTimeout).AsDouble(fFlushTimeout);
   // DOUT0(("fFlushTimeout = %5.1f %s", fFlushTimeout, dabc::xmlFlushTimeout));

   fPool = port->GetMemoryPool();
   fReadyBuffers = 0;

   fTgtCheckGap = false;

   // one cannot have too much resend requests
   //fResend.Allocate(port->InputQueueCapacity() * fBufferSize / UDP_DATA_SIZE);

   // put here additional socket options of nettrans:
   ////////JAM this is copy of hadaq nettrans.c how to set this?
   //         int rcvBufLenReq = 1 * (1 << 20);
   //            int rcvBufLenRet;
   //            size_t rcvBufLenLen = (size_t) sizeof(rcvBufLenReq);
   //
   //            if (setsockopt(   fSocket, SOL_SOCKET, SO_RCVBUF, &rcvBufLenReq, rcvBufLenLen) == -1) {
   //               syslog(LOG_WARNING, "setsockopt(..., SO_RCVBUF, ...): %s", strerror(errno));
   //            }
   //
   //            if (getsockopt(   fSocket, SOL_SOCKET, SO_RCVBUF, &rcvBufLenRet, (socklen_t *) &rcvBufLenLen) == -1) {
   //               syslog(LOG_WARNING, "getsockopt(..., SO_RCVBUF, ...): %s", strerror(errno));
   //            }
   //            if (rcvBufLenRet < rcvBufLenReq) {
   //               syslog(LOG_WARNING, "UDP receive buffer length (%d) smaller than requested buffer length (%d)", rcvBufLenRet,
   //                     rcvBufLenReq);
   //            }
   //




   DOUT2(("hadaq::UdpDataSocket:: Pool = %p buffer size = %u", fPool(), fBufferSize));
}

void hadaq::UdpDataSocket::ProcessEvent(const dabc::EventId& evnt)
{
   switch (evnt.GetCode()) {
      case evntSocketRead: {

         // this is required for DABC 2.0 to again enable read event generation
         SetDoingInput(true);

         void *tgt = fTgtPtr;
         if (tgt==0) tgt = fTempBuf;

         ssize_t len = DoRecvBufferHdr(&fTgtHdr, sizeof(UdpDataPacket),
                                        tgt, UDP_DATA_SIZE);
//         if (len>0) {
//            fTotalRecvPacket++;
////            DOUT0(("READ Packet %d len %d", ntohl(fTgtHdr.pktid), len));
//            AddDataPacket(len, tgt);
//         }
          len++;
         // TODO: implement reading data from socket with
         //DoRecvBuffer(void* buf, ssize_t len)
         // after figuring out the length of next hadtu packet with
         // DoRecvBufferHdr(&fTgtHdr, sizeof(UdpDataPacket),


         break;
      }

      case evntStartTransport: {

         // no need to do anything when daq is already running
//         if (daqActive()) {
//            DOUT1(("Daq active - ignore start transport?"));
//            return;
//         }
//
//         ResetDaq();
//
//         daqState = daqStarting;
//
//         fStartList.clear();
//
//         fStartList.addPut(ROC_NX_FIFO_RESET, 1);
//         fStartList.addPut(ROC_NX_FIFO_RESET, 0);
//         fStartList.addPut(ROC_ETH_START_DAQ, 1);
//
//         CmdNOper cmd(&fStartList);

         //if (fDev) fDev->Submit(Assign(cmd));
         break;
      }

      case evntStopTransport: {
         // this is situation, when normal module stops its transports
         // here we do put(stop_daq) ourself and just waiting confirmation that command completed
         // there is no yet way so suspend daq from module

//         if (!daqActive()) {
//            DOUT1(("Daq is not active - no need to stop transport"));
//            return;
//         }
//
//         daqState = daqStopping;
//
//         fTgtBuf = 0; // forget about buffer, it is owned by the queue
//         fTgtBufIndx = 0;
//         fTgtShift = 0;
//         fTgtPtr = 0;
//
//         fLastDelivery.Reset();

         //if (fDev) fDev->Submit(Assign(CmdPut(ROC_ETH_STOP_DAQ, 1)));
         break;
      }

      case evntConfirmCmd: {
//         if (evnt.GetArg() == 0) {
//            ResetDaq();
//            daqState = daqFails;
//            ActivateTimeout(-1.);
//         } else
//         if (daqState == daqStarting) {
//            daqState = daqRuns;
//            fLastDelivery.GetNow();
//            AddBuffersToQueue();
//            ActivateTimeout(0.0001);
//         } else
//         if (daqState == daqStopping)  {
//            DOUT3(("STOP DAQ command is ready - go in normal state"));
//            ResetDaq();
//         }
         break;
      }

      case evntFillBuffer:
         AddBuffersToQueue();
         break;

      default:
         dabc::SocketWorker::ProcessEvent(evnt);
         break;
   }
}

bool hadaq::UdpDataSocket::ReplyCommand(dabc::Command cmd)
{
   int res = cmd.GetResult();

   DOUT3(("hadaq::UdpDataSocket::ReplyCommand %s res = %s ", cmd.GetName(), DBOOL(res)));

   FireEvent(evntConfirmCmd, res==dabc::cmd_true ? 1 : 0);

   return true;
}

bool hadaq::UdpDataSocket::Recv(dabc::Buffer& buf)
{
   {
      dabc::LockGuard lock(fQueueMutex);
      if (fReadyBuffers==0) return false;
      if (fQueue.Size()<=0) return false;

   #if DABC_VERSION_CODE >= DABC_VERSION(1,9,2)
      fQueue.PopBuffer(buf);
    #else
      buf << fQueue.Pop();
    #endif

      fReadyBuffers--;
   }
   FireEvent(evntFillBuffer);

   return !buf.null();
}

unsigned  hadaq::UdpDataSocket::RecvQueueSize() const
{
   dabc::LockGuard guard(fQueueMutex);

   return fReadyBuffers;
}

dabc::Buffer& hadaq::UdpDataSocket::RecvBuffer(unsigned indx) const
{
   dabc::LockGuard lock(fQueueMutex);

   if (indx>=fReadyBuffers)
      throw dabc::Exception(dabc::format("Wrong index %u ready %u in hadaq::UdpDataSocket::RecvBuffer", indx, fReadyBuffers).c_str());

   return fQueue.ItemRef(indx);
}

bool hadaq::UdpDataSocket::ProcessPoolRequest()
{
   FireEvent(evntFillBuffer);
   return true;
}

void hadaq::UdpDataSocket::StartTransport()
{
   DOUT3(("Starting UDP transport "));

   FireEvent(evntStartTransport);
}

void hadaq::UdpDataSocket::StopTransport()
{
   DOUT3(("Stopping udp transport %ld", fTotalRecvPacket));

   FireEvent(evntStopTransport);
}

void hadaq::UdpDataSocket::AddBuffersToQueue(bool checkanyway)
{
   unsigned cnt = 0;

   {
      dabc::LockGuard lock(fQueueMutex);
      cnt = fQueue.Capacity() - fQueue.Size();
   }

   bool isanynew = false;

   while (cnt) {
      dabc::Buffer buf = fPool.TakeBufferReq(this, fBufferSize);
      if (buf.null()) break;

      fTransportBuffers++;

      isanynew = true;
      cnt--;
      buf.SetTypeId(hadaq::mbt_HadaqEvents);
      dabc::LockGuard lock(fQueueMutex);
      fQueue.Push(buf);

      if (fTgtBuf==0) {
         fTgtBuf = &fQueue.Last();
         fTgtShift = 0;
         fTgtPtr = (char*) fTgtBuf->SegmentPtr();
      }

   }

   //if (isanynew || checkanyway) CheckNextRequest();
}

//bool hadaq::UdpDataSocket::CheckNextRequest(bool check_retrans)
//{
//   if (!daqActive()) return false;
//
//   UdpDataRequestFull req;
//   dabc::TimeStamp curr_tm = dabc::Now();
//
//   // send request each 0.2 sec,
//   // if there is no replies on last request send it much faster - every 0.01 sec.
//   bool dosend = lastRequestTm.Expired(curr_tm, lastRequestSeen ? 0.2 : 0.01);
//
//   int can_send = 0;
//   if (fTgtBuf) {
//      can_send += (fBufferSize - fTgtShift) / UDP_DATA_SIZE;
//      can_send += (fTransportBuffers - fTgtBufIndx - 1) * (fBufferSize / UDP_DATA_SIZE);
//   }
//
//   if (can_send > (int) fTransferWindow) can_send = fTransferWindow;
//
//   if (fResend.Size() >= fTransferWindow) can_send = 0; else
//   if (can_send + fResend.Size() > fTransferWindow)
//      can_send = fTransferWindow - fResend.Size();
//
//   req.frontpktid = fTgtNextId + can_send;
//
//   // if newly calculated front id bigger than last
//   if ((req.frontpktid - lastSendFrontId) < 0x80000000) {
//
//     if ((req.frontpktid - lastSendFrontId) >= fTransferWindow / 3) dosend = true;
//
//   } else
//      req.frontpktid = lastSendFrontId;
//
//   req.tailpktid = fTgtTailId;
//
//   req.numresend = 0;
//
//   if (can_send==0) dosend = false;
//
//   if (!check_retrans && !dosend) return false;
//
//   for (unsigned n=0; n<fResend.Size(); n++) {
//      ResendInfo* entry = fResend.ItemPtr(n);
//
//      if ((entry->numtry>0) && !entry->lasttm.Expired(curr_tm, 0.1)) continue;
//
//      entry->lasttm = curr_tm;
//      entry->numtry++;
//      if (entry->numtry < 8) {
//         req.resend[req.numresend++] = entry->pktid;
//
//         dosend = true;
//
//         if (req.numresend >= sizeof(req.resend) / 4) {
//            EOUT(("Number of resends more than one can pack in the retransmit packet"));
//            break;
//         }
//
//      } else {
//         EOUT(("Roc:%u Drop pkt %u\n", rocNumber, entry->pktid));
//
//         fTgtCheckGap = true;
//
//         memset(entry->ptr, 0, UDP_DATA_SIZE);
//
//         roc::Message msg;
//
//         msg.setRocNumber(rocNumber);
//         msg.setMessageType(roc::MSG_SYS);
//         msg.setSysMesType(roc::SYSMSG_PACKETLOST);
//         msg.copyto(entry->ptr, fFormat);
//
//         fResend.RemoveItem(n);
//         n--;
//      }
//
//   }
//
//   if (!dosend) return false;
//
//   uint32_t pkt_size = sizeof(UdpDataRequest) + req.numresend * sizeof(uint32_t);
//
//   // make request always 4 byte aligned
//   while ((pkt_size < MAX_UDP_PAYLOAD) &&
//          (pkt_size + UDP_PAYLOAD_OFFSET) % 4) pkt_size++;
//
//   lastRequestTm = curr_tm;
//   lastRequestSeen = false;
//   lastSendFrontId = req.frontpktid;
//   lastRequestId++;
//
////   DOUT0(("Send request id:%u  Range: 0x%04x - 0x%04x nresend:%d resend[0] = 0x%04x tgtbuf %p ptr %p tgtsize %u",
////         lastRequestId, req.tailpktid, req.frontpktid, req.numresend,
////         req.numresend > 0 ? req.resend[0] : 0, fTgtBuf, fTgtPtr, fTransportBuffers));
//
//   req.password = htonl(ROC_PASSWORD);
//   req.reqpktid = htonl(lastRequestId);
//   req.frontpktid = htonl(req.frontpktid);
//   req.tailpktid = htonl(req.tailpktid);
//   for (uint32_t n=0; n < req.numresend; n++)
//      req.resend[n] = htonl(req.resend[n]);
//   req.numresend = htonl(req.numresend);
//
//   DoSendBuffer(&req, pkt_size);
//
//   if (fDev && fDev->fCtrlCh) fDev->fCtrlCh->SetLastSendTime();
//
//   return true;
//}

double hadaq::UdpDataSocket::ProcessTimeout(double)
{
   if (!daqActive()) return -1;

//   if (fTgtBuf == 0)
//      AddBuffersToQueue(true);
//   else
//      CheckNextRequest();
//
//   // check if we should flush current buffer
//   if (!fLastDelivery.null() && fLastDelivery.Expired(fFlushTimeout)) {
////          DOUT0(("Doing flush timeout = %3.1f dist = %5.1f last = %8.6f", fFlushTimeout, fLastDelivery.SpentTillNow(), fLastDelivery.AsDouble()));
//          CheckReadyBuffers(true);
//   }

//   DOUT0(("CALIBR tm1:%f tm2:%f %s", fLastCalibrAction, fLastCalibrStart, DBOOL(dabc::IsNullTime(fLastCalibrStart))));

   return 0.01;
}


void hadaq::UdpDataSocket::ResetDaq()
{
//   daqCheckStop = false;
//   daqState = daqInit;
//
//   fTransportBuffers = 0;
//
//   fTgtBuf = 0;
//   fTgtBufIndx = 0;
//   fTgtShift = 0;
//   fTgtPtr = 0;
//
//   fTgtNextId = 0;
//   fTgtTailId = 0;
//   fTgtCheckGap = false;
//
//   lastRequestId = 0;
//   lastSendFrontId = 0;
//   lastRequestTm.Reset();
//   lastRequestSeen = true;
//
//   fResend.Reset();
//
//   dabc::LockGuard lock(fQueueMutex);
//   fQueue.Cleanup();
//   fReadyBuffers = 0;
//
//   fLastDelivery.Reset();
}

void hadaq::UdpDataSocket::AddDataPacket(int len, void* tgt)
{
//   uint32_t src_pktid = ntohl(fTgtHdr.pktid);
//
//   if (tgt==0) {
//      DOUT0(("Packet 0x%04x has no place buf %p bufindx %u queue %u ready %u", src_pktid, fTgtBuf, fTgtBufIndx, fQueue.Size(), fReadyBuffers));
//      for (unsigned n=0;n < fResend.Size(); n++)
//         DOUT0(("   Need resend 0x%04x retry %d", fResend.ItemPtr(n)->pktid, fResend.ItemPtr(n)->numtry));
//
//      CheckNextRequest();
//
//      return;
//   }
//
//   if (len <= (int) sizeof(UdpDataPacket)) {
//      EOUT(("Too few data received %d", len));
//      return;
//   }
//
//   if (ntohl(fTgtHdr.lastreqid) == lastRequestId) lastRequestSeen = true;
//
//   int nummsgs = ntohl(fTgtHdr.nummsg);
//
//   uint32_t gap = src_pktid - fTgtNextId;
//
//   int data_len = nummsgs * 6;
//
////   if (gap!=0)
////      DOUT0(("Packet id:0x%04x Head:0x%04x NumMsgs:%d gap:%u", src_pktid, fTgtNextId, nummsgs, gap));
//
//   bool packetaccepted = false;
//   bool doflush = false;
//
//   if ((fTgtPtr==tgt) && (gap < fBufferSize / UDP_DATA_SIZE * fTransportBuffers)) {
//
//      if (gap>0) {
//         // some packets are lost on the way, move pointer forward and
//         // remember packets which should be resubmit
//         void* src = fTgtPtr;
//
//         while (fTgtNextId != src_pktid) {
//
//            ResendInfo* info = fResend.PushEmpty();
//
////            DOUT0(("!!!! Lost packet 0x%04x", fTgtNextId));
//
//            info->pktid = fTgtNextId;
//            info->lasttm.Reset();
//            info->numtry = 0;
//            info->buf = fTgtBuf;
//            info->bufindx = fTgtBufIndx;
//            info->ptr = fTgtPtr;
//
//            fTgtNextId++;
//            fTgtShift += UDP_DATA_SIZE;
//            fTgtPtr += UDP_DATA_SIZE;
//
//            if (fTgtShift + UDP_DATA_SIZE > fTgtBuf->GetTotalSize()) {
//               fTgtBuf->SetTotalSize(fTgtShift);
//               fTgtBufIndx++;
//               if (fTgtBufIndx >= fTransportBuffers) {
//                  EOUT(("One get packet out of the available buffer spaces gap = %u indx %u numbufs %u !!!!", gap, fTgtBufIndx, fTransportBuffers));
//                  return;
//               }
//
//               {
//                  dabc::LockGuard lock(fQueueMutex);
//                  fTgtBuf = &fQueue.ItemRef(fReadyBuffers + fTgtBufIndx);
//               }
//
//               fTgtPtr = (char*) fTgtBuf->SegmentPtr();
//               fTgtShift = 0;
//            }
//         }
//
//         // copy data which was received into the wrong place of the buffers
//         memcpy(fTgtPtr, src, data_len);
//
////         DOUT1(("Copy pkt 0x%04x to buffer %p shift %u", src_pktid, fTgtBuf, fTgtShift));
//      }
//
//      // from here just normal situation when next packet is arrived
//
//      if (fResend.Size()==0) fTgtTailId = fTgtNextId;
//
//      fTgtNextId++;
//
//      fTgtShift += data_len;
//      fTgtPtr += data_len;
//
//      if (fTgtBuf->GetTotalSize() < fTgtShift + UDP_DATA_SIZE) {
//         fTgtPtr = 0;
//         fTgtBuf->SetTotalSize(fTgtShift);
//         fTgtShift = 0;
//         fTgtBuf = 0;
//         fTgtBufIndx++;
//      }
//
//      packetaccepted = true;
//
//   } else {
//      // this is retransmitted packet, may be received in temporary place
//      for (unsigned n=0; n<fResend.Size(); n++) {
//         ResendInfo* entry = fResend.ItemPtr(n);
//         if (entry->pktid != src_pktid) continue;
//
//         DOUT3(("Get retransmitted packet 0x%04x", src_pktid));
//
//         fTotalResubmPacket++;
//
//         memcpy(entry->ptr, tgt, data_len);
//         if (data_len < (int) UDP_DATA_SIZE) {
//            void* restptr = (char*) entry->ptr + data_len;
//            memset(restptr, 0, UDP_DATA_SIZE - data_len);
//            fTgtCheckGap = true;
//         }
//
//         fResend.RemoveItem(n);
//
//         // if all packets retransmitted, one can allow to skip buffers on roc,
//         // beside next packet, which is required
//         if (fResend.Size()==0) fTgtTailId = fTgtNextId - 1;
//
//         packetaccepted = true;
//
//         break;
//      }
//
//   }
//
//   if (!packetaccepted) {
//      DOUT3(("ROC:%u Packet 0x%04x was not accepted - FLUSH???  ready = %u transport = %u tgtindx = %u buf %p", fDev->rocNumber(), src_pktid, fReadyBuffers, fTransportBuffers, fTgtBufIndx, fTgtBuf));
////      dabc::SetDebugLevel(1);
//   }
//
//   // check incoming data for stop/start messages
//   if (packetaccepted && (data_len>0) && (tgt!=0) && daqCheckStop) {
////      DOUT0(("Search special kind of message !!!"));
//
//      Iterator iter(fFormat);
//      iter.assign(tgt, data_len);
//
//      while (iter.next())
//         if (iter.msg().isStopDaqMsg()) {
//            DOUT2(("Find STOP_DAQ message"));
//            doflush = true;
//         }
//   }
//
//   CheckReadyBuffers(doflush);
}

void hadaq::UdpDataSocket::CompressBuffer(dabc::Buffer& buf)
{
   if (buf.null()) return;

//   Iterator iter(fFormat);
//   if (!iter.assign(buf.SegmentPtr(), buf.SegmentSize())) return;
//
//   uint8_t* tgt = (uint8_t*) buf.SegmentPtr();
//   uint8_t* src = tgt;
//
//   uint32_t rawsize = roc::Message::RawSize(fFormat);
//   dabc::BufferSize_t tgtsize = 0;
//
//   while (iter.next()) {
//      if (iter.msg().isNopMsg()) {
//         src += rawsize;
//      } else {
//         if (tgt!=src) memcpy(tgt, src, rawsize);
//         src += rawsize;
//         tgt += rawsize;
//         tgtsize += rawsize;
//      }
//   }
//
//   if (tgtsize==0)
//      EOUT(("Zero size after compress !!!"));

//   buf.SetTotalSize(tgtsize);
}

void hadaq::UdpDataSocket::CheckReadyBuffers(bool doflush)
{
//   if (doflush) DOUT0(("doing flush %d", rocNumber));

//   if (doflush && (fTgtBuf!=0) && (fTgtShift>0) && (fResend.Size()==0)) {
//      DOUT2(("Flush buffer when recv of size %u", fTgtShift));
//      fTgtPtr = 0;
//      fTgtBuf->SetTotalSize(fTgtShift);
//      fTgtShift = 0;
//      fTgtBuf = 0;
//      fTgtBufIndx++;
//   }
//
//   if (fTgtBufIndx>0) {
//      unsigned minindx = fTgtBufIndx;
//
//      for (unsigned n=0; n<fResend.Size(); n++) {
//         unsigned indx = fResend.ItemPtr(n)->bufindx;
//         if (indx < minindx) minindx = indx;
//      }
//
////      DOUT0(("CheckReadyBuffers minindx = %u resend = %u", minindx, fResend.Size()));
//
//      if (minindx>0) {
//
//         fTransportBuffers -= minindx;
//         fTgtBufIndx -= minindx;
//         for (unsigned n=0; n<fResend.Size(); n++)
//            fResend.ItemPtr(n)->bufindx -= minindx;
//
//         {
//            dabc::LockGuard lock(fQueueMutex);
//
//            // check all buffers on gaps, if necessary
//            if (fTgtCheckGap)
//               for (unsigned n=0;n<minindx;n++)
//                  CompressBuffer(fQueue.ItemRef(fReadyBuffers + n));
//
//            fReadyBuffers += minindx;
//         }
//
//         while (minindx--) FirePortInput();
//
////         DOUT0(("!!!!!!!!!!!!!!!! Do outptut !!!!!!!!!!!!!!!"));
//
//         fLastDelivery.GetNow();
//      }
//   }
//
//   if ((fTgtBuf==0) && (fTgtBufIndx<fTransportBuffers)) {
//      dabc::LockGuard lock(fQueueMutex);
//      fTgtBuf = &fQueue.ItemRef(fReadyBuffers + fTgtBufIndx);
//      fTgtPtr = (char*) fTgtBuf->SegmentPtr();
//      fTgtShift = 0;
//      // one can disable checks once we have no data in queues at all
//      if ((fTgtBufIndx==0) && (fResend.Size()==0)) {
////         if (fTgtCheckGap) DOUT0(("!!! DISABLE COMPRESS !!!"));
//         fTgtCheckGap = false;
//      }
//   }
//
//   if (fTgtBuf == 0)
//      AddBuffersToQueue();
//   else
//      CheckNextRequest();
}

//bool hadaq::UdpDataSocket::prepareForSuspend()
//{
//   if (!daqActive()) return false;
//   daqCheckStop = true;
//   return true;
//}

int hadaq::UdpDataSocket::GetParameter(const char* name)
{
//   if ((strcmp(name, hadaq::xmlRocNumber)==0) && fDev) return fDev->rocNumber();
//   if (strcmp(name, hadaq::xmlMsgFormat)==0) return fFormat;
//   if (strcmp(name, hadaq::xmlTransportKind)==0) return roc::kind_UDP;

   return dabc::Transport::GetParameter(name);
}
