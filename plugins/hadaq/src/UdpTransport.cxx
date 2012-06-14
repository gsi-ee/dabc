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

#include <iostream>
#include <errno.h>

#include <math.h>


hadaq::UdpDataSocket::UdpDataSocket(dabc::Reference port) :
   dabc::SocketWorker(),
   dabc::Transport(port.Ref()),
   dabc::MemoryPoolRequester(),
   fQueueMutex(),
   fQueue(((dabc::Port*) port())->InputQueueCapacity()),
   fTgtBuf()
{
   // we will react on all input packets
   SetDoingInput(true);

   fTgtShift = 0;
   fTgtPtr = 0;
   fEndPtr =0;
   fBufferSize = 0;
   fTempBuf=0;
   fTotalRecvPacket = 0;
   fTotalDiscardPacket=0;
   fTotalRecvMsg=0;
   fTotalDiscardMsg=0;
   fTotalRecvBytes=0;



   fFlushTimeout = .1;
   fBufferSize = 2*DEFAULT_MTU;


   ConfigureFor((dabc::Port*) port());
}

hadaq::UdpDataSocket::~UdpDataSocket()
{
   delete fTempBuf;
}

void hadaq::UdpDataSocket::ConfigureFor(dabc::Port* port)
{
   fBufferSize = port->Cfg(dabc::xmlBufferSize).AsInt(fBufferSize);
   fFlushTimeout = port->Cfg(dabc::xmlFlushTimeout).AsDouble(fFlushTimeout);
   // DOUT0(("fFlushTimeout = %5.1f %s", fFlushTimeout, dabc::xmlFlushTimeout));

   fPool = port->GetMemoryPool();

   fMTU=port->Cfg(hadaq::xmlMTUsize).AsInt(DEFAULT_MTU);
   delete fTempBuf;
   fTempBuf = new char[fMTU];

   int nport= port->Cfg(hadaq::xmlUdpPort).AsInt(0);
   std::string hostname = port->Cfg(hadaq::xmlUdpAddress).AsStdStr("0.0.0.0");
   int rcvBufLenReq = port->Cfg(hadaq::xmlUdpBuffer).AsInt(1 * (1 << 20));


   if(OpenUdp(nport, nport, nport, rcvBufLenReq)<0)
      {
          EOUT(("hadaq::UdpDataSocket:: failed to open udp port %d with receive buffer %d", nport,rcvBufLenReq));
          CloseSocket();
          OnConnectionClosed();
          return;
      }
   DOUT0(("hadaq::UdpDataSocket:: Open udp port %d with rcvbuf %d, dabc buffer size = %u", nport, rcvBufLenReq, fBufferSize));
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

//      case evntStartTransport: {
//
//         break;
//      }
//
//      case evntStopTransport: {
//
//         break;
//      }
//
//      case evntConfirmCmd: {
//         }
//         break;
//      }
//
//      case evntFillBuffer:
////         AddBuffersToQueue();
//         break;

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
      if (fQueue.Size()<=0) return false;

   #if DABC_VERSION_CODE >= DABC_VERSION(1,9,2)
      fQueue.PopBuffer(buf);
    #else
      buf << fQueue.Pop();
    #endif
   }
   //FireEvent(evntFillBuffer);

   return !buf.null();
}

unsigned  hadaq::UdpDataSocket::RecvQueueSize() const
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
   std::cout <<"RecvPackets:"<<fTotalRecvPacket<<", DiscPackets:"<< fTotalDiscardPacket<<", RecvMsg:"<<fTotalRecvMsg<<", DiscMsg:"<< fTotalDiscardMsg<<", RecvBytes:"<<fTotalRecvBytes<<std::endl;

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
      EOUT(("hadaq::UdpDataSocket::ReadUdp NEVER COME HERE: zero receive pointer"));
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
      if((size_t) (fEndPtr-fTgtPtr) < msgsize)
         {
            // rest of expected message does not fit anymore in current buffer
            NewReceiveBuffer(true); // copy the spanning fragment to new dabc buffer
         }
   } else {
      //std::cout <<"!!!! fTgtShift:"<<fTgtShift<<", hadTU size:"<< hadTu->GetSize()<< std::endl;
      // we finished receive with current hadtu message, check it:
      if (fTgtShift != msgsize || memcmp((char*) hadTu + hadTu->GetSize(), (char*) hadTu, 32)) {
          // received size does not match header info, or header!=trailer contents
          fTotalDiscardMsg++;
          std::cout <<"discarded message:"<<fTotalDiscardMsg<<std::endl;

      } else {
         fTotalRecvMsg++;
         fTotalRecvBytes += fTgtShift;
         //fTgtPtr += fTgtShift;
         fTgtPtr +=hadTu->GetSize(); // we will overwrite the trailing block of this message again
      }
      fTgtShift = 0;
      if(fTgtBuf.null() || (size_t)(fEndPtr-fTgtPtr) < fMTU)
      {
         NewReceiveBuffer(false); // no more space for full mtu in buffer, finalize it:
      }

   } //if (len == fMTU)


}

