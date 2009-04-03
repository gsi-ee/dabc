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

#include "roc/UdpDevice.h"

#include <iostream>

#include "dabc/Manager.h"

#include "roc/Defines.h"


roc::UdpControlSocket::UdpControlSocket(UdpDevice* dev, int fd) :
   dabc::SocketIOProcessor(fd),
   fDev(dev)
{
   SetDoingInput(true);
}

roc::UdpControlSocket::~UdpControlSocket()
{
   if (fDev) fDev->fCtrl = 0;
   fDev = 0;
}

void roc::UdpControlSocket::ProcessEvent(dabc::EventId evnt)
{
   switch (dabc::GetEventCode(evnt)) {
      case evntSendCtrl:
         DOUT0(("Send packet of size %u", fDev->controlSendSize));
         StartSend(&(fDev->controlSend), fDev->controlSendSize);
//         StartRecv(&fRecvBuf, sizeof(fRecvBuf), false, true);
         break;
      default:
         dabc::SocketIOProcessor::ProcessEvent(evnt);
   }
}

bool roc::UdpControlSocket::OnRecvProvideBuffer()
{
   DOUT0(("Try recv single buffer"));

   return LetRecv(&fRecvBuf, sizeof(fRecvBuf), true);
}

void roc::UdpControlSocket::OnRecvCompleted()
{
   DOUT0(("On recv single buffer %u", fLastRecvSize));

   SetDoingInput(true);

   if (fDev) fDev->processCtrlMessage(&fRecvBuf, fLastRecvSize);
}

// __________________________________________________________


roc::UdpDevice::UdpDevice(dabc::Basic* parent, const char* name, const char* thrdname, dabc::Command* cmd) :
   roc::Device(parent, name),
   fCtrl(0),
   fCond(),
   ctrlState_(0),
   controlSendSize(0),
   isBrdStat(false),
   displayConsoleOutput_(false)
{
   fConnected = false;

   std::string remhost = GetCfgStr(roc::xmlBoardIP, "", cmd);
   if (remhost.length()==0) return;

   if (!dabc::mgr()->MakeThreadFor(this, thrdname)) return;

   int nport = 8725;

   int fd = dabc::SocketThread::StartUdp(nport, nport, nport+1000);

   DOUT0(("Socket %d port %d  remote %s", fd, nport, remhost.c_str()));

   fd = dabc::SocketThread::ConnectUdp(fd, remhost.c_str(), ROC_DEFAULT_CTRLPORT);

   if (fd<=0) return;

   fCtrl = new UdpControlSocket(this, fd);

   fCtrl->AssignProcessorToThread(ProcessorThread());

   DOUT1(("Create control socket for port %d connected to host %s", nport, remhost.c_str()));

   fConnected = true;
}

roc::UdpDevice::~UdpDevice()
{
   if (fCtrl) {
      fCtrl->fDev = 0;
      fCtrl->DestroyProcessor();
   }
   fCtrl = 0;
}

int roc::UdpDevice::CreateTransport(dabc::Command* cmd, dabc::Port* port)
{
   return roc::Device::CreateTransport(cmd, port);
}

bool roc::UdpDevice::poke(uint32_t addr, uint32_t value)
{
   double tmout = 5;

   fErrNo = 0;

   bool show_progress = false;

   // define operations, which takes longer time as usual one
   switch (addr) {
      case ROC_CFG_WRITE:
      case ROC_CFG_READ:
      case ROC_OVERWRITE_SD_FILE:
      case ROC_DO_AUTO_DELAY:
      case ROC_DO_AUTO_LATENCY:
      case ROC_FLASH_KIBFILE_FROM_DDR:
         if (tmout < 10) tmout = 10.;
         show_progress = true;
         break;
   }

   controlSend.tag = ROC_POKE;
   controlSend.address = htonl(addr);
   controlSend.value = htonl(value);
   controlSendSize = sizeof(UdpMessage);

   fErrNo = 6;

   if (performCtrlLoop(tmout, show_progress))
      fErrNo = ntohl(controlRecv.value);

   DOUT0(("Roc:%s Poke(0x%04x, 0x%04x) res = %d\n", GetName(), addr, value, fErrNo));

   return fErrNo == 0;
}

uint32_t roc::UdpDevice::peek(uint32_t addr)
{
   double tmout = 5;

   DOUT0(("Doing peek"));

   controlSend.tag = ROC_PEEK;
   controlSend.address = htonl(addr);
   controlSendSize = sizeof(UdpMessage);

   fErrNo = 6;

   uint32_t res = 0;

   if (performCtrlLoop(tmout, false)) {
      res = ntohl(controlRecv.value);
      fErrNo = ntohl(controlRecv.address);
   }

   DOUT0(("Roc:%s Peek(0x%04x, 0x%04x) res = %d\n", GetName(), addr, res, fErrNo));

   return res;
}

