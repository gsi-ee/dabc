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
#include <stdlib.h>
#include <sched.h>

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>

#include "dabc/timing.h"
#include "dabc/Manager.h"

// according to specification maximal UDP packet is 65,507 or 0xFFE3
#define DEFAULT_MTU 0xFFF0

hadaq::NewAddon::NewAddon(int fd, int nport, int mtu, bool debug, int maxloop, double reduce, double lost) :
   dabc::SocketAddon(fd),
   TransportInfo(nport),
   fTgtPtr(),
   fMTU(mtu > 0 ? mtu : DEFAULT_MTU),
   fMtuBuffer(nullptr),
   fSkipCnt(0),
   fSendCnt(0),
   fMaxLoopCnt(maxloop > 1 ? maxloop : 1),
   fReduce(reduce < 0.1 ? 0.1 : (reduce > 1. ? 1. : reduce)),
   fLostRate(lost),
   fLostCnt(lost>0 ? 1 : -1),
   fDebug(debug),
   fRunning(false)
{
   fMtuBuffer = malloc(fMTU);
}

hadaq::NewAddon::~NewAddon()
{
   free(fMtuBuffer);
}

void hadaq::NewAddon::ProcessEvent(const dabc::EventId& evnt)
{
   if (evnt.GetCode() == evntSocketRead) {

      // DOUT0("Addon %d get read event", fNPort);

      // ignore events when not waiting for the new data
      if (fRunning) { ReadUdp(); SetDoingInput(true); }
   } else {
      dabc::SocketAddon::ProcessEvent(evnt);
   }
}

long hadaq::NewAddon::Notify(const std::string &msg, int arg)
{
   if (msg == "TransportWantToStart") {
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


bool hadaq::NewAddon::CloseBuffer()
{
   if (fTgtPtr.null()) return false;
   unsigned fill_sz = fTgtPtr.distance_to_ownbuf();
   if (fill_sz == 0) return false;

   fTgtPtr.buf().SetTypeId(hadaq::mbt_HadaqTransportUnit);
   fTgtPtr.buf().SetTotalSize(fill_sz);
   fTgtPtr.reset();

   fSendCnt++;
   fTotalProducedBuffers++;
   return true;
}


bool hadaq::NewAddon::ReadUdp()
{
   if (!fRunning) return false;

   hadaq::NewTransport* tr = dynamic_cast<hadaq::NewTransport*> (fWorker());
   if (!tr) { EOUT("No transport assigned"); return false; }

   void *tgt = nullptr;

   if (fTgtPtr.null()) {
      if (!tr->AssignNewBuffer(0, this)) {
         if (fSkipCnt++<10) { fTotalArtificialSkip++; return false; }
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

   int cnt = fMaxLoopCnt;

   while (cnt-- > 0) {

      if (tgt != fMtuBuffer) tgt = fTgtPtr.ptr();

      /* this was old form which is not necessary - socket is already bind with the port */
      //  socklen_t socklen = sizeof(fSockAddr);
      //  ssize_t res = recvfrom(Socket(), fTgtPtr.ptr(), fMTU, 0, (sockaddr*) &fSockAddr, &socklen);

      ssize_t res = recv(Socket(), tgt, fMTU, MSG_DONTWAIT);

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

      if ((fLostCnt > 0) && (--fLostCnt == 0)) {
         // artificial drop of received UDP packet
         fLostCnt = (int) (1 / fLostRate * (0.5 + 1.* rand() / RAND_MAX));
         if (fLostCnt < 3) fLostCnt = 3;
         fTotalArtificialLosts++;
         continue;
      }

      hadaq::HadTu* hadTu = (hadaq::HadTu*) tgt;
      int msgsize = hadTu->GetPaddedSize() + 32; // trb sender adds a 32 byte control trailer identical to event header

      std::string errmsg;

      if (res != msgsize) {
         errmsg = dabc::format("Send buffer %d differ from message size %d - ignore it", res, msgsize);
      } else
      if (memcmp((char*) hadTu + hadTu->GetPaddedSize(), (char*) hadTu, 32) != 0) {
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

      if (tgt == fMtuBuffer) {
         // skip single MTU
         fTotalDiscardPacket++;
         fTotalDiscardBytes+=res;
         return false;
      }

      fTotalRecvPacket++;
      fTotalRecvBytes += res;

      fTgtPtr.shift(hadTu->GetPaddedSize());

      // when rest size is smaller that mtu, one should close buffer
      if ((fTgtPtr.rawsize() < fMTU) || (fTgtPtr.consumed_size() > fReduce)) {
         CloseBuffer();
         tr->BufferReady();
         if (!tr->AssignNewBuffer(0,this)) return false;
      }
   }

   return true; // indicate that buffer reading will be finished by callback
}

int hadaq::NewAddon::OpenUdp(const std::string &host, int nport, int rcvbuflen)
{
   int fd = socket(PF_INET, SOCK_DGRAM, 0);
   if (fd < 0) return -1;

   if (!dabc::SocketThread::SetNonBlockSocket(fd)) {
      EOUT("Cannot set non-blocking mode for UDP socket %d", fd);
      close(fd);
      return -1;
   }

   if (rcvbuflen > 0) {
      // for hadaq application: set receive buffer length _before_ bind:
      //         int rcvBufLenReq = 1 * (1 << 20);
      socklen_t rcvBufLenLen = sizeof(rcvbuflen);
      if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuflen, rcvBufLenLen) == -1) {
         EOUT("Fail to setsockopt SO_RCVBUF %s", strerror(errno));
      }

      int rcvBufLenRet = 0;
      if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvBufLenRet, &rcvBufLenLen) == -1) {
         EOUT("fail to getsockopt SO_RCVBUF, ...): %s", strerror(errno));
      }

      if (rcvBufLenRet < rcvbuflen) {
         EOUT("UDP receive buffer length (%d) smaller than requested buffer length (%d)", rcvBufLenRet, rcvbuflen);
         rcvbuflen = rcvBufLenRet;
      } else {
         DOUT0("SO_RCVBUF   Configured %ld  Actual %ld", (long) rcvbuflen, (long) rcvBufLenRet);
      }
   }

   if ((host.length()>0) && (host!="host")) {
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

hadaq::NewTransport::NewTransport(dabc::Command cmd, const dabc::PortRef& inpport, NewAddon* addon, double flush) :
   dabc::Transport(cmd, inpport, 0),
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

   DOUT3("Starting hadaq::DataTransport %s id %d", GetName(), fIdNumber);
}

hadaq::NewTransport::~NewTransport()
{
}

int hadaq::NewTransport::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("ResetTransportStat")) {
      NewAddon *addon = dynamic_cast<NewAddon *> (fAddon());
      if (addon) addon->ClearCounters();
      return dabc::cmd_true;
   }

   if (cmd.IsName("GetHadaqTransportInfo")) {
      TransportInfo *info = (TransportInfo *) (dynamic_cast<NewAddon*> (fAddon()));
      cmd.SetPtr("Info", info);
      cmd.SetUInt("UdpPort", info ? info->fNPort : 0);
      return dabc::cmd_true;
   }

   if (cmd.IsName("TdcCalibrations")) {
      // ignore this command
      return dabc::cmd_true;
   }

   return dabc::Transport::ExecuteCommand(cmd);
}


