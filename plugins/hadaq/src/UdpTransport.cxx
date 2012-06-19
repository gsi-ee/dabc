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
#include "hadaq/Iterator.h"

#include "dabc/timing.h"
#include "dabc/Port.h"
#include "dabc/version.h"

#include <iostream>
#include <errno.h>

#include <math.h>


//#define UDP_PRINT_EVENTS


hadaq::UdpDataSocket::UdpDataSocket(dabc::Reference port) :
      dabc::SocketWorker(), dabc::Transport(port.Ref()), dabc::MemoryPoolRequester(), fQueueMutex(), fQueue(
            ((dabc::Port*) port())->InputQueueCapacity()), fTgtBuf()
{
   // we will react on all input packets
   SetDoingInput(true);

   fTgtShift = 0;
   fTgtPtr = 0;
   fEndPtr = 0;
   fBufferSize = 0;
   fTempBuf = 0;
//   fCurrentEvent = 0;

   fTotalRecvPacket = 0;
   fTotalDiscardPacket = 0;
   fTotalRecvMsg = 0;
   fTotalDiscardMsg = 0;
   fTotalRecvBytes = 0;
   fTotalRecvEvents =0;

   //fFlushTimeout = .1;
   fBufferSize = 2 * DEFAULT_MTU;
   fBuildFullEvent = false;

   ConfigureFor((dabc::Port*) port());
}

hadaq::UdpDataSocket::~UdpDataSocket()
{
   delete fTempBuf;
}

void hadaq::UdpDataSocket::ConfigureFor(dabc::Port* port)
{
   fBufferSize = port->Cfg(dabc::xmlBufferSize).AsInt(fBufferSize);
   //fFlushTimeout = port->Cfg(dabc::xmlFlushTimeout).AsDouble(fFlushTimeout);
   // DOUT0(("fFlushTimeout = %5.1f %s", fFlushTimeout, dabc::xmlFlushTimeout));

   fBuildFullEvent = port->Cfg(hadaq::xmlBuildFullEvent).AsBool(fBuildFullEvent);

   fPool = port->GetMemoryPool();

   fMTU = port->Cfg(hadaq::xmlMTUsize).AsInt(DEFAULT_MTU);
   delete fTempBuf;
   fTempBuf = new char[fMTU];

   int nport = port->Cfg(hadaq::xmlUdpPort).AsInt(0);
   std::string hostname = port->Cfg(hadaq::xmlUdpAddress).AsStdStr("0.0.0.0");
   int rcvBufLenReq = port->Cfg(hadaq::xmlUdpBuffer).AsInt(1 * (1 << 20));

   if (OpenUdp(nport, nport, nport, rcvBufLenReq) < 0) {
      EOUT(("hadaq::UdpDataSocket:: failed to open udp port %d with receive buffer %d", nport,rcvBufLenReq));
      CloseSocket();
      OnConnectionClosed();
      return;
   }
   DOUT0(("hadaq::UdpDataSocket:: Open udp port %d with rcvbuf %d, dabc buffer size = %u, buildfullevents=%d", nport, rcvBufLenReq, fBufferSize, fBuildFullEvent));
   std::cout <<"---------- fBuildFullEvent is "<< fBuildFullEvent << std::endl;
}

void hadaq::UdpDataSocket::ProcessEvent(const dabc::EventId& evnt)
{
   switch (evnt.GetCode()) {
      case evntSocketRead: {

         // this is required for DABC 2.0 to again enable read event generation
         SetDoingInput(true);
         ReadUdp();
         break;
      }

      default:
         dabc::SocketWorker::ProcessEvent(evnt);
         break;
   }
}

bool hadaq::UdpDataSocket::ReplyCommand(dabc::Command cmd)
{
//   int res = cmd.GetResult();
//
//   DOUT3(("hadaq::UdpDataSocket::ReplyCommand %s res = %s ", cmd.GetName(), DBOOL(res)));
//
//   //FireEvent(evntConfirmCmd, res==dabc::cmd_true ? 1 : 0);

   return true;
}

bool hadaq::UdpDataSocket::Recv(dabc::Buffer& buf)
{
   {
      dabc::LockGuard lock(fQueueMutex);
      if (fQueue.Size() <= 0)
         return false;

#if DABC_VERSION_CODE >= DABC_VERSION(1,9,2)
      fQueue.PopBuffer(buf);
#else
      buf << fQueue.Pop();
#endif
   }
   return !buf.null();
}