bool roc::UdpDevice::performCtrlLoop(double total_tmout_sec, bool show_progress)
{
   // normal reaction time of the ROC is 0.5 ms, therefore one can resubmit command
   // as soon as several ms without no reply
   // At the same time there are commands which takes seconds (like SD card write)
   // Therefore, one should try again and again if command is done

   if (IsExecutionThread()) {
      EOUT(("Cannot perform control from our own thread !!!"));
      return false;
   }

   {
      // before we start sending, indicate that we now in control loop
      // and can accept replies from ROC
      dabc::LockGuard guard(fCond.CondMutex());
      if (fCtrl==0) return false;
      if (ctrlState_!=0) {
         EOUT(("cannot start operation - somebody else uses control loop"));
         return false;
      }

      ctrlState_ = 1;
   }

   controlSend.password = htonl(ROC_PASSWORD);
   controlSend.id = htonl(currentMessagePacketId++);

   bool res = false;

   // send aligned to 4 bytes packet to the ROC

   while ((controlSendSize < MAX_UDP_PAYLOAD) &&
          (controlSendSize + UDP_PAYLOAD_OFFSET) % 4) controlSendSize++;

   if (total_tmout_sec>20.) show_progress = true;

   // if fast mode we will try to resend as fast as possible
   bool fast_mode = (total_tmout_sec < 10.) && !show_progress;
   int loopcnt = 0;
   bool wasprogressout = false;
   bool doresend = true;

   do {
      if (doresend) {
         fCtrl->FireEvent(roc::UdpControlSocket::evntSendCtrl);
         doresend = false;
      }

      double wait_tm = fast_mode ? (loopcnt++ < 4 ? 0.01 : loopcnt*0.1) : 1.;

      dabc::LockGuard guard(fCond.CondMutex());

      fCond._DoWait(wait_tm);

      // resend packet in fast mode always, in slow mode only in the end of interval
      doresend = fast_mode ? true : total_tmout_sec <=5.;

      total_tmout_sec -= wait_tm;

      if (ctrlState_ == 2)
         res = true;
      else
      if (show_progress) {
          std::cout << ".";
          std::cout.flush();
          wasprogressout = true;
      }
   } while (!res && (total_tmout_sec>0.));

   if (wasprogressout) std::cout << std::endl;

   dabc::LockGuard guard(fCond.CondMutex());
   ctrlState_ = 0;
   return res;
}

void roc::UdpDevice::processCtrlMessage(UdpMessageFull* pkt, unsigned len)
{
   // procees PEEK or POKE reply messages from ROC

   if(pkt->tag == ROC_CONSOLE) {
      pkt->address = ntohl(pkt->address);

      switch (pkt->address) {
         case ROC_STATBLOCK:
            setBoardStat(pkt->rawdata, displayConsoleOutput_);
            break;

         case ROC_DEBUGMSG:
            if (displayConsoleOutput_)
               DOUT0(("\033[0;%dm Roc:%d %s \033[0m", 36, 0, (const char*) pkt->rawdata));
            break;
         default:
            if (displayConsoleOutput_)
               DOUT0(("Error addr 0x%04x in cosle message\n", pkt->address));
      }

      return;
   }

   dabc::LockGuard guard(fCond.CondMutex());

   // check first that user waits for reply
   if (ctrlState_ != 1) return;

   // if packed id is not the same, do not react
   if(controlSend.id != pkt->id) return;

   // if packed tag is not the same, do not react
   if(controlSend.tag != pkt->tag) return;

   memcpy(&controlRecv, pkt, len);

   ctrlState_ = 2;
   fCond._DoFire();
}

void roc::UdpDevice::setBoardStat(void* rawdata, bool print)
{
   UdpStatistic* src = (UdpStatistic*) rawdata;

   if (src!=0) {
      brdStat.dataRate = ntohl(src->dataRate);
      brdStat.sendRate = ntohl(src->sendRate);
      brdStat.recvRate = ntohl(src->recvRate);
      brdStat.nopRate = ntohl(src->nopRate);
      brdStat.frameRate = ntohl(src->frameRate);
      brdStat.takePerf = ntohl(src->takePerf);
      brdStat.dispPerf = ntohl(src->dispPerf);
      brdStat.sendPerf = ntohl(src->sendPerf);
      isBrdStat = true;
   }

   if (print && isBrdStat)
      DOUT0(("\033[0;%dm Roc:%u  Data:%6.3f MB/s Send:%6.3f MB/s Recv:%6.3f MB/s NOPs:%2u Frames:%2u Data:%4.1f%s Disp:%4.1f%s Send:%4.1f%s \033[0m",
         36, 0, brdStat.dataRate*1e-6, brdStat.sendRate*1e-6, brdStat.recvRate*1e-6,
             brdStat.nopRate, brdStat.frameRate,
             brdStat.takePerf*1e-3,"%", brdStat.dispPerf*1e-3,"%", brdStat.sendPerf*1e-3,"%"));
}


