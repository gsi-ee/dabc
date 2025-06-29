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

#include "dogma/UdpTransport.h"

#include "dabc/Manager.h"

#include <cerrno>
#include <cmath>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <sched.h>

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>


// according to specification maximal UDP packet is 65,507 or 0xFFE3
#define DEFAULT_MTU 0xFFF0

dogma::UdpAddon::UdpAddon(int fd, const std::string &host, int nport, int rcvbuflen, int mtu, bool debug, bool print, int maxloop, double reduce) :
   dabc::SocketAddon(fd),
   TransportInfo(nport),
   fTgtPtr(),
   fHostName(host),
   fRecvBufLen(rcvbuflen),
   fMTU(mtu > 0 ? mtu : DEFAULT_MTU),
   fMtuBuffer(nullptr),
   fSkipCnt(0),
   fSendCnt(0),
   fMaxLoopCnt(maxloop > 1 ? maxloop : 1),
   fReduce(reduce < 0.1 ? 0.1 : (reduce > 1. ? 1. : reduce)),
   fDebug(debug),
   fPrint(print),
   fRunning(false),
   fMaxProcDist(0.)
{
   fMtuBuffer = std::malloc(fMTU);
   fUdpProfiler.Reserve(50);
}

dogma::UdpAddon::~UdpAddon()
{
   std::free(fMtuBuffer);
}

void dogma::UdpAddon::ProcessEvent(const dabc::EventId& evnt)
{
   if (evnt.GetCode() == evntSocketRead) {

      // ignore events when not waiting for the new data
      if (fRunning) { ReadUdp(); SetDoingInput(true); }
   } else {
      dabc::SocketAddon::ProcessEvent(evnt);
   }
}

long dogma::UdpAddon::Notify(const std::string &msg, int arg)
{
   if (msg == "TransportWantToStart") {
      fLastProcTm.GetNow();
      fMaxProcDist = 0;
      fRunning = true;
      SetDoingInput(true);
      return 0;
   }

   if (msg == "TransportWantToStop") {
      fRunning = false;
      SetDoingInput(false);
      return 0;
   }

   return dabc::SocketAddon::Notify(msg, arg);
}


bool dogma::UdpAddon::CloseBuffer()
{
   if (fTgtPtr.null()) return false;
   unsigned fill_sz = fTgtPtr.distance_to_ownbuf();
   if (fill_sz == 0) return false;

   fTgtPtr.buf().SetTypeId(dogma::mbt_DogmaTransportUnit);
   fTgtPtr.buf().SetTotalSize(fill_sz);
   fTgtPtr.reset();

   fSendCnt++;
   fTotalProducedBuffers++;
   return true;
}


bool dogma::UdpAddon::ReadUdp()
{
   if (!fRunning)
      return false;

   PROFILER_GURAD(fUdpProfiler, "start", 0)

   auto tr = static_cast<dogma::UdpTransport *> (fWorker());
   if (!tr) {
      EOUT("No transport assigned");
      return false;
   }

   if (fDebug) {
      double tm = fLastProcTm.SpentTillNow(true);
      if (tm > fMaxProcDist) fMaxProcDist = tm;
   }

   void *tgt = nullptr;

   PROFILER_BLOCK("buf1")

   if (fTgtPtr.null()) {
      if (!tr->AssignNewBuffer(0, this)) {
         if (fSkipCnt++ < 10) { fTotalArtificialSkip++; return false; }
         tgt = fMtuBuffer;
      }
   }

   if (tgt != fMtuBuffer) {
      if (fTgtPtr.rawsize() < fMTU) {
         DOUT0("UDP:%d Should never happen - rest size is smaller than MTU", fNPort);
         return false;
      }
      fSkipCnt = 0;
   }

   PROFILER_BLOCK("loop1")

   int cnt = fMaxLoopCnt;

   while (cnt-- > 0) {

      PROFILER_BLOCKN("recv", 5)

      if (tgt != fMtuBuffer)
         tgt = fTgtPtr.ptr();

      /* this was old form which is not necessary - socket is already bind with the port */
      //  socklen_t socklen = sizeof(fSockAddr);
      //  ssize_t res = recvfrom(Socket(), fTgtPtr.ptr(), fMTU, 0, (sockaddr*) &fSockAddr, &socklen);

      ssize_t res = recv(Socket(), tgt, fMTU, MSG_DONTWAIT);

      PROFILER_BLOCK("chk")

      if (res == 0) {
         DOUT0("UDP:%d Seems to be, socket was closed", fNPort);
         return false;
      }

      if (res < 0) {
         // socket do not have data, one should enable event processing
         // otherwise we need to poll for the new data
         if (errno == EAGAIN) break;
         EOUT("Socket error");
         return false;
      }

      auto tu = static_cast<DogmaTu *>(tgt);
      auto msgsize = tu->GetSize(); // trb sender adds a 32 byte control trailer identical to event header

      std::string errmsg;

      if (res != msgsize) {
         errmsg = dabc::format("Send buffer %ld differ from message size %ld - ignore it", (long) res, (long) msgsize);
      } else if (!tu->IsMagic()) {
         fTotalDiscardMagic++;
         errmsg = "Magic number not match";
      }

      if (!errmsg.empty()) {
         fTotalDiscardPacket++;
         fTotalDiscardBytes += res;
         continue;
      }

      if (tgt == fMtuBuffer) {
         // skip single MTU
         fTotalDiscardPacket++;
         fTotalDiscardBytes += res;
         return false;
      }

      PROFILER_BLOCK("shift")

      if (fPrint)
         DOUT0("Event addr: %lu type: 0x%x trignum; %lu, time: %lu paylod: %lu",
            (long unsigned) tu->GetAddr(), (unsigned) tu->GetTrigType(), (long unsigned) tu->GetTrigNumber(), (long unsigned) tu->GetTrigTime(), (long unsigned) tu->GetPayloadLen());

      fTotalRecvPacket++;
      fTotalRecvBytes += res;

      fTgtPtr.shift(msgsize);


      PROFILER_BLOCK("buf2")

      auto rawsz = fTgtPtr.rawsize(); // remaining raw size

      // when rest size is smaller that mtu, one should close buffer
      // or if filled size bigger than allowed reduced size
      if ((rawsz < fMTU) || (fBufferSize - rawsz > fBufferSize * fReduce)) {
         CloseBuffer();
         tr->BufferReady();
         if (!tr->AssignNewBuffer(0, this))
            return false;
      }
   }

   PROFILER_BLOCK("ret")

   return true; // indicate that buffer reading will be finished by callback
}