unsigned hadaq::UdpDataSocket::RecvQueueSize() const
{
   dabc::LockGuard guard(fQueueMutex);
   return fQueue.Size();
}

dabc::Buffer& hadaq::UdpDataSocket::RecvBuffer(unsigned indx) const
{
   dabc::LockGuard lock(fQueueMutex);
   return fQueue.ItemRef(indx);
}

bool hadaq::UdpDataSocket::ProcessPoolRequest()
{
   //FireEvent(evntFillBuffer);
   return true;
}

void hadaq::UdpDataSocket::StartTransport()
{
   DOUT0(("Starting UDP transport "));
   //FireEvent(evntStartTransport);
   NewReceiveBuffer();
}

void hadaq::UdpDataSocket::StopTransport()
{
   DOUT0(("Stopping hada:udp transport -"));
   // FIXME: again, we see strange things in DOUT, wrong or shifted values!
   //DOUT0(("RecvPackets:%u, DiscPackets:%u, RecvMsg:%u, DiscMsg:%u, RecvBytes:%u",
   //       fTotalRecvPacket, fTotalDiscardPacket, fTotalRecvMsg, fTotalDiscardMsg, fTotalRecvBytes));
   std::cout << "RecvPackets:" << fTotalRecvPacket << ", DiscPackets:"
         << fTotalDiscardPacket << ", RecvMsg:" << fTotalRecvMsg << ", DiscMsg:"
         << fTotalDiscardMsg << ", RecvBytes:" << fTotalRecvBytes<< ", fTotalRecvEvents:"<< fTotalRecvEvents<< std::endl;

   //FireEvent(evntStopTransport);

}

double hadaq::UdpDataSocket::ProcessTimeout(double)
{

   return 0.01;
}

int hadaq::UdpDataSocket::GetParameter(const char* name)
{
//   if ((strcmp(name, hadaq::xmlRocNumber)==0) && fDev) return fDev->rocNumber();
//   if (strcmp(name, hadaq::xmlMsgFormat)==0) return fFormat;
//   if (strcmp(name, hadaq::xmlTransportKind)==0) return roc::kind_UDP;

   return dabc::Transport::GetParameter(name);
}

void hadaq::UdpDataSocket::ReadUdp()
{
   // this function is a "translation" to dabc of the hadaq nettrans.c assembleMsg()
   if (fTgtPtr == 0) {
      // this will happen if the udp senders are active before our transport is configured
      //EOUT(("hadaq::UdpDataSocket::ReadUdp NEVER COME HERE: zero receive pointer"));
      return;
   }
   DOUT5(("ReadUdp before DoUdpRecvFrom"));
   ssize_t len = DoUdpRecvFrom(fTgtPtr + fTgtShift, fMTU); // request with full mtu buffer
   // check here length and validitiy:
   if (len < 0) {
      EOUT(("hadaq::UdpDataSocket::ReadUdp failed after recvfrom"));
      return; // NEVER COME HERE, DoUdpRecvFrom should have handled this!
   }

   if (fTgtBuf.null()) {
      // we used dummy buffer for receive
      fTgtShift = 0;
      fTotalDiscardPacket++;
   } else {
      // regular received into dabc buffer:
      fTgtShift += len;
      fTotalRecvPacket++;
   }
   hadaq::HadTu* hadTu = (hadaq::HadTu*) fTgtPtr;
   size_t msgsize = hadTu->GetSize() + 32; // trb sender adds a 32 byte control trailer identical to event header
   if ((size_t) len == fMTU) {
      // hadtu send from trb is bigger than mtu, we expect end of messages in the next recvfrom
      if ((size_t)(fEndPtr - fTgtPtr) < msgsize) {
         // rest of expected message does not fit anymore in current buffer
         NewReceiveBuffer(true); // copy the spanning fragment to new dabc buffer
      } //   if ((size_t)(fEndPtr - fTgtPtr
   } else {
      //std::cout << "!!!! fTgtShift:" << fTgtShift << ", hadTU size:"
      //      << hadTu->GetSize() << std::endl;
      // we finished receive with current hadtu message, check it:
      if (fTgtShift != msgsize
            || memcmp((char*) hadTu + hadTu->GetSize(), (char*) hadTu, 32)) {
         // received size does not match header info, or header!=trailer contents
         fTgtShift = 0;
         fTotalDiscardMsg++;
         std::cout << "discarded message:" << fTotalDiscardMsg << std::endl;

      } else {
         fTotalRecvMsg++;
         fTotalRecvBytes += fTgtShift;
         //fTgtPtr += fTgtShift;
         fTgtPtr += hadTu->GetSize(); // we will overwrite the trailing block of this message again
         fTgtShift = 0;
         if (fTgtBuf.null() || (size_t)(fEndPtr - fTgtPtr) < fMTU) {
            NewReceiveBuffer(false); // no more space for full mtu in buffer, finalize old and get new:
         }

//         else {
//            NextEvent(); // just finalize old event and insert new header at current tgtptr
//         }
      } // if (fTgtShift != msgsize

   } //if (len == fMTU)

}