void  hadaq::UdpDataSocket::NewReceiveBuffer(bool copyspanning)
{

   DOUT5(("NewReceiveBuffer"));
   dabc::Buffer oldbuf=fTgtBuf;
   if(!oldbuf.null())
   {
      unsigned filledsize=fTgtPtr- (char*) fTgtBuf.SegmentPtr();
      oldbuf.SetTotalSize(filledsize);
   }


   fTgtBuf = fPool.TakeBufferReq(this, fBufferSize);
   if (!fTgtBuf.null())
       {
          fTgtBuf.SetTypeId(hadaq::mbt_HadaqEvents);
          if(copyspanning)
             {
                memcpy(fTgtBuf.SegmentPtr(),fTgtPtr, fTgtShift);
                DOUT0(("Copied %d spanning bytes to new DABC buffer of size %d",fTgtShift,fTgtBuf.SegmentSize()));
             }
          else
             {
                fTgtShift = 0; // note that we do keep shift in the other case of a spanning event
             }
          fTgtPtr = (char*) fTgtBuf.SegmentPtr();
          fEndPtr = fTgtPtr+ fTgtBuf.SegmentSize();
          DOUT5(("NewReceiveBuffer gets fTgtPtr: %x, fEndPtr: %x",fTgtPtr, fEndPtr));
          if(fTgtBuf.SegmentSize()<fMTU)
             {
             EOUT(("hadaq::UdpDataSocket::NewReceiveBuffer - buffer segment size %d < mtu $d",fTgtBuf.SegmentSize(),fMTU));
             OnConnectionClosed(); // FIXME: better error handling here!
             }
       }
   else
       {

          fTgtPtr = fTempBuf;
          fEndPtr =  fTempBuf + sizeof(fTempBuf);
          DOUT0(("hadaq::UdpDataSocket:: No pool buffer available for socket read, use internal dummy!"));
       }

    if(!oldbuf.null())
    {
       fQueue.Push(oldbuf); // old buffer to transport receive queue no sooner than have copied spanning event
       FirePortInput();
    }

}



ssize_t hadaq::UdpDataSocket::DoUdpRecvFrom(void* buf, size_t len, int flags)
{
   ssize_t res = 0;
   size_t socklen = (size_t) sizeof(fSockAddr);
   sockaddr* saptr= (sockaddr*) &fSockAddr; // trick for compiler
   res = recvfrom(fSocket, buf, len, flags, saptr, (socklen_t *) &socklen);
   if (res == 0)
      OnConnectionClosed();
   else if (res < 0) {
      if (errno != EAGAIN)
         OnSocketError(errno, "When recvfrom()");
   }
   return res;
}


int hadaq::UdpDataSocket::OpenUdp(int& portnum, int portmin, int portmax, int& rcvBufLenReq)
{

   int fd = socket(PF_INET, SOCK_DGRAM, 0);
   if (fd<0) return -1;
   SetSocket(fd);
   int numtests = 1; // at least test value of portnum
   if ((portmin>0) && (portmax>0) && (portmin<=portmax)) numtests+=(portmax-portmin+1);

   if (dabc::SocketThread::SetNonBlockSocket(fd))
      for(int ntest=0;ntest<numtests;ntest++) {
         if ((ntest==0) && (portnum<0)) continue;
         if (ntest>0) portnum = portmin - 1 + ntest;

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

            if (getsockopt(fd , SOL_SOCKET, SO_RCVBUF, &rcvBufLenRet, (socklen_t *) &rcvBufLenLen) == -1) {
               EOUT(( "getsockopt(..., SO_RCVBUF, ...): %s", strerror(errno)));
            }
            if (rcvBufLenRet < rcvBufLenReq) {
               EOUT(("UDP receive buffer length (%d) smaller than requested buffer length (%d)", rcvBufLenRet, rcvBufLenReq));
               rcvBufLenReq=rcvBufLenRet;
            }
         }


         if (!bind(fd, (struct sockaddr *)&fSockAddr, sizeof(fSockAddr))) return fd;
      }
   CloseSocket();
   //close(fd);
   return -1;
}
