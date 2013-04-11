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
#include "roc/defines_roc.h"
#include "roc/defines_udp.h"
#include "roc/Message.h"
#include "roc/Iterator.h"

#include "dabc/timing.h"
#include "dabc/version.h"
#include "dabc/DataTransport.h"

#include "mbs/MbsTypeDefs.h"

#include <math.h>

const unsigned UDP_DATA_SIZE = ((sizeof(roc::UdpDataPacketFull) - sizeof(roc::UdpDataPacket)) / 6) * 6;


roc::UdpSocketAddon::UdpSocketAddon(UdpDevice* dev, int fd, int queuesize, int fmt, bool mbsheader, unsigned rocid) :
   dabc::SocketAddon(fd),
   fDev(dev),
   fBufferSize(0),
   fTransferWindow(30),
   fQueue(queuesize),
   fReadyBuffers(0),
   fTransportBuffers(0),
   fTgtBuf(0),
   fTgtBufIndx(0),
   fTgtShift(0),
   fTgtZeroShift(0),
   fTgtHdr(),
   fTgtPtr(0),
   fTgtNextId(0),
   fTgtTailId(0),
   fTgtCheckGap(false),
   fTempBuf(),
   fResend(),
   daqState(daqInit),
   daqCheckStop(false),
   fRecvBuf(),
   lastRequestTm(),
   lastRequestSeen(false),
   lastRequestId(0),
   lastSendFrontId(0),
   rocNumber(rocid),
   fTotalRecvPacket(0),
   fTotalResubmPacket(0),
   fFlushTimeout(1.),
   fLastDelivery(),
   fStartList(),
   fFormat(fmt),
   fMbsHeader(mbsheader),
   fBufCounter(0)
{
   if (fMbsHeader) fTgtZeroShift = sizeof(mbs::EventHeader) + sizeof(mbs::SubeventHeader);
}

roc::UdpSocketAddon::~UdpSocketAddon()
{
   if (fDev) fDev->fDataCh = 0;
   fDev = 0;
}

double roc::UdpSocketAddon::ProcessTimeout(double last_diff)
{
   if (!daqActive()) return -1;

   if (fTgtBuf != 0)
      CheckNextRequest();

   // check if we should flush current buffer
   if (!fLastDelivery.null() && fLastDelivery.Expired(fFlushTimeout)) {
       CheckReadyBuffers(true);
   }

//   DOUT0("CALIBR tm1:%f tm2:%f %s", fLastCalibrAction, fLastCalibrStart, DBOOL(dabc::IsNullTime(fLastCalibrStart)));

   return 0.01;
}


long roc::UdpSocketAddon::Notify(const std::string& cmd, int arg)
{
   DOUT0("roc::UdpSocketAddon::Notify %s", cmd.c_str());

   if (cmd == "StartTransport") {
      if (daqActive()) {
         DOUT1("Daq active - ignore start transport?");
         return 1;
      }

      ResetDaq();

      daqState = daqStarting;

      fStartList.clear();


      // FIXME: here board-specific sequence should be used
      fStartList.addPut(ROC_NX_FIFO_RESET, 1);
      fStartList.addPut(ROC_NX_FIFO_RESET, 0);
      fStartList.addPut(ROC_ETH_START_DAQ, 1);

      DOUT0("********* Invoke start list **********");

      CmdNOper cmd(&fStartList);
      cmd.SetBool("ReplyTransport", true);
      if (fDev) fDev->Submit(cmd);

      return 1;
   }

   if (cmd == "StopTransport") {
      if (!daqActive()) {
         DOUT1("Daq is not active - no need to stop transport");
         return 1;
      }

      daqState = daqStopping;

      fTgtBuf = 0; // forget about buffer, it is owned by the queue
      fTgtBufIndx = 0;
      fTgtShift = 0;
      fTgtPtr = 0;

      fLastDelivery.Reset();

      CmdPut cmd(ROC_ETH_STOP_DAQ, 1);
      cmd.SetBool("ReplyTransport", true);
      if (fDev) fDev->Submit(cmd);

      return 1;

   }

   return dabc::SocketAddon::Notify(cmd, arg);
}