void hadaq::UdpDataSocket::NewReceiveBuffer(bool copyspanning)
{

   DOUT5(("NewReceiveBuffer, copyspanning=%d",copyspanning));
   dabc::Buffer outbuf;
   if (!fTgtBuf.null()) {
      // first shrink last receive buffer to full received hadtu envelopes:
      // to cut last trailing event header, otherwise we get trouble with read iterator for evtbuilding
      unsigned filledsize = fTgtPtr - (char*) fTgtBuf.SegmentPtr();
      fTgtBuf.SetTotalSize(filledsize);

      // then we optionally format the output buffer

      if (fBuildFullEvent) {
         // in simple eventbuilding mode, we have to scan old receive buffer for actual subevents
         // and add an hades event header for each of those

         fEvtBuf = fPool.TakeBufferReq(this, fBufferSize);
         if (fEvtBuf.null()) {
            EOUT(("hadaq::UdpDataSocket::NewReceiveBuffer - could not take event buffer of size %d ",fBufferSize));
            OnConnectionClosed(); // FIXME: better error handling here!

         }
         fEvtBuf.SetTypeId(hadaq::mbt_HadaqEvents); // buffer will contain events with subevents
         fEvtPtr = (char*) fEvtBuf.SegmentPtr();
         fEvtEndPtr = (char*) fEvtBuf.SegmentPtr() + fEvtBuf.SegmentSize();

         outbuf = fEvtBuf;
         FillEventBuffer();


      } else {
         // no event building: pass receive buffer over as it is
         outbuf = fTgtBuf;
      }
   } //  if (!fTgtBuf.null()) oldbuffer

// now get new receive buffer
   fTgtBuf = fPool.TakeBufferReq(this, fBufferSize);
   if (!fTgtBuf.null()) {
      fTgtBuf.SetTypeId(hadaq::mbt_HadaqTransportUnit); // raw transport unit with subevents as from trb

      char* bufstart = (char*) fTgtBuf.SegmentPtr(); // is modified by puteventheader
      if (copyspanning) {
         // copy data with full NetTransx event header for spanning events anyway
         size_t copylength = fTgtShift;
//         if (fBuildFullEvent)
//            copylength += sizeof(hadaq::Event);
         memcpy(bufstart, fTgtPtr, copylength);
         DOUT0(
               ("Copied %d spanning bytes to new DABC buffer of size %d",copylength,fTgtBuf.SegmentSize()));
         fTgtPtr = bufstart;
//         if (fBuildFullEvent)
//            fTgtPtr += sizeof(hadaq::Event);
//         fCurrentEvent = (hadaq::Event*) bufstart;
         // note that we do keep old fTgtShift for the spanning event
      } else {
         fTgtPtr = bufstart;
         //NextEvent();
      }
      fEndPtr = (char*) fTgtBuf.SegmentPtr() + fTgtBuf.SegmentSize();
      DOUT3(("NewReceiveBuffer gets fTgtPtr: %x, fEndPtr: %x",fTgtPtr, fEndPtr));
      if (fTgtBuf.SegmentSize() < fMTU) {
         EOUT(
               ("hadaq::UdpDataSocket::NewReceiveBuffer - buffer segment size %d < mtu $d",fTgtBuf.SegmentSize(),fMTU));
         OnConnectionClosed(); // FIXME: better error handling here!
      }

   } else {
      //fCurrentEvent = (hadaq::Event*) fTempBuf;
      fTgtPtr = fTempBuf;
      fEndPtr = fTempBuf + sizeof(fTempBuf);
      fTgtShift = 0;
      DOUT0(
            ("hadaq::UdpDataSocket:: No pool buffer available for socket read, use internal dummy!"));
   } //if (!fTgtBuf.null()) newbuffer

   if (!outbuf.null()) {
      fQueue.Push(outbuf); // put old buffer to transport queue no sooner than we have copied spanning event
      FirePortInput();
   }

}




