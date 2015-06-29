// $Id$

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

#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/syscall.h>


#include "dabc/timing.h"
#include "dabc/Manager.h"


hadaq::DataSocketAddon::DataSocketAddon(int fd, int nport, int mtu, double flush, bool debug, int maxloop) :
   dabc::SocketAddon(fd),
   dabc::DataInput(),
   fNPort(nport),
   fTgtPtr(),
   fWaitMoreData(false),
   fMTU(mtu > 0 ? mtu : DEFAULT_MTU),
   fFlushTimeout(flush),
   fSendCnt(0),
   fMaxLoopCnt(maxloop > 1 ? maxloop : 1),
   fTotalRecvPacket(0),
   fTotalDiscardPacket(0),
   fTotalDiscard32Packet(0),
   fTotalRecvBytes(0),
   fTotalDiscardBytes(0),
   fTotalProducedBuffers(0),
   fDebug(debug)
{
   fPid = syscall(SYS_gettid);
}

hadaq::DataSocketAddon::~DataSocketAddon()
{
}

void hadaq::DataSocketAddon::ProcessEvent(const dabc::EventId& evnt)
{
   if (evnt.GetCode() == evntSocketRead) {
      // inside method if necessary SetDoingInput(true); should be done

      // ignore events when not waiting for the new data
      if (!fWaitMoreData) return;

      unsigned res = ReadUdp();

      if (res != dabc::di_CallBack) {
         fWaitMoreData = false;
         MakeCallback(res);
      }

      return;
   }

   dabc::SocketAddon::ProcessEvent(evnt);
}

long hadaq::DataSocketAddon::Notify(const std::string& msg, int arg)
{
   if (msg == "TransportWantToStop") {

      if (fWaitMoreData) {
         DOUT2("hadaq::DataSocketAddon notified, stop waiting data");
         fWaitMoreData = false;
         MakeCallback(dabc::di_Ok);
      }

      return 0;
   }

   return dabc::SocketAddon::Notify(msg, arg);
}


double hadaq::DataSocketAddon::ProcessTimeout(double lastdiff)
{
   if (!fWaitMoreData) return -1;

   if ((fTgtPtr.distance_to_ownbuf()>0) && (fSendCnt==0)) {
      fWaitMoreData = false;
      MakeCallback(dabc::di_Ok);
      return -1;
   }

   // check buffer with period of fFlushTimeout
   return fFlushTimeout;
}


unsigned hadaq::DataSocketAddon::ReadUdp()
{
   if (fTgtPtr.null()) {
      // if call was done from socket, just do nothing and wait buffer
      DOUT0("UDP:%d ReadUdp at wrong moment - no buffer to read", fNPort);
      return dabc::di_Error;
   }

   if (fTgtPtr.rawsize() < fMTU) {
      DOUT0("UDP:%d Should never happen - rest size is smaller than MTU", fNPort);
      return dabc::di_Error;
   }

   int cnt = fMaxLoopCnt;

   while (cnt-- > 0) {

      /* this was old form which is not necessary - socket is already bind with the port */
      //  socklen_t socklen = sizeof(fSockAddr);
      //  ssize_t res = recvfrom(Socket(), fTgtPtr.ptr(), fMTU, 0, (sockaddr*) &fSockAddr, &socklen);

      ssize_t res = recv(Socket(), fTgtPtr.ptr(), fMTU, 0);

      if (res == 0) {
         DOUT0("UDP:%d Seems to be, socket was closed", fNPort);
         return dabc::di_EndOfStream;
      }

      if (res<0) {
         // socket do not have data, one should enable event processing
         // otherwise we need to poll for the new data
         if (errno == EAGAIN) break;
         EOUT("Socket error");
         return dabc::di_Error;
      }

      hadaq::HadTu* hadTu = (hadaq::HadTu*) fTgtPtr.ptr();
      int msgsize = hadTu->GetPaddedSize() + 32; // trb sender adds a 32 byte control trailer identical to event header

      std::string errmsg;

      if (res != msgsize) {
         errmsg = dabc::format("Send buffer %d differ from message size %d - ignore it", res, msgsize);
      } else
      if (memcmp((char*) hadTu + hadTu->GetPaddedSize(), (char*) hadTu, 32)!=0) {
         fTotalDiscard32Packet++;
         errmsg = "Trailing 32 bytes do not match to header - ignore packet";
      }

      if (!errmsg.empty()) {
         DOUT3("UDP:%d %s", fNPort, errmsg.c_str());
         if (fDebug && (dabc::lgr()->GetDebugLevel()>2)) {
            errmsg = dabc::format("   Packet length %d", res);
            uint32_t* ptr = (uint32_t*) hadTu;
            for (unsigned n=0;n<res/4;n++) {
               if (n%8 == 0) {
                  printf("   %s\n", errmsg.c_str());
                  errmsg = dabc::format("0x%04x:", n*4);
               }

               errmsg.append(dabc::format(" 0x%08x", (unsigned) ptr[n]));
            }
            printf("   %s\n",errmsg.c_str());
         }

         fTotalDiscardPacket++;
         fTotalDiscardBytes+=res;
         continue;
      }

      fTotalRecvPacket++;
      fTotalRecvBytes += res;

      fTgtPtr.shift(hadTu->GetPaddedSize());

      // when rest size is smaller that mtu, one should close buffer
      if (fTgtPtr.rawsize() < fMTU)
         return dabc::di_Ok; // this is end
   }

   SetDoingInput(true);
   return dabc::di_CallBack; // indicate that buffer reading will be finished by callback
}