void roc::UdpSocketAddon::ReplyTransport(dabc::Command cmd, bool res)
{
   DOUT0("********* ReplyTransport  cmd %s **********", cmd.GetName());

   if (!res) {
      ResetDaq();
      daqState = daqFails;
      ActivateTimeout(-1.);
   } else
   if (daqState == daqStarting) {
      daqState = daqRuns;
      fLastDelivery.GetNow();
      DOUT0("Activate transport %s %s!!", DBOOL(daqActive()), DBOOL(IsDoingInput()));
      SetDoingInput(true);
      ActivateTimeout(0.0001);
   } else
   if (daqState == daqStopping)  {
      DOUT3("STOP DAQ command is ready - go in normal state");
      ResetDaq();
   }
}


void roc::UdpSocketAddon::ProcessEvent(const dabc::EventId& evnt)
{
   switch (evnt.GetCode()) {
      case evntSocketRead: {

         // this is required for DABC 2.0 to again enable read event generation
         SetDoingInput(true);

         void *tgt = fTgtPtr;
         if (tgt==0) tgt = fTempBuf;

         ssize_t len = DoRecvBufferHdr(&fTgtHdr, sizeof(UdpDataPacket),
                                        tgt, UDP_DATA_SIZE);
         if (len>0) {
            fTotalRecvPacket++;
//            DOUT0("READ Packet %d len %d", ntohl(fTgtHdr.pktid), len);
            AddDataPacket(len, tgt);
         }

         break;
      }
      default:
         dabc::SocketAddon::ProcessEvent(evnt);
   }
}


void roc::UdpSocketAddon::AddBufferToQueue(dabc::Buffer& buf)
{
   if (fBufferSize==0) {
      fBufferSize = buf.GetTotalSize();
      fResend.Allocate(fQueue.Capacity() * fBufferSize / UDP_DATA_SIZE);
   }

   fTransportBuffers++;

   buf.SetTypeId(fMbsHeader ? mbs::mbt_MbsEvents : roc::rbt_RawRocData + fFormat);
   fQueue.PushBuffer(buf);

   if (fTgtBuf==0) {
      fTgtBuf = &fQueue.Back();
      DOUT0("Assign next tgt buffer %p", fTgtBuf);

      fTgtShift = fTgtZeroShift;
      fTgtPtr = (char*) fTgtBuf->SegmentPtr() + fTgtShift;
   }

   CheckNextRequest();
}