bool hadaq::NewTransport::StartTransport()
{
   AssignNewBuffer(0); // provide immediately buffer - if possible

   fAddon.Notify("TransportWantToStart");

   return dabc::Transport::StartTransport();
}

bool hadaq::NewTransport::StopTransport()
{
   FlushBuffer(true);

   fAddon.Notify("TransportWantToStop");

   return dabc::Transport::StopTransport();
}

void hadaq::NewTransport::ProcessTimerEvent(unsigned timer)
{
   if (TimerName(timer) == "FlushTimer") {
      FlushBuffer(false);
   } else {
      dabc::Transport::ProcessTimerEvent(timer);
   }
}

bool hadaq::NewTransport::ProcessSend(unsigned port)
{
   if (fNumReadyBufs > 0) {
      dabc::Buffer buf = TakeBuffer(0);
      Send(port, buf);
      fNumReadyBufs--;
   }

   return fNumReadyBufs > 0;
}

bool hadaq::NewTransport::ProcessBuffer(unsigned pool)
{
   // check that required element available in the pool

   NewAddon* addon = (NewAddon*) fAddon();

   if (AssignNewBuffer(pool, addon))
      addon->ReadUdp();

   return false;
}

bool hadaq::NewTransport::AssignNewBuffer(unsigned pool, NewAddon* addon)
{
   // assign  new buffer to the addon

   if (fBufAssigned || (NumCanTake(pool) <= fNumReadyBufs)) return false;

   if (!addon) addon = (NewAddon*) fAddon();

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

   addon->fTgtPtr.reset(buf, 0, bufsize);

   fBufAssigned = true;

   if (addon->fTgtPtr.rawsize() < addon->fMTU) {
      EOUT("not enough space in the buffer - at least %u is required", addon->fMTU);
      CloseTransport(true);
      return false;
   }

   return true;
}

void hadaq::NewTransport::BufferReady()
{
   fBufAssigned = false;
   fNumReadyBufs++;

   while (CanSend(0))
      if (!ProcessSend(0)) break;
}

void hadaq::NewTransport::FlushBuffer(bool onclose)
{
   NewAddon* addon = dynamic_cast<NewAddon*> (fAddon());

   if (onclose || (fLastSendCnt == addon->fSendCnt)) {
      if (addon->CloseBuffer()) {
         BufferReady();
         if (!onclose) AssignNewBuffer(0, addon);
      }
   }

   fLastSendCnt = addon->fSendCnt;
}