void hadaq::DataSocketAddon::MakeCallback(unsigned arg)
{
   dabc::InputTransport* tr = dynamic_cast<dabc::InputTransport*> (fWorker());

   if (tr==0) {
      EOUT("Did not found InputTransport on other side worker %p", fWorker());
      SubmitWorkerCmd(dabc::Command("CloseTransport"));
   } else {
      // DOUT0("Activate CallBack with arg %u", arg);
      tr->Read_CallBack(arg);
   }
}


unsigned hadaq::DataSocketAddon::Read_Start(dabc::Buffer& buf)
{
   if (!fTgtPtr.null() || fWaitMoreData) {
      EOUT("Read_Start at wrong moment");
      return dabc::di_Error;
   }

   fTgtPtr.reset(buf);

   if (fTgtPtr.rawsize() < fMTU) {
      EOUT("not enough space in the buffer - at least %u is required", fMTU);
      return dabc::di_Error;
   }

   unsigned res = ReadUdp();

   fWaitMoreData = (res == dabc::di_CallBack);

   // we are waiting for event callback, configure else timeout
   if (fWaitMoreData) {
      fSendCnt = 0;
      ActivateTimeout(fFlushTimeout);
   }

   DOUT3("hadaq::DataSocketAddon::Read_Start buf %u res %u", buf.GetTotalSize(), res);

   return res;
}

unsigned hadaq::DataSocketAddon::Read_Complete(dabc::Buffer& buf)
{
   if (fTgtPtr.null()) return dabc::di_Ok;

   unsigned fill_sz = fTgtPtr.distance_to_ownbuf();
   fTgtPtr.reset();

   if (fill_sz==0) EOUT("Zero bytes was read");
   buf.SetTypeId(hadaq::mbt_HadaqTransportUnit);

   buf.SetTotalSize(fill_sz);

   fSendCnt++;
   fTotalProducedBuffers++;

   DOUT3("hadaq::DataSocketAddon::Read_Complete buf %u", buf.GetTotalSize());

//   DOUT0("Receiver %d produce buffer of size %u", fNPort, buf.GetTotalSize());
   return dabc::di_Ok;
}

void hadaq::DataSocketAddon::ClearCounters()
{
   fTotalRecvPacket = 0;
   fTotalDiscardPacket = 0;
   fTotalDiscard32Packet = 0;
   fTotalRecvBytes = 0;
   fTotalDiscardBytes = 0;
   fTotalProducedBuffers = 0;
}

int hadaq::DataSocketAddon::OpenUdp(int nport, int rcvbuflen)
{
   int fd = socket(PF_INET, SOCK_DGRAM, 0);
   if (fd < 0) return -1;

   if (!dabc::SocketThread::SetNonBlockSocket(fd)) {
      EOUT("Cannot set non-blocking mode");
      close(fd);
      return -1;
   }

   sockaddr_in addr;
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(nport);

   if (rcvbuflen > 0) {
       // for hadaq application: set receive buffer length _before_ bind:
       //         int rcvBufLenReq = 1 * (1 << 20);
       int rcvBufLenRet;
       socklen_t rcvBufLenLen = sizeof(rcvbuflen);
       if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuflen, rcvBufLenLen) == -1) {
          EOUT("Fail to setsockopt SO_RCVBUF %s", strerror(errno));
       }

      if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvBufLenRet, &rcvBufLenLen) == -1) {
          EOUT("fail to getsockopt SO_RCVBUF, ...): %s", strerror(errno));
      }

      if (rcvBufLenRet < rcvbuflen) {
         EOUT("UDP receive buffer length (%d) smaller than requested buffer length (%d)", rcvBufLenRet, rcvbuflen);
         rcvbuflen = rcvBufLenRet;
      }
   }

   if (!bind(fd, (struct sockaddr *) &addr, sizeof(addr))) return fd;
   close(fd);
   return -1;
}

// =================================================================================