bool roc::UdpSocketAddon::CheckNextRequest(bool check_retrans)
{
   if (!daqActive()) return false;

   UdpDataRequestFull req;
   dabc::TimeStamp curr_tm = dabc::Now();

   // send request each 0.2 sec,
   // if there is no replies on last request send it much faster - every 0.01 sec.
   bool dosend = lastRequestTm.Expired(curr_tm, lastRequestSeen ? 0.2 : 0.01);

   int can_send = 0;
   if (fTgtBuf) {
      can_send += (fBufferSize - fTgtShift) / UDP_DATA_SIZE;
      can_send += (fTransportBuffers - fTgtBufIndx - 1) * (fBufferSize / UDP_DATA_SIZE);
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

      if ((entry->numtry>0) && !entry->lasttm.Expired(curr_tm, 0.1)) continue;

      entry->lasttm = curr_tm;
      entry->numtry++;
      if (entry->numtry < 8) {
         req.resend[req.numresend++] = entry->pktid;

         dosend = true;

         if (req.numresend >= sizeof(req.resend) / 4) {
            EOUT("Number of resends more than one can pack in the retransmit packet");
            break;
         }

      } else {
         EOUT("Roc:%u Drop pkt %u\n", rocNumber, entry->pktid);

         fTgtCheckGap = true;

         memset(entry->ptr, 0, UDP_DATA_SIZE);

         roc::Message msg;

         msg.setRocNumber(rocNumber);
         msg.setMessageType(roc::MSG_SYS);
         msg.setSysMesType(roc::SYSMSG_PACKETLOST);
         msg.copyto(entry->ptr, fFormat);

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

//   DOUT0("Send request id:%u  Range: 0x%04x - 0x%04x nresend:%d resend[0] = 0x%04x tgtbuf %p ptr %p tgtsize %u",
//         lastRequestId, req.tailpktid, req.frontpktid, req.numresend,
//         req.numresend > 0 ? req.resend[0] : 0, fTgtBuf, fTgtPtr, fTransportBuffers);

   req.password = htonl(ROC_PASSWORD);
   req.reqpktid = htonl(lastRequestId);
   req.frontpktid = htonl(req.frontpktid);
   req.tailpktid = htonl(req.tailpktid);
   for (uint32_t n=0; n < req.numresend; n++)
      req.resend[n] = htonl(req.resend[n]);
   req.numresend = htonl(req.numresend);

//   DOUT0("Send request");

   DoSendBuffer(&req, pkt_size);

   if (fDev) fDev->SetLastSendTime();

   return true;
}


void roc::UdpSocketAddon::AddDataPacket(int len, void* tgt)
{
   uint32_t src_pktid = ntohl(fTgtHdr.pktid);

   if (tgt==0) {
      DOUT0("Packet 0x%04x has no place buf %p bufindx %u queue %u ready %u", src_pktid, fTgtBuf, fTgtBufIndx, fQueue.Size(), fReadyBuffers);
      for (unsigned n=0;n < fResend.Size(); n++)
         DOUT0("   Need resend 0x%04x retry %d", fResend.ItemPtr(n)->pktid, fResend.ItemPtr(n)->numtry);

      CheckNextRequest();

      return;
   }

   if (len <= (int) sizeof(UdpDataPacket)) {
      EOUT("Too few data received %d", len);
      return;
   }

   if (ntohl(fTgtHdr.lastreqid) == lastRequestId) lastRequestSeen = true;

   int nummsgs = ntohl(fTgtHdr.nummsg);

   uint32_t gap = src_pktid - fTgtNextId;

   int data_len = nummsgs * 6;

//   if (gap!=0)
//      DOUT0("Packet id:0x%04x Head:0x%04x NumMsgs:%d gap:%u", src_pktid, fTgtNextId, nummsgs, gap);

   bool packetaccepted = false;
   bool doflush = false;

   if ((fTgtPtr==tgt) && (gap < fBufferSize / UDP_DATA_SIZE * fTransportBuffers)) {

      if (gap>0) {
         // some packets are lost on the way, move pointer forward and
         // remember packets which should be resubmit
         void* src = fTgtPtr;

         while (fTgtNextId != src_pktid) {

            ResendInfo* info = fResend.PushEmpty();

//            DOUT0("!!!! Lost packet 0x%04x", fTgtNextId);

            info->pktid = fTgtNextId;
            info->lasttm.Reset();
            info->numtry = 0;
            info->buf = fTgtBuf;
            info->bufindx = fTgtBufIndx;
            info->ptr = fTgtPtr;

            fTgtNextId++;
            fTgtShift += UDP_DATA_SIZE;
            fTgtPtr += UDP_DATA_SIZE;

            if (fTgtShift + UDP_DATA_SIZE > fTgtBuf->GetTotalSize()) {

               FinishTgtBuffer();

               if (fTgtBufIndx >= fTransportBuffers) {
                  EOUT("One get packet out of the available buffer spaces gap = %u indx %u numbufs %u !!!!", gap, fTgtBufIndx, fTransportBuffers);
                  return;
               }

               fTgtBuf = &fQueue.Item(fReadyBuffers + fTgtBufIndx);

               fTgtShift = fTgtZeroShift;
               fTgtPtr = (char*) fTgtBuf->SegmentPtr() + fTgtShift;
            }
         }

         // copy data which was received into the wrong place of the buffers
         memcpy(fTgtPtr, src, data_len);

//         DOUT1("Copy pkt 0x%04x to buffer %p shift %u", src_pktid, fTgtBuf, fTgtShift);
      }

      // from here just normal situation when next packet is arrived

      if (fResend.Size()==0) fTgtTailId = fTgtNextId;

      fTgtNextId++;

      fTgtShift += data_len;
      fTgtPtr += data_len;

      if (fTgtBuf->GetTotalSize() < fTgtShift + UDP_DATA_SIZE) {
         FinishTgtBuffer();
      }

      packetaccepted = true;

   } else {
      // this is retransmitted packet, may be received in temporary place
      for (unsigned n=0; n<fResend.Size(); n++) {
         ResendInfo* entry = fResend.ItemPtr(n);
         if (entry->pktid != src_pktid) continue;

         DOUT3("Get retransmitted packet 0x%04x", src_pktid);

         fTotalResubmPacket++;

         memcpy(entry->ptr, tgt, data_len);
         if (data_len < (int) UDP_DATA_SIZE) {
            void* restptr = (char*) entry->ptr + data_len;
            memset(restptr, 0, UDP_DATA_SIZE - data_len);
            fTgtCheckGap = true;
         }

         fResend.RemoveItem(n);

         // if all packets retransmitted, one can allow to skip buffers on roc,
         // beside next packet, which is required
         if (fResend.Size()==0) fTgtTailId = fTgtNextId - 1;

         packetaccepted = true;

         break;
      }

   }

   if (!packetaccepted) {
      DOUT3("ROC:%u Packet 0x%04x was not accepted - FLUSH???  ready = %u transport = %u tgtindx = %u buf %p", fDev->rocNumber(), src_pktid, fReadyBuffers, fTransportBuffers, fTgtBufIndx, fTgtBuf);
//      dabc::SetDebugLevel(1);
   }

   // check incoming data for stop/start messages
   if (packetaccepted && (data_len>0) && (tgt!=0) && daqCheckStop) {
//      DOUT0("Search special kind of message !!!");

      Iterator iter(fFormat);
      iter.assign(tgt, data_len);

      while (iter.next())
         if (iter.msg().isStopDaqMsg()) {
            DOUT2("Find STOP_DAQ message");
            doflush = true;
         }
   }

   CheckReadyBuffers(doflush);
}

void roc::UdpSocketAddon::FinishTgtBuffer()
{
   if (fTgtBuf==0) {
      EOUT("Internal failure!!!");
      exit(765);
   }

   fTgtBuf->SetTotalSize(fTgtShift);

   if (fMbsHeader) {

      // add MBS header when specified

      mbs::EventHeader* evhdr = (mbs::EventHeader*) fTgtBuf->SegmentPtr();
      evhdr->Init(fBufCounter++);
      evhdr->SetFullSize(fTgtShift);

      mbs::SubeventHeader* subhdr = evhdr->SubEvents();
      subhdr->Init(rocNumber, roc::proc_RawData, fFormat);
      subhdr->SetFullSize(fTgtShift - sizeof(mbs::EventHeader));
   }

   fTgtPtr = 0;
   fTgtShift = 0;
   fTgtBuf = 0;
   fTgtBufIndx++;
}

void roc::UdpSocketAddon::CheckReadyBuffers(bool doflush)
{
//   if (doflush) DOUT0("doing flush %d", rocNumber);

   if (doflush && (fTgtBuf!=0) && (fTgtShift>fTgtZeroShift) && (fResend.Size()==0)) {
      DOUT2("Flush buffer when recv of size %u", fTgtShift);
      FinishTgtBuffer();
   }

   bool inform_transport(false);

   if (fTgtBufIndx>0) {
      unsigned minindx = fTgtBufIndx;

      for (unsigned n=0; n<fResend.Size(); n++) {
         unsigned indx = fResend.ItemPtr(n)->bufindx;
         if (indx < minindx) minindx = indx;
      }

//      DOUT0("CheckReadyBuffers minindx = %u resend = %u", minindx, fResend.Size());

      if (minindx>0) {

         fTransportBuffers -= minindx;
         fTgtBufIndx -= minindx;
         for (unsigned n=0; n<fResend.Size(); n++)
            fResend.ItemPtr(n)->bufindx -= minindx;

         // check all buffers on gaps, if necessary
         if (fTgtCheckGap)
            for (unsigned n=0;n<minindx;n++)
               CompressBuffer(fQueue.Item(fReadyBuffers + n));

        fReadyBuffers += minindx;

        inform_transport = minindx>0;

        fLastDelivery.GetNow();

      }
   }

   if ((fTgtBuf==0) && (fTgtBufIndx<fTransportBuffers)) {
      fTgtBuf = &fQueue.Item(fReadyBuffers + fTgtBufIndx);
      fTgtShift = fTgtZeroShift;
      fTgtPtr = (char*) fTgtBuf->SegmentPtr() + fTgtShift;
      // one can disable checks once we have no data in queues at all
      if ((fTgtBufIndx==0) && (fResend.Size()==0)) {
//         if (fTgtCheckGap) DOUT0("!!! DISABLE COMPRESS !!!");
         fTgtCheckGap = false;
      }
   }

   if (fTgtBuf != 0)
      CheckNextRequest();

   if (inform_transport)
      MakeCallback(dabc::di_QueueBufReady);

   // before here was request for new buffers when fTgtBuf==0,
   // buffer will be delivered to the addon by other means in other
}

void roc::UdpSocketAddon::CompressBuffer(dabc::Buffer& buf)
{
   if (buf.null()) return;

   Iterator iter(fFormat);
   if (!iter.assign(buf.SegmentPtr(), buf.SegmentSize())) return;

   uint8_t* tgt = (uint8_t*) buf.SegmentPtr();
   uint8_t* src = tgt;

   uint32_t rawsize = roc::Message::RawSize(fFormat);
   dabc::BufferSize_t tgtsize = 0;

   while (iter.next()) {
      if (iter.msg().isNopMsg()) {
         src += rawsize;
      } else {
         if (tgt!=src) memcpy(tgt, src, rawsize);
         src += rawsize;
         tgt += rawsize;
         tgtsize += rawsize;
      }
   }

   if (tgtsize==0)
      EOUT("Zero size after compress !!!");

   buf.SetTotalSize(tgtsize);
}

bool roc::UdpSocketAddon::prepareForSuspend()
{
   if (!daqActive()) return false;
   daqCheckStop = true;
   return true;
}



void roc::UdpSocketAddon::ResetDaq()
{
   daqCheckStop = false;
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
   lastRequestTm.Reset();
   lastRequestSeen = true;

   fResend.Reset();

   if (fQueue.Size()>0) {
      EOUT("!!!!!! Transport wait for our call-back - what to do !!!!");
      MakeCallback(dabc::di_SkipBuffer);
   }

   fQueue.Cleanup();
   fReadyBuffers = 0;

   fLastDelivery.Reset();
}


void roc::UdpSocketAddon::MakeCallback(unsigned arg)
{
   dabc::InputTransport* tr = dynamic_cast<dabc::InputTransport*> (fWorker());

   if (tr==0) {
      EOUT("Didnot found DataInputTransport on other side worker %p", fWorker());
      SubmitWorkerCmd("CloseTransport");
   } else {
      // DOUT0("Activate CallBack with arg %u", arg);
      tr->Read_CallBack(arg);
   }
}

unsigned roc::UdpSocketAddon::Read_Start(dabc::Buffer& buf)
{
//   DOUT0("roc::UdpSocketAddon::Read_Start  size %u", buf.GetTotalSize());

   if (fQueue.Full()) {
      EOUT("Queue is already full, why we get new buffer???");
      return dabc::di_Error;
   }

   AddBufferToQueue(buf);

   return fQueue.Full() ? dabc::di_HasEnoughBuf : dabc::di_NeedMoreBuf;
}

unsigned roc::UdpSocketAddon::Read_Complete(dabc::Buffer& buf)
{
//   DOUT0("roc::UdpSocketAddon::Read_Complete numready %u", fReadyBuffers);

   if ((fReadyBuffers==0) || (fQueue.Size()==0) || !buf.null()) return dabc::di_Error;

   fQueue.PopBuffer(buf);

   fReadyBuffers--;

   return (fReadyBuffers>0) ? dabc::di_MoreBufReady : dabc::di_Ok;
}