void  hadaq::UdpDataSocket::FillEventBuffer()
{
   // TODO: implement iterator functions for hadtu buffer
   //         hadaq::ReadIterator hiter(fTgtBuf);
   //         //hadaq::Event* hev;
   //         while (hiter.NextEvent()) {
   //            //hadaq::Event* hev=hiter.evt;
   //            // udp envelope is hades event header (32 bytes)
   //            while (hiter.NextSubEvent()) {
   //               hadaq::Subevent* source = hiter.subevnt();
   /////////////////////////////////////////////
             // manual approach:
            char* cursor=(char*) fTgtBuf.SegmentPtr();
            size_t bufsize=fTgtBuf.SegmentSize();
            while(bufsize> sizeof(hadaq::HadTu) + sizeof(hadaq::Subevent)){

               // outer loop get envelope header: This is a plain HadTu header!

               hadaq::HadTu* hev= (hadaq::Subevent*) cursor;
               size_t hevsize=hev->GetSize();
#ifdef UDP_PRINT_EVENTS
               std::cout <<"Container  0x"<< std::hex<< (int)hev <<" of size"<< std::dec<< hevsize << std::endl;
               char* ptr= cursor;
               for(int i=0; i<16; ++i)
                    {
                       std::cout <<"evt("<<i<<")=0x"<< (std::hex)<< ( (short) *(ptr+i) & 0xff ) <<" , ";
                    }
               std::cout <<(std::dec)<< std::endl;
#endif
               cursor += sizeof(hadaq::HadTu);
               bufsize -=sizeof(hadaq::HadTu);
               while(hevsize>sizeof(hadaq::Subevent)){
                   // inner loop scans actual subevents
                   hadaq::Subevent* sev= (hadaq::Subevent*) cursor;
                   size_t sublen = sev->GetSize(); // real payload of subevent
                   size_t padlen = sev->GetPaddedSize(); // actual copied data length, 8 byte padded
                   size_t evlen = sublen + sizeof(hadaq::Event);
                   // DUMP EVENT
#ifdef UDP_PRINT_EVENTS
                   std::cout <<" subevent 0x"<< std::hex<< (int)sev <<" of size "<< std::dec << sublen <<", padded size "<< std::dec << padlen<< std::endl;
                   char* ptr= cursor + sizeof(hadaq::Subevent);
                   for(int i=0; i<20; ++i)
                       {
                          std::cout <<"sub("<<i<<")=0x"<< (std::hex)<< ( (short) *(ptr+i) & 0xff ) <<" , ";
                       }
                  std::cout <<(std::dec)<< std::endl;
#endif
                  ///////////////


                  // each subevent of single stream is wrapped into event header
                  size_t subrest = (size_t)(fEvtEndPtr - fEvtPtr);
                  if (subrest < evlen) {
                     DOUT0(("hadaq::UdpDataSocket:: Remaining subevents do not fit in Event output buffer of size %d! Drop them. Check mempool setup",fEvtBuf.SegmentSize()));
                     break;
                     // TODO: dynamic spanning of output buffer. Maybe left to full combiner module
                  }

                  // TODO: handle this with write iterator later
                  hadaq::Event* curev = hadaq::Event::PutEventHeader(&fEvtPtr,
                        EvtId_data);
                  memcpy(fEvtPtr, (char*) sev, padlen);
                  curev->SetSize(evlen); // only one subevent in this mode.
                  // currId = currId | (DAQVERSION << 12);
                  uint32_t currId = 0x00003001; // we define DAQVERSION 3 for DABC
                  curev->SetId(currId);
                  curev->SetSeqNr(fTotalRecvEvents++);
                  curev->SetRunNr(0); // TODO: do we need to set from parameter? maybe left to eventbuilder module later.
#ifdef UDP_PRINT_EVENTS
                  std::cout << "Build NextEvent " << fTotalRecvEvents
                        << " with curev:" << (int) curev << ",fEvtPtr:"
                        << (int) fEvtPtr;
                  std::cout << ", evlen:" << evlen << std::endl;
#endif

                  fEvtPtr += padlen;
                  cursor += padlen;
                  hevsize -= padlen;
                  bufsize -=padlen;

               } // while next subevent
               //break; // use only first hadtu in buffer
            } // while next hadtu

            unsigned filledsize = fEvtPtr - (char*) fEvtBuf.SegmentPtr();
            fEvtBuf.SetTotalSize(filledsize);
}