int dogma::UdpAddon::OpenUdp(const std::string &host, int nport, int rcvbuflen)
{
   int fd = socket(PF_INET, SOCK_DGRAM, 0);
   if (fd < 0) return -1;

   if (!dabc::SocketThread::SetNonBlockSocket(fd)) {
      EOUT("Cannot set non-blocking mode for UDP socket %d", fd);
      close(fd);
      return -1;
   }

   if (rcvbuflen > 0) {
      uint64_t arg_rcvbuflen = (uint32_t) rcvbuflen;
      if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &arg_rcvbuflen, sizeof(arg_rcvbuflen)) == -1) {
         EOUT("Fail to setsockopt SO_RCVBUF %s", strerror(errno));
      }

      arg_rcvbuflen = 0;
      socklen_t arg_len = sizeof(arg_rcvbuflen);
      if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &arg_rcvbuflen, &arg_len) == -1) {
         EOUT("fail to getsockopt SO_RCVBUF, ...): %s", strerror(errno));
      }

      if ((int) arg_rcvbuflen < rcvbuflen) {
         EOUT("UDP receive buffer length (%u) smaller than requested buffer length (%d)", (unsigned) arg_rcvbuflen, rcvbuflen);
      } else if (rcvbuflen == (int) arg_rcvbuflen / 2) {
         DOUT0("SO_RCVBUF   Configured %d", rcvbuflen);
      } else {
         DOUT0("SO_RCVBUF   Configured %d  Actual %u", rcvbuflen, (unsigned) arg_rcvbuflen);
      }
   }

   if ((host.length() > 0) && (host != "host")) {
      struct addrinfo hints, *info = nullptr;

      memset(&hints, 0, sizeof(hints));
      hints.ai_flags    = AI_PASSIVE;
      hints.ai_family   = AF_UNSPEC; //AF_INET;
      hints.ai_socktype = SOCK_DGRAM;

      std::string service = std::to_string(nport);

      getaddrinfo(host.c_str(), service.c_str(), &hints, &info);

      if (info && bind(fd, info->ai_addr, info->ai_addrlen) == 0) return fd;
   }

   sockaddr_in addr;
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(nport);

   if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == 0) return fd;

   close(fd);
   return -1;
}


// ========================================================================================

dogma::UdpTransport::UdpTransport(dabc::Command cmd, const dabc::PortRef& inpport, UdpAddon* addon, double flush, double heartbeat) :
   dabc::Transport(cmd, inpport, nullptr),
   fIdNumber(0),
   fNumReadyBufs(0),
   fBufAssigned(false),
   fLastSendCnt(0)
{
   // do not process to much events at once, let another transports a chance
   SetPortLoopLength(OutputName(), 2);

   // low-down priority of module/port events, let process I/O events faster
   SetModulePriority(2);

   addon->SetIOPriority(1); // this is priority of main I/O events, higher than module events

   fIdNumber = inpport.ItemSubId();

   AssignAddon(addon);

   if (flush > 0)
      CreateTimer("FlushTimer", flush);

   if (heartbeat > 0)
      CreateTimer("HeartbeatTimer", heartbeat);

   DOUT3("Starting hadaq::DataTransport %s id %d", GetName(), fIdNumber);
}

dogma::UdpTransport::~UdpTransport()
{
}