hadaq::DataTransport::DataTransport(dabc::Command cmd, const dabc::PortRef& inpport, DataSocketAddon* addon, bool observer) :
   dabc::InputTransport(cmd, inpport, addon, true),
   fIdNumber(0),
   fWithObserver(observer),
   fDataRateName()
{
   // do not process to much events at once, let another transports a chance
   SetPortLoopLength(OutputName(), 2);

   fIdNumber = inpport.ItemSubId();

   DOUT3("Starting hadaq::DataTransport %s %s id %d", GetName(), (fWithObserver ? "with observer" : ""), fIdNumber);

   if(fWithObserver) {
      // workaround to suppress problems with dim observer when this ratemeter is registered:
      std::string ratesprefix = inpport.GetName();
      fDataRateName = ratesprefix + "-Datarate";
      CreatePar(fDataRateName).SetRatemeter(false, 5.).SetUnits("MB");
      //Par(fDataRateName).SetDebugLevel(1);
      SetPortRatemeter(OutputName(), Par(fDataRateName));

      CreateNetmemPar(dabc::format("pktsReceived%d",fIdNumber));
      CreateNetmemPar(dabc::format("pktsDiscarded%d",fIdNumber));
      CreateNetmemPar(dabc::format("msgsReceived%d",fIdNumber));
      CreateNetmemPar(dabc::format("msgsDiscarded%d",fIdNumber));
      CreateNetmemPar(dabc::format("bytesReceived%d",fIdNumber));
      CreateNetmemPar(dabc::format("netmemBuff%d",fIdNumber));
      CreateNetmemPar(dabc::format("bytesReceivedRate%d",fIdNumber));
      CreateNetmemPar(dabc::format("portNr%d",fIdNumber));
      SetNetmemPar(dabc::format("portNr%d",fIdNumber), addon->fNPort);

      CreateNetmemPar("coreNr");
      // exclude PID from shared mem not to interfere with default pid of observer
      // TODO: fix default pid export of worker ?
      CreateNetmemPar("PID");


      SetNetmemPar("PID", (int) addon->fPid);
      SetNetmemPar("coreNr", hadaq::CoreAffinity(addon->fPid));
      CreateTimer("ObserverTimer", 1, false);
      DOUT3("hadaq::DataTransport created observer parameters");
   }
}

hadaq::DataTransport::~DataTransport()
{

}

void hadaq::DataTransport::ProcessTimerEvent(unsigned timer)
{
   if (TimerName(timer) == "ObserverTimer")
      UpdateExportedCounters();

   dabc::InputTransport::ProcessTimerEvent(timer);
}


std::string  hadaq::DataTransport::GetNetmemParName(const std::string& name)
{
   return dabc::format("%s-%s",hadaq::NetmemPrefix,name.c_str());
}

void hadaq::DataTransport::CreateNetmemPar(const std::string& name)
{
   CreatePar(GetNetmemParName(name));
}

void hadaq::DataTransport::SetNetmemPar(const std::string& name, unsigned value)
{
   Par(GetNetmemParName(name)).SetValue(value);
}

bool hadaq::DataTransport::UpdateExportedCounters()
{
   DataSocketAddon* addon = dynamic_cast<DataSocketAddon*> (fAddon());

   if(!fWithObserver || (addon==0)) return false;

   SetNetmemPar(dabc::format("pktsReceived%d", fIdNumber), addon->fTotalRecvPacket);
   SetNetmemPar(dabc::format("pktsDiscarded%d", fIdNumber), addon->fTotalDiscardPacket);
   SetNetmemPar(dabc::format("msgsReceived%d", fIdNumber), addon->fTotalRecvPacket);
   SetNetmemPar(dabc::format("msgsDiscarded%d", fIdNumber), addon->fTotalDiscardPacket);
   SetNetmemPar(dabc::format("bytesReceived%d", fIdNumber), addon->fTotalRecvBytes);
   unsigned capacity = PortQueueCapacity(OutputName());
   float ratio = 100.;
   if (capacity>0) ratio -= 100.* NumCanSend()/capacity;
   SetNetmemPar(dabc::format("netmemBuff%d",fIdNumber), (unsigned) ratio);
   SetNetmemPar(dabc::format("bytesReceivedRate%d",fIdNumber), (unsigned) (Par(fDataRateName).Value().AsDouble() * 1024 * 1024));

// does updating the affinity cause performance loss?
//    static int affcount=0;
//    if(affcount++ % 20)
//     {
//     SetNetmemPar("PID", (int) addon->fPid);
//     SetNetmemPar("coreNr", hadaq::CoreAffinity(addon->fPid));
//     //SetNetmemPar("coreNr", hadaq::CoreAffinity(0));
//     }
   return true;
}

int hadaq::DataTransport::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("ResetExportedCounters")) {
      DataSocketAddon* addon = dynamic_cast<DataSocketAddon*> (fAddon());
      if (addon!=0) addon->ClearCounters();
      UpdateExportedCounters();
      return dabc::cmd_true;
   } else
   if (cmd.IsName("GetHadaqTransportInfo")) {
      cmd.SetPtr("Addon", fAddon());
      return dabc::cmd_true;
   }

   return dabc::InputTransport::ExecuteCommand(cmd);
}