ssize_t hadaq::UdpDataSocket::DoUdpRecvFrom(void* buf, size_t len, int flags)
{
   ssize_t res = 0;
   size_t socklen = (size_t) sizeof(fSockAddr);
   sockaddr* saptr = (sockaddr*) &fSockAddr; // trick for compiler
   //std::cout <<"recvfrom writes to "<<(int) buf <<", len="<<len<< std::endl;
   res = recvfrom(fSocket, buf, len, flags, saptr, (socklen_t *) &socklen);
   //std::cout <<"recvfrom returns"<<res << std::endl;
   if (res == 0)
      OnConnectionClosed();
   else if (res < 0) {
      if (errno != EAGAIN)
         OnSocketError(errno, "When recvfrom()");
   }
   return res;
}

int hadaq::UdpDataSocket::OpenUdp(int& portnum, int portmin, int portmax,
      int& rcvBufLenReq)
{

   int fd = socket(PF_INET, SOCK_DGRAM, 0);
   if (fd < 0)
      return -1;
   SetSocket(fd);
   int numtests = 1; // at least test value of portnum
   if ((portmin > 0) && (portmax > 0) && (portmin <= portmax))
      numtests += (portmax - portmin + 1);

   if (dabc::SocketThread::SetNonBlockSocket(fd))
      for (int ntest = 0; ntest < numtests; ntest++) {
         if ((ntest == 0) && (portnum < 0))
            continue;
         if (ntest > 0)
            portnum = portmin - 1 + ntest;

         memset(&fSockAddr, 0, sizeof(fSockAddr));
         fSockAddr.sin_family = AF_INET;
         fSockAddr.sin_port = htons(portnum);

         if (rcvBufLenReq != 0) {
            // for hadaq application: set receive buffer length _before_ bind:
            //         int rcvBufLenReq = 1 * (1 << 20);
            int rcvBufLenRet;
            size_t rcvBufLenLen = (size_t) sizeof(rcvBufLenReq);
            if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvBufLenReq,
                  rcvBufLenLen) == -1) {
               EOUT(( "setsockopt(..., SO_RCVBUF, ...): %s", strerror(errno)));
            }

            if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvBufLenRet,
                  (socklen_t *) &rcvBufLenLen) == -1) {
               EOUT(( "getsockopt(..., SO_RCVBUF, ...): %s", strerror(errno)));
            }
            if (rcvBufLenRet < rcvBufLenReq) {
               EOUT(("UDP receive buffer length (%d) smaller than requested buffer length (%d)", rcvBufLenRet, rcvBufLenReq));
               rcvBufLenReq = rcvBufLenRet;
            }
         }

         if (!bind(fd, (struct sockaddr *) &fSockAddr, sizeof(fSockAddr)))
            return fd;
      }
   CloseSocket();
   //close(fd);
   return -1;
}

//void hadaq::UdpDataSocket::NextEvent()
//{
//   if (fBuildFullEvent) {
//      // first finalize old event:
//      if (fCurrentEvent) {
//         hadaq::Subevent* sub = (hadaq::Subevent*) ((char*) (fCurrentEvent)
//               + sizeof(hadaq::Event));
//
//         fCurrentEvent->SetSize(sub->GetSize() + sizeof(hadaq::Event)); // only one subevent in this mode.
//         // currId = currId | (DAQVERSION << 12);
//         uint32_t currId = 0x00003001; // we define DAQVERSION 3 for DABC
//         fCurrentEvent->SetId(currId);
//         fCurrentEvent->SetSeqNr(fTotalRecvEvents++);
//         fCurrentEvent->SetRunNr(0); // TODO: do we need to set from parameter? maybe left to eventbuilder module later.
//      }
//      // now set up next event header
//      char* bufstart = fTgtPtr; // is modified by puteventheader
//      fCurrentEvent = hadaq::Event::PutEventHeader(&bufstart, EvtId_data);
//      fTgtPtr = bufstart;
//   } else {
//      fCurrentEvent = (hadaq::Event*) fTgtPtr;
//   }
//   fTgtShift = 0;
////   std::cout <<"NextEvent with fCurrentEvent:"<<(int) fCurrentEvent<<",fTgtPtr:"<< (int) fTgtPtr;
////   std::cout << ", fTgtShift:"<< fTgtShift<<std::endl;
//
//}