int dogma::UdpTransport::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("ResetTransportStat")) {
      auto addon = static_cast<UdpAddon *>(fAddon());
      if (addon) addon->ClearCounters();
      return dabc::cmd_true;
   } else if (cmd.IsName("ReinitTransport")) {
      auto addon = static_cast<UdpAddon *>(fAddon());
      if (addon) {
         addon->CloseSocket();
         int fd = dogma::UdpAddon::OpenUdp(addon->fHostName, addon->fNPort, addon->fRecvBufLen);
         if (fd <= 0) {
            EOUT("Cannot recreate UDP socket for port %d", addon->fNPort);
            dabc::mgr.StopApplication();
         }
         addon->SetSocket(fd);
         addon->ClearCounters();
      }
      return dabc::cmd_true;
   } else if (cmd.IsName("GetDogmaTransportInfo")) {
      TransportInfo *info = (TransportInfo *) (dynamic_cast<UdpAddon*> (fAddon()));
      cmd.SetPtr("Info", info);
      cmd.SetUInt("UdpPort", info ? info->fNPort : 0);
      return dabc::cmd_true;
   } else if (cmd.IsName("TdcCalibrations") || cmd.IsName("CalibrRefresh")) {
      // ignore this command
      return dabc::cmd_true;
   }

   return dabc::Transport::ExecuteCommand(cmd);
}

bool dogma::UdpTransport::StartTransport()
{
   fLastDebugTm.GetNow();

   AssignNewBuffer(0); // provide immediately buffer - if possible

   fAddon.Notify("TransportWantToStart");

   return dabc::Transport::StartTransport();
}

bool dogma::UdpTransport::StopTransport()
{
   FlushBuffer(true);

   fAddon.Notify("TransportWantToStop");

   return dabc::Transport::StopTransport();
}

void dogma::UdpTransport::ProcessTimerEvent(unsigned timer)
{
   std::string name = TimerName(timer);
   if (name == "HeartbeatTimer") {
      auto addon = static_cast<UdpAddon *>(fAddon());

      if (addon) {
         addon->ReadUdp();
         addon->SetDoingInput(true);
         if (addon->fDebug) {
            addon->fUdpProfiler.MakeStatistic();
            addon->fProfilerInfo = addon->fUdpProfiler.Format();
         }
      }

   } else if (name == "FlushTimer") {
      FlushBuffer(false);

      auto addon = static_cast<UdpAddon *>(fAddon());

      if (addon && addon->fDebug && fLastDebugTm.Expired(1.)) {
         DOUT1("UDP %d NumReady:%u CanTake:%u BufAssigned:%s CanSend:%u DoingInp %s maxlooptm = %5.3f", fIdNumber, fNumReadyBufs, NumCanTake(0), DBOOL(fBufAssigned), NumCanSend(0), DBOOL(addon->IsDoingInput()), addon->fMaxProcDist);
         fLastDebugTm.GetNow();
         addon->fMaxProcDist = 0;
      }

   } else {
      dabc::Transport::ProcessTimerEvent(timer);
   }
}

bool dogma::UdpTransport::ProcessSend(unsigned port)
{
   if (fNumReadyBufs > 0) {
      dabc::Buffer buf = TakeBuffer(0);
      Send(port, buf);
      fNumReadyBufs--;
   }

   return fNumReadyBufs > 0;
}

bool dogma::UdpTransport::ProcessBuffer(unsigned pool)
{
   // check that required element available in the pool

   auto addon = static_cast<UdpAddon *>(fAddon());

   if (AssignNewBuffer(pool, addon)) {
      addon->ReadUdp();
      addon->SetDoingInput(true);
   }

   return false;
}

bool dogma::UdpTransport::AssignNewBuffer(unsigned pool, UdpAddon *addon)
{
   // assign  new buffer to the addon

   if (fBufAssigned || (NumCanTake(pool) <= fNumReadyBufs))
      return false;

   if (!addon)
      addon = static_cast<UdpAddon *>(fAddon());

   if (addon->HasBuffer()) {
      EOUT("should not happen");
      return false;
   }

   dabc::Buffer buf = PoolQueueItem(pool, fNumReadyBufs);
   if (buf.null()) {
      EOUT("Empty buffer when all checks already done - strange");
      CloseTransport(true);
      return false;
   }

   unsigned bufsize = (unsigned) buf.SegmentSize(0);

   addon->fBufferSize = bufsize;

   addon->fTgtPtr.reset(buf, 0, bufsize);

   fBufAssigned = true;

   if (addon->fTgtPtr.rawsize() < addon->fMTU) {
      EOUT("not enough space in the buffer - at least %u is required", addon->fMTU);
      CloseTransport(true);
      return false;
   }

   return true;
}

void dogma::UdpTransport::BufferReady()
{
   fBufAssigned = false;
   fNumReadyBufs++;

   while (CanSend(0))
      if (!ProcessSend(0)) break;
}

void dogma::UdpTransport::FlushBuffer(bool onclose)
{
   auto addon = static_cast<UdpAddon *>(fAddon());

   if (onclose || (fLastSendCnt == addon->fSendCnt)) {
      if (addon->CloseBuffer()) {
         BufferReady();
         if (!onclose) AssignNewBuffer(0, addon);
      }
   }

   fLastSendCnt = addon->fSendCnt;
}
