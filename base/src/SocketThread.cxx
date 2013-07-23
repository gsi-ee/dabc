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

#include "dabc/SocketThread.h"

#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Buffer.h"
#include "dabc/SocketDevice.h"

const char* SocketErr(int err)
{
   switch (err) {
      case -1: return "Internal";
      case EAGAIN: return "EAGAIN";
      case EBADF:  return "EBADF";
      case ECONNREFUSED: return "ECONNREFUSED";
      case EFAULT: return "EFAULT";
      case EINTR: return "EINTR";
      case EINVAL: return "EINVAL";
      case ENOMEM: return "ENOMEM";
      case ENOTCONN: return "ENOTCONN";
      case ENOTSOCK:  return "ENOTSOCK";;

      case EACCES:  return "EACCES";
      case ECONNRESET:  return "ECONNRESET";
      case EDESTADDRREQ:  return "EDESTADDRREQ";
      case EISCONN:  return "EISCONN";
      case EMSGSIZE:  return "EMSGSIZE";
      case ENOBUFS:  return "ENOBUFS";
      case EOPNOTSUPP:  return "EOPNOTSUPP";
      case EPIPE:  return "EPIPE";

      case EPERM: return "EPERM";
      case EADDRINUSE: return "EADDRINUSE";
      case EAFNOSUPPORT: return "EAFNOSUPPORT";
      case EALREADY: return "EALREADY";
      case EINPROGRESS: return "EINPROGRESS";
      case ENETUNREACH: return "ENETUNREACH";
      case ETIMEDOUT: return "ETIMEDOUT";
   }

   return "UNCKNOWN";
}

// _____________________________________________________________________

dabc::SocketAddon::SocketAddon(int fd) :
   WorkerAddon("socket"),
   fSocket(fd),
   fDoingInput(false),
   fDoingOutput(false),
   fDeliverEventsToWorker(false)
{
}

dabc::SocketAddon::~SocketAddon()
{
   CloseSocket();
}


void dabc::SocketAddon::ProcessEvent(const EventId& evnt)
{
   switch (evnt.GetCode()) {
      case evntSocketRead:
         break;

      case evntSocketWrite:
         break;

      case evntSocketError:
         OnSocketError(0, "When working");
         break;

      default:
         WorkerAddon::ProcessEvent(evnt);
   }
}

void dabc::SocketAddon::SetSocket(int fd)
{
   CloseSocket();
   fSocket = fd;
}

int dabc::SocketAddon::TakeSocket()
{
   int fd = fSocket;
   fSocket = -1;
   return fd;
}

void dabc::SocketAddon::CloseSocket()
{
   if (fSocket<0) return;

   DOUT3("~~~~~~~~~~~~~~~~ Close socket %d", fSocket);
   close(fSocket);
   fSocket = -1;
}

int dabc::SocketAddon::TakeSocketError()
{
   if (Socket()<0) return -1;

   int myerrno = 753642;
   socklen_t optlen = sizeof(myerrno);

   int res = getsockopt(Socket(), SOL_SOCKET, SO_ERROR, &myerrno, &optlen);

   if ((res<0) || (myerrno == 753642)) return -1;

   return myerrno;
}

void dabc::SocketAddon::OnConnectionClosed()
{
   if (IsDeliverEventsToWorker()) {
      DOUT2("Addon:%p Connection closed - worker should process", this);
      FireWorkerEvent(evntSocketCloseInfo);
   } else {
      DOUT2("Connection closed - destroy socket");
      DeleteWorker();
   }
}

void dabc::SocketAddon::OnSocketError(int errnum, const std::string& info)
{
   EOUT("SocketError fd:%d %d:%s %s", Socket(), errnum, SocketErr(errnum), info.c_str());

   if (IsDeliverEventsToWorker()) {
      FireWorkerEvent(evntSocketErrorInfo);
   } else {
      DeleteWorker();
   }
}

ssize_t dabc::SocketAddon::DoRecvBuffer(void* buf, ssize_t len)
{
   ssize_t res = recv(fSocket, buf, len, MSG_DONTWAIT | MSG_NOSIGNAL);

   if (res==0) OnConnectionClosed(); else
   if (res<0) {
     if (errno!=EAGAIN) OnSocketError(errno, "When recv()");
   }

   return res;
}

ssize_t dabc::SocketAddon::DoRecvBufferHdr(void* hdr, ssize_t hdrlen, void* buf, ssize_t len, void* srcaddr, unsigned srcaddrlen)
{
   struct iovec iov[2];

   iov[0].iov_base = hdr;
   iov[0].iov_len = hdrlen;

   iov[1].iov_base = buf;
   iov[1].iov_len = len;

   struct msghdr msg;

   msg.msg_name = srcaddr;
   msg.msg_namelen = srcaddrlen;
   msg.msg_iov = iov;
   msg.msg_iovlen = buf ? 2 : 1;
   msg.msg_control = 0;
   msg.msg_controllen = 0;
   msg.msg_flags = 0;

   ssize_t res = recvmsg(fSocket, &msg, MSG_DONTWAIT | MSG_NOSIGNAL);

   if (res==0) OnConnectionClosed(); else
   if (res<0) {
     if (errno!=EAGAIN) OnSocketError(errno, "When recv()");
   }

   return res;
}

ssize_t dabc::SocketAddon::DoSendBuffer(void* buf, ssize_t len)
{
   ssize_t res = send(fSocket, buf, len, MSG_DONTWAIT | MSG_NOSIGNAL);

   if (res==0) OnConnectionClosed(); else
   if (res<0) {
     if (errno!=EAGAIN) OnSocketError(errno, "When send()");
   }

   return res;
}

ssize_t dabc::SocketAddon::DoSendBufferHdr(void* hdr, ssize_t hdrlen, void* buf, ssize_t len, void* tgtaddr, unsigned tgtaddrlen)
{
   struct iovec iov[2];

   iov[0].iov_base = hdr;
   iov[0].iov_len = hdrlen;

   iov[1].iov_base = buf;
   iov[1].iov_len = len;

   struct msghdr msg;

   msg.msg_name = tgtaddr;
   msg.msg_namelen = tgtaddrlen;
   msg.msg_iov = iov;
   msg.msg_iovlen = buf ? 2 : 1;
   msg.msg_control = 0;
   msg.msg_controllen = 0;
   msg.msg_flags = 0;

   ssize_t res = sendmsg(fSocket, &msg, MSG_DONTWAIT | MSG_NOSIGNAL);

   if (res==0) OnConnectionClosed(); else
   if (res<0) {
     if (errno!=EAGAIN) OnSocketError(errno, "When sendmsg()");
   }

   return res;
}


// _____________________________________________________________________

dabc::SocketIOAddon::SocketIOAddon(int fd, bool isdatagram, bool usemsg) :
   SocketAddon(fd),
   fDatagramSocket(isdatagram),
   fUseMsgOper(usemsg),
   fSendUseMsg(true),
   fSendIOV(0),
   fSendIOVSize(0),
   fSendIOVFirst(0),
   fSendIOVNumber(0),
   fSendUseAddr(false),
   fRecvUseMsg(true),
   fRecvIOV(0),
   fRecvIOVSize(0),
   fRecvIOVFirst(0),
   fRecvIOVNumber(0),
   fLastRecvSize(0)
{
   if (IsDatagramSocket() && !fUseMsgOper) {
      EOUT("Dangerous - datagram socket MUST use sendmsg()/recvmsg() operation to be able send/recv segmented buffers, force");
      fUseMsgOper = true;
   }

   memset(&fSendAddr, 0, sizeof(fSendAddr));

   #ifdef SOCKET_PROFILING
       fSendOper = 0;
       fSendTime = 0.;
       fSendSize = 0;
       fRecvOper = 0;
       fRecvTime = 0.;
       fRecvSize = 0.;
   #endif
}

dabc::SocketIOAddon::~SocketIOAddon()
{
   #ifdef SOCKET_PROFILING
      DOUT1("SocketIOAddon::~SocketIOAddon Send:%ld Recv:%ld", fSendOper, fRecvOper);
      if (fSendOper>0)
         DOUT1("   Send time:%5.1f microsec sz:%7.1f", fSendTime*1e6/fSendOper, 1.*fSendSize/fSendOper);
      if (fRecvOper>0)
         DOUT1("   Recv time:%5.1f microsec sz:%7.1f", fRecvTime*1e6/fRecvOper, 1.*fRecvSize/fRecvOper);
   #endif

   DOUT4("Destroying SocketIOAddon %p fd:%d", this, Socket());

   AllocateSendIOV(0);
   AllocateRecvIOV(0);
}

void dabc::SocketIOAddon::SetSendAddr(const std::string& host, int port)
{
   if (!IsDatagramSocket()) {
      EOUT("Cannot specify send addr for non-datagram sockets");
      return;
   }
   memset(&fSendAddr, 0, sizeof(fSendAddr));
   fSendUseAddr = false;

/*   if (!host.empty() && (port>0)) {
      fSendUseAddr = true;
      fSendAddr.sin_family = AF_INET;
      fSendAddr.sin_addr.s_addr = inet_addr(host.c_str());
      fSendAddr.sin_port = htons(port);
   }
*/

   struct hostent *h = gethostbyname(host.c_str());
   if ((h==0) || (h->h_addrtype!=AF_INET) || host.empty()) {
      EOUT("Cannot get host information for %s", host.c_str());
      return;
   }

   fSendAddr.sin_family = AF_INET;
   memcpy(&fSendAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
   fSendAddr.sin_port = htons (port);

   fSendUseAddr = true;

}


void dabc::SocketIOAddon::AllocateSendIOV(unsigned size)
{
   if (fSendIOV!=0) delete [] fSendIOV;

   fSendIOV = 0;
   fSendIOVSize = 0;
   fSendIOVFirst = 0;
   fSendIOVNumber = 0;

   if (size<=0) return;

   fSendIOVSize = size;
   fSendIOV = new struct iovec [size];
}

void dabc::SocketIOAddon::AllocateRecvIOV(unsigned size)
{
   if (fRecvIOV!=0) delete [] fRecvIOV;

   fRecvIOV = 0;
   fRecvIOVSize = 0;
   fRecvIOVFirst = 0;
   fRecvIOVNumber = 0;

   if (size<=0) return;

   fRecvIOVSize = size;
   fRecvIOV = new struct iovec [size];
}

bool dabc::SocketIOAddon::StartSend(const void* buf1, unsigned size1,
                                    const void* buf2, unsigned size2,
                                    const void* buf3, unsigned size3)
{
   if (fSendIOVNumber>0) {
      EOUT("Current send operation not yet completed");
      return false;
   }

   if (fSendIOVSize<3) AllocateSendIOV(8);

   int indx = 0;
   if (buf1 && (size1>0)) {
      fSendIOV[indx].iov_base = (void*) buf1;
      fSendIOV[indx].iov_len = size1;
      indx++;
   }

   if (buf2 && (size2>0)) {
      fSendIOV[indx].iov_base = (void*) buf2;
      fSendIOV[indx].iov_len = size2;
      indx++;
   }

   if (buf3 && (size3>0)) {
      fSendIOV[indx].iov_base = (void*) buf3;
      fSendIOV[indx].iov_len = size3;
      indx++;
   }

   if (indx==0) {
      EOUT("No buffer specified");
      return false;
   }

   fSendUseMsg = fUseMsgOper;
   fSendIOVFirst = 0;
   fSendIOVNumber = indx;

   // TODO: Should we inform thread directly that we want to send data??
   SetDoingOutput(true);

   return true;
}


bool dabc::SocketIOAddon::StartRecv(void* buf, size_t size)
{
   return StartRecvHdr(0, 0, buf, size);
}

bool dabc::SocketIOAddon::StartSend(const Buffer& buf)
{
   // this is simple version,
   // where only buffer itself without header is transported

   return StartNetSend(0, 0, buf);
}

bool dabc::SocketIOAddon::StartRecvHdr(void* hdr, unsigned hdrsize, void* buf, size_t size)
{
   if (fRecvIOVNumber>0) {
      EOUT("Current recv operation not yet completed");
      return false;
   }

   if (fRecvIOVSize<2) AllocateRecvIOV(8);

   int indx = 0;

   if ((hdr!=0) && (hdrsize>0)) {
      fRecvIOV[indx].iov_base = hdr;
      fRecvIOV[indx].iov_len = hdrsize;
      indx++;
   }

   fRecvIOV[indx].iov_base = buf;
   fRecvIOV[indx].iov_len = size;
   indx++;

   fRecvUseMsg = fUseMsgOper;
   fRecvIOVFirst = 0;
   fRecvIOVNumber = indx;

   // TODO: Should we inform thread directly that we want to recv data??
   SetDoingInput(true);

   return true;
}


bool dabc::SocketIOAddon::StartRecv(Buffer& buf, BufferSize_t datasize)
{
   return StartNetRecv(0, 0, buf, datasize);
}

bool dabc::SocketIOAddon::StartNetRecv(void* hdr, unsigned hdrsize, Buffer& buf, BufferSize_t datasize)
{
   // datasize==0 here really means that there is no data to get !!!!

   if (fRecvIOVNumber>0) {
      EOUT("Current recv operation not yet completed");
      return false;
   }

   if (buf.null()) return false;

   if (fRecvIOVSize<=buf.NumSegments()) AllocateRecvIOV(buf.NumSegments()+1);

   fRecvUseMsg = fUseMsgOper;
   fRecvIOVFirst = 0;

   int indx = 0;

   if ((hdr!=0) && (hdrsize>0)) {
      fRecvIOV[indx].iov_base = hdr;
      fRecvIOV[indx].iov_len = hdrsize;
      indx++;
   }

   for (unsigned nseg=0; nseg<buf.NumSegments(); nseg++) {
      BufferSize_t segsize = buf.SegmentSize(nseg);
      if (segsize>datasize) segsize = datasize;
      if (segsize==0) break;

      fRecvIOV[indx].iov_base = buf.SegmentPtr(nseg);
      fRecvIOV[indx].iov_len = segsize;
      indx++;

      datasize-=segsize;
   }

   fRecvIOVNumber = indx;

   // TODO: Should we inform thread directly that we want to recv data??
   SetDoingInput(true);

   return true;
}

bool dabc::SocketIOAddon::StartNetSend(void* hdr, unsigned hdrsize, const Buffer& buf)
{
   if (fSendIOVNumber>0) {
      EOUT("Current send operation not yet completed");
      return false;
   }

   if (buf.null()) return false;

   if (fSendIOVSize<=buf.NumSegments()) AllocateSendIOV(buf.NumSegments()+1);

   fSendUseMsg = fUseMsgOper;
   fSendIOVFirst = 0;

   int indx = 0;

   if ((hdr!=0) && (hdrsize>0)) {
      fSendIOV[indx].iov_base = hdr;
      fSendIOV[indx].iov_len = hdrsize;
      indx++;
   }

   for (unsigned nseg=0; nseg<buf.NumSegments(); nseg++) {
      fSendIOV[indx].iov_base = buf.SegmentPtr(nseg);
      fSendIOV[indx].iov_len = buf.SegmentSize(nseg);
      indx++;
   }

   fSendIOVNumber = indx;

   // TODO: Should we inform thread that we want to send data??
   SetDoingOutput(true);

   return true;
}

void dabc::SocketIOAddon::ProcessEvent(const EventId& evnt)
{
//   DOUT0("IO addon:%p process event %u", this, evnt.GetCode());

    switch (evnt.GetCode()) {
       case evntSocketRead: {

          if (IsLogging())
             DOUT0("Socket %d wants to receive number %d usemsg %s", Socket(), fRecvIOVNumber, DBOOL(fRecvUseMsg));

          // nothing to recv
          if (fRecvIOVNumber==0) return;

          if ((fRecvIOV==0) || (fSocket<0)) {
             EOUT("HARD PROBLEM when reading socket");
             OnSocketError(1, "Missing socket when evntSocketRead fired");
             return;
          }

          #ifdef SOCKET_PROFILING
             fRecvOper++;
             TimeStamp tm1 = dabc::Now();
          #endif

          fLastRecvSize = 0;
          ssize_t res = 0;

//          DOUT1("Socket %d fRecvIOV = %p fRecvIOVFirst = %u number %u iov: %p %u",
//                fSocket, fRecvIOV, fRecvIOVFirst, fRecvIOVNumber,
//                fRecvIOV[fRecvIOVFirst].iov_base, fRecvIOV[fRecvIOVFirst].iov_len);

          if (fRecvUseMsg) {

             struct msghdr msg;

             msg.msg_name = &fRecvAddr;
             msg.msg_namelen = sizeof(fRecvAddr);
             msg.msg_iov = &(fRecvIOV[fRecvIOVFirst]);
             msg.msg_iovlen = fRecvIOVNumber - fRecvIOVFirst;
             msg.msg_control = 0;
             msg.msg_controllen = 0;
             msg.msg_flags = 0;

             res = recvmsg(fSocket, &msg, MSG_DONTWAIT | MSG_NOSIGNAL);
          } else {
             socklen_t addrlen = sizeof(fRecvAddr);
             res = recvfrom(fSocket, fRecvIOV[fRecvIOVFirst].iov_base, fRecvIOV[fRecvIOVFirst].iov_len, MSG_DONTWAIT | MSG_NOSIGNAL, (sockaddr*) &fRecvAddr, &addrlen);
          }

//          if (IsLogging())
//             DOUT0("Socket %d get receive %d", Socket(), res);

          #ifdef SOCKET_PROFILING
             TimeStamp tm2 = dabc::Now();
             fRecvTime += (tm2-tm1);
             if (res>0) fRecvSize += res;
          #endif

          if (res==0) {
             // DOUT0("Addon:%p socket:%d res==0 when doing read usemsg %s numseg %u seg0.len %u", this, fSocket, DBOOL(fRecvUseMsg), fRecvIOVNumber - fRecvIOVFirst, fRecvIOV[fRecvIOVFirst].iov_len);
             OnConnectionClosed();
             return;
          }

          if (res<0) {
             if (errno!=EAGAIN)
                OnSocketError(errno, "When recvmsg()");
             else {
                // we indicating that we want to receive data but there is nothing to read
                // why we get message at all?
                SetDoingInput(true);
                EOUT("Why socket read message produce but we do not get any data??");
             }

             return;
          }

          fLastRecvSize = res;

          if (IsDatagramSocket()) {
             // for datagram the only recv message is possible
             fRecvIOVFirst = 0;
             fRecvIOVNumber = 0;

//             if (IsLogging())
//                DOUT0("Socket %d signals COMPL", Socket());

             OnRecvCompleted();
             return;
          }

          while (res>0) {

             struct iovec* rec = &(fRecvIOV[fRecvIOVFirst]);

             if (rec->iov_len <= (unsigned) res) {
                // just skip current rec, jump to next
                res -= rec->iov_len;
                fRecvIOVFirst++;

                if (fRecvIOVFirst==fRecvIOVNumber) {
                   if (res!=0) EOUT("Internal error - length after recvmsg() not zero");

                   fRecvIOVFirst = 0;
                   fRecvIOVNumber = 0;

//                 if (IsLogging())
//                    DOUT0("Socket %d signals COMPL", Socket());

                   OnRecvCompleted();

                   return;
                }
             } else {
                rec->iov_len -= res;
                rec->iov_base = (char*)rec->iov_base + res;
                res = 0;
             }
          }

          // there is still some portion of data should be read from the socket, indicate this for the thread
          SetDoingInput(true);

          break;
       }

       case evntSocketWrite: {

          if (fSendIOVNumber==0) return; // nothing to send

          #ifdef SOCKET_PROFILING
             fSendOper++;
             TimeStamp tm1 = dabc::Now();
          #endif

          if ((fSocket<0) || (fSendIOV==0)) {
             EOUT("HARD PROBLEM when trying write socket");
          }

          ssize_t res = 0;

          if (fSendUseMsg) {

             struct msghdr msg;

             msg.msg_name = fSendUseAddr ? &fSendAddr : 0;
             msg.msg_namelen = fSendUseAddr ? sizeof(fSendAddr) : 0;;
             msg.msg_iov = &(fSendIOV[fSendIOVFirst]);
             msg.msg_iovlen = fSendIOVNumber - fSendIOVFirst;
             msg.msg_control = 0;
             msg.msg_controllen = 0;
             msg.msg_flags = 0;

             res = sendmsg(fSocket, &msg, MSG_DONTWAIT | MSG_NOSIGNAL);
          } else
             res = send(fSocket, fSendIOV[fSendIOVFirst].iov_base, fSendIOV[fSendIOVFirst].iov_len, MSG_DONTWAIT | MSG_NOSIGNAL);

          #ifdef SOCKET_PROFILING
             TimeStamp tm2 = dabc::Now();
             fSendTime += (tm2-tm1);
             if (res>0) fSendSize += res;
          #endif


          if (res==0) {
             OnConnectionClosed();
             return;
          }

          if (res<0) {
             DOUT2("Error when sending via socket %d  usemsg %s first %d number %d", fSocket, DBOOL(fSendUseMsg), fSendIOVFirst, fSendIOVNumber);

             if (errno!=EAGAIN)
                OnSocketError(errno, "When sendmsg()");
             else {
                // we indicating that we want to receive data but there is nothing to read
                // why we get message at all?
                SetDoingOutput(true);
                EOUT("Why socket write message produce but we did not send any bytes?");
             }
             return;
          }

          DOUT5("Socket %d send %d bytes", Socket(), res);

          while (res>0) {
             struct iovec* rec = &(fSendIOV[fSendIOVFirst]);

             if (rec->iov_len <= (unsigned) res) {
                // just skip current rec, jump to next
                res -= rec->iov_len;
                fSendIOVFirst++;

                if (fSendIOVFirst==fSendIOVNumber) {
                   if (res!=0) EOUT("Internal error - length after sendmsg() not zero");

                   fSendIOVFirst = 0;
                   fSendIOVNumber = 0;

                   OnSendCompleted();

                   return;
                }
             } else {
                rec->iov_len -= res;
                rec->iov_base = (char*)rec->iov_base + res;
                res = 0;
             }
          }

          // we are informing that there is some data still to send
          SetDoingOutput(true);

          break;
       }
       default:
          SocketAddon::ProcessEvent(evnt);
    }
}

void dabc::SocketIOAddon::CancelIOOperations()
{
   fSendIOVNumber = 0;
   fRecvIOVNumber = 0;
}

// ___________________________________________________________________

dabc::SocketServerAddon::SocketServerAddon(int serversocket, int portnum) :
   SocketConnectAddon(serversocket),
   fServerPortNumber(portnum),
   fServerHostName()
{
   fServerHostName = dabc::SocketDevice::GetLocalHost(true);

//   dabc::SocketThread::DefineHostName();

   SetDoingInput(true);
   listen(Socket(), 10);

   DOUT2("Create dabc::SocketServerAddon");
}

void dabc::SocketServerAddon::ProcessEvent(const EventId& evnt)
{
    switch (evnt.GetCode()) {
       case evntSocketRead: {

          // we accept more connections
          SetDoingInput(true);

          int connfd = accept(Socket(), 0, 0);

          if (connfd<0) {
             EOUT("Error with accept");
             return;
          }

          listen(Socket(), 10);

          if (!dabc::SocketThread::SetNonBlockSocket(connfd)) {
             EOUT("Cannot set nonblocking flag for connected socket");
             close(connfd);
             return;
          }

          DOUT2("We get new connection with fd: %d", connfd);

          OnClientConnected(connfd);

          break;
       }

       default:
          dabc::SocketConnectAddon::ProcessEvent(evnt);
    }
}

void dabc::SocketServerAddon::OnClientConnected(int fd)
{
   Command cmd("SocketConnect");
   cmd.SetStr("Type", "Server");
   cmd.SetInt("fd", fd);

   if (!fConnRcv.null() && !fConnId.empty()) {
      cmd.SetStr("ConnId", fConnId);
      fConnRcv.Submit(cmd);
   } else
   if (!fWorker.null()) {
      ((Worker*) fWorker())->Submit(cmd);
   } else {
      EOUT("Method not implemented - socked will be closed");
      close(fd);
   }
}

// _____________________________________________________________________

dabc::SocketClientAddon::SocketClientAddon(const struct addrinfo* serv_addr, int fd) :
   SocketConnectAddon(fd),
   fRetry(1),
   fRetryTmout(-1)
{
   if (serv_addr==0) {
      EOUT("Server address not specified");
      return;
   }

   fServAddr.ai_flags = serv_addr->ai_flags;
   fServAddr.ai_family = serv_addr->ai_family;
   fServAddr.ai_socktype = serv_addr->ai_socktype;
   fServAddr.ai_protocol = serv_addr->ai_protocol;
   fServAddr.ai_addrlen = serv_addr->ai_addrlen;
   fServAddr.ai_addr = 0;
   if ((serv_addr->ai_addrlen>0) && (serv_addr->ai_addr!=0)) {
       fServAddr.ai_addr = (sockaddr*) malloc(fServAddr.ai_addrlen);
       memcpy(fServAddr.ai_addr, serv_addr->ai_addr, fServAddr.ai_addrlen);
   }

   fServAddr.ai_canonname = 0;
   if (serv_addr->ai_canonname) {
      fServAddr.ai_canonname = (char*) malloc(strlen(serv_addr->ai_canonname) + 1);
      strcpy(fServAddr.ai_canonname, serv_addr->ai_canonname);
   }
   fServAddr.ai_next = 0;
}

dabc::SocketClientAddon::~SocketClientAddon()
{
//   DOUT0("Actual destroy of SocketClientAddon %p worker %p", this, fWorker());

   free(fServAddr.ai_addr); fServAddr.ai_addr = 0;
   free(fServAddr.ai_canonname); fServAddr.ai_canonname = 0;
}

void dabc::SocketClientAddon::SetRetryOpt(int nretry, double tmout)
{
   fRetry = nretry;
   fRetryTmout = tmout;
}

void dabc::SocketClientAddon::OnThreadAssigned()
{
   FireWorkerEvent(evntSocketStartConnect);
}

void dabc::SocketClientAddon::ProcessEvent(const EventId& evnt)
{
   switch (evnt.GetCode()) {
       case evntSocketWrite: {

          // we could get write event after socket was closed by error - ignore such event
          if (Socket()<=0) return;

          // we can check if connection established

          int myerrno = TakeSocketError();

          if (myerrno==0) {
             DOUT5("Connection done %7.5f", dabc::Now().AsDouble());
             int fd = TakeSocket();
             OnConnectionEstablished(fd);
             return;
          }

          DOUT3("Postponed connect socket err:%d %s", myerrno, SocketErr(myerrno));

          break;
       }

       case evntSocketError: {
          // int myerrno = TakeSocketError();
          // DOUT3("Doing connect socket err:%d %s", myerrno, SocketErr(myerrno));
          break;
       }

       case evntSocketStartConnect: {
          // start next attempt for connection

          DOUT3("Start next connect attempt sock:%d", Socket());

          if (Socket()<=0) {
             int fd = socket(fServAddr.ai_family, fServAddr.ai_socktype, fServAddr.ai_protocol);

             if (fd<=0)
                EOUT("Cannot create socket with given address");
             else
                SetSocket(fd);
          }

          if (Socket()>0) {
             dabc::SocketThread::SetNonBlockSocket(Socket());

             int res = connect(Socket(), fServAddr.ai_addr, fServAddr.ai_addrlen);

             if (res==0) {
                int fd = TakeSocket();
                OnConnectionEstablished(fd);
                return;
             }

             if (errno==EINPROGRESS) {
                DOUT3("Connection in progress %7.5f", dabc::Now().AsDouble());
                SetDoingOutput(true);
                return;
             }

             DOUT3("When calling connection socket err:%d %s", errno, SocketErr(errno));
          }

          break;
       }

       default:
          dabc::SocketConnectAddon::ProcessEvent(evnt);
          return;
   }

   SetDoingOutput(false);
   CloseSocket();

   if (--fRetry>0) {
      DOUT3("Try connect after %5.1f s  n:%d", fRetryTmout, fRetry);

      ActivateTimeout(fRetryTmout > 0. ? fRetryTmout : 0.);
   } else {
      OnConnectionFailed();
   }
}

double dabc::SocketClientAddon::ProcessTimeout(double)
{
   // activate connection start again

   FireWorkerEvent(evntSocketStartConnect);
   return -1.;
}

void dabc::SocketClientAddon::OnConnectionEstablished(int fd)
{
   Command cmd("SocketConnect");
   cmd.SetStr("Type", "Client");
   cmd.SetInt("fd", fd);
   cmd.SetStr("ConnId", fConnId);

   if (!fWorker.null() && IsDeliverEventsToWorker()) {
      ((Worker*) fWorker())->Submit(cmd);
      return;
   }

   if (!fConnRcv.null()) {
      fConnRcv.Submit(cmd);
   } else {
      EOUT("Connection established, but not processed - close socket");
      close(fd);
   }

   DeleteWorker();
}

void dabc::SocketClientAddon::OnConnectionFailed()
{
   Command cmd("SocketConnect");
   cmd.SetStr("Type", "Error");
   cmd.SetStr("ConnId", fConnId);

   if (!fWorker.null() && IsDeliverEventsToWorker()) {
      ((Worker*) fWorker())->Submit(cmd);
      return;
   }

   if (!fConnRcv.null()) {
      fConnRcv.Submit(cmd);
   } else {
      EOUT("Connection failed to establish, error not processed");
   }

   DeleteWorker();
}


// _______________________________________________________________________

dabc::SocketThread::SocketThread(Reference parent, const std::string& name, Command cmd) :
   dabc::Thread(parent, name, cmd),
   fPipeFired(false),
   fWaitFire(false),
   fScalerCounter(10),
   f_sizeufds(0),
   f_ufds(0),
   f_recs(0),
   fIsAnySocket(false),
   fCheckNewEvents(true)
{

#ifdef SOCKET_PROFILING
   fWaitCalls = 0;
   fWaitDone = 0;
   fWaitTime = 0;
   fFillTime = 0;
   fPipeCalled = 0;
#endif

   fPipe[0] = 0;
   fPipe[1] = 0;
   pipe(fPipe);

   // by this call we rebuild ufds array, for now only for the pipe
   WorkersSetChanged();

}

dabc::SocketThread::~SocketThread()
{
   // !!!!!! we should stop thread before destroying anything while
   // thread may use some structures in the MainLoop !!!!!!!!

   Stop(1.);

   if (fPipe[0]!=0) { close(fPipe[0]);  fPipe[0] = 0; }
   if (fPipe[1]!=0) { close(fPipe[1]);  fPipe[1] = 0; }

   if (f_ufds!=0) {
      delete[] f_ufds;
      delete[] f_recs;
      f_ufds = 0;
      f_recs = 0;
      f_sizeufds = 0;
   }

   #ifdef SOCKET_PROFILING
     DOUT1("Thrd:%s Wait called %ld done %ld ratio %5.3f %s  Pipe:%ld", GetName(), fWaitCalls, fWaitDone, (fWaitCalls>0 ? 100.*fWaitDone/fWaitCalls : 0.) ,"%", fPipeCalled);
     if (fWaitDone>0)
        DOUT1("Aver times fill:%5.1f microsec wait:%5.1f microsec", fFillTime*1e6/fWaitDone, fWaitTime*1e6/fWaitDone);
   #endif
}

bool dabc::SocketThread::CompatibleClass(const std::string& clname) const
{
   if (Thread::CompatibleClass(clname)) return true;
   return clname == typeSocketThread;
}

bool dabc::SocketThread::SetNonBlockSocket(int fd)
{
   int opts = fcntl(fd, F_GETFL);
   if (opts < 0) {
      EOUT("fcntl(F_GETFL) failed");
      return false;
   }
   opts = (opts | O_NONBLOCK);
   if (fcntl(fd, F_SETFL,opts) < 0) {
      EOUT("fcntl(F_SETFL) failed");
      return false;
   }
   return true;
}


int dabc::SocketThread::StartServer(int& portnum, int portmin, int portmax)
{
   int numtests = 1; // at least test value of portnum
   if ((portmin>0) && (portmax>0) && (portmin<=portmax)) numtests+=(portmax-portmin+1);

   int firsttest = portnum;

   for(int ntest=0;ntest<numtests;ntest++) {

     if ((ntest==0) && (portnum<0)) continue;

     if (ntest>0) portnum = portmin - 1 + ntest;

     int sockfd = -1;

     struct addrinfo hints, *info;

     memset(&hints, 0, sizeof(hints));
     hints.ai_flags    = AI_PASSIVE;
     hints.ai_family   = AF_UNSPEC; //AF_INET;
     hints.ai_socktype = SOCK_STREAM;

     std::string localhost = dabc::SocketDevice::GetLocalHost(false);

     const char* myhostname = localhost.empty() ? 0 : localhost.c_str();
     char service[100];
     sprintf(service, "%d", portnum);

     int n = getaddrinfo(myhostname, service, &hints, &info);

     DOUT2("GetAddrInfo %s:%s res = %d", (myhostname ? myhostname : "---"), service, n);

     if (n < 0) {
        EOUT("Cannot get addr info for service %s:%s", (myhostname ? myhostname : "localhost"), service);
        continue;
     }

     for (struct addrinfo *t = info; t; t = t->ai_next) {

        sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
        if (sockfd >= 0) {

           int opt = 1;

           setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

           if (!bind(sockfd, t->ai_addr, t->ai_addrlen)) break;
           close(sockfd);
           sockfd = -1;
        }
     }

     freeaddrinfo(info);

     if (sockfd<0) {
       DOUT3("Cannot bind socket to service %s", service);
       continue;
     }

     if (dabc::SocketThread::SetNonBlockSocket(sockfd)) return sockfd;

     EOUT("Cannot set nonblocking flag for server socket");
     close(sockfd);
   }

   EOUT("Cannot bind server socket to port %d or find its in range %d:%d", firsttest, portmin, portmax);
   portnum = -1;
   return -1;
}

std::string dabc::SocketThread::DefineHostName()
{
   char hostname[1000];
   if (gethostname(hostname, sizeof(hostname)))
      EOUT("ERROR gethostname");
   else
      DOUT3( "gethostname = %s", hostname);
   return std::string(hostname);
}

dabc::SocketServerAddon* dabc::SocketThread::CreateServerAddon(int nport, int portmin, int portmax)
{
   int fd = dabc::SocketThread::StartServer(nport, portmin, portmax);

   return fd < 0 ? 0 : new SocketServerAddon(fd, nport);
}

int dabc::SocketThread::StartClient(const char* host, int nport)
{
   char service[100];

   sprintf(service, "%d", nport);

   struct addrinfo *info;
   struct addrinfo hints;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   int sockfd(-1);

   if (getaddrinfo(host, service, &hints, &info)!=0) return sockfd;

   for (struct addrinfo *t = info; t; t = t->ai_next) {

      sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);

      if (sockfd<=0) { sockfd = -1; continue; }

      if (connect(sockfd, t->ai_addr, t->ai_addrlen)==0) {
         if (dabc::SocketThread::SetNonBlockSocket(sockfd))
            break; // socket is initialized - one could return it
         else
            EOUT("Cannot set non-blocking flag for client socket");
      }

      close(sockfd);
      sockfd = -1;
   }

   // always must be called
   freeaddrinfo(info);

   return sockfd;
}

bool dabc::SocketThread::AttachMulticast(int socket_descriptor, const std::string& host)
{
   if (host.empty() || (socket_descriptor<=0)) {
      EOUT("Multicast address or socket handle not specified");
      return false;
   }

   struct hostent *server_host_name = gethostbyname(host.c_str());
   if (server_host_name==0) {
      EOUT("Cannot get host information for %s", host.c_str());
      return false;
   }

   struct ip_mreq command;

   // Allow to use same port by many processes


   // Allow to receive broadcast to this port
   int loop = 1;
   if (setsockopt (socket_descriptor, IPPROTO_IP, IP_MULTICAST_LOOP,
         &loop, sizeof (loop)) < 0) {
      EOUT("Fail setsockopt IP_MULTICAST_LOOP");
      return false;
   }

   // Join the multicast group
   command.imr_multiaddr.s_addr = inet_addr (host.c_str());
   command.imr_interface.s_addr = htonl (INADDR_ANY);
   if (command.imr_multiaddr.s_addr == (in_addr_t)-1) {
      EOUT("%s is not valid address", host.c_str());
      return false;
   }
   if (setsockopt(socket_descriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP,
         &command, sizeof (command)) < 0) {
      EOUT("File setsockopt IP_ADD_MEMBERSHIP");
      return false;
   }

  return true;
}


void dabc::SocketThread::DettachMulticast(int handle, const std::string& host)
{
   if ((handle<0) || host.empty()) return;

   struct ip_mreq command;

   command.imr_multiaddr.s_addr = inet_addr (host.c_str());
   command.imr_interface.s_addr = htonl (INADDR_ANY);

   // Remove socket from multicast group
   if (setsockopt (handle, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                   &command, sizeof (command)) < 0 ) {
      EOUT("Fail setsockopt:IP_DROP_MEMBERSHIP");
   }
}

int dabc::SocketThread::CreateUdp()
{
   int fd = socket(PF_INET, SOCK_DGRAM, 0);
   if (fd<0) return -1;

   if (SetNonBlockSocket(fd)) return fd;
   close(fd);
   return -1;
}

void dabc::SocketThread::CloseUdp(int fd)
{
   if (fd>0) close(fd);
}


int dabc::SocketThread::BindUdp(int fd, int nport, int portmin, int portmax)
{
   if (fd<0) return -1;

   struct sockaddr_in m_addr;
   int numtests = 1; // at least test value of portnum
   if ((portmin>0) && (portmax>0) && (portmin<=portmax)) numtests+=(portmax-portmin+1);

   for(int ntest=0;ntest<numtests;ntest++) {
      if ((ntest==0) && (nport<0)) continue;
      if (ntest>0) nport = portmin - 1 + ntest;

      memset(&m_addr, 0, sizeof(m_addr));
      m_addr.sin_family = AF_INET;
      // m_addr.s_addr = htonl (INADDR_ANY);
      m_addr.sin_port = htons(nport);

      if (bind(fd, (struct sockaddr *)&m_addr, sizeof(m_addr))==0) return nport;
   }

   return -1;
}

int dabc::SocketThread::ConnectUdp(int fd, const std::string& remhost, int remport)
{
   if (fd<0) return fd;

   struct hostent *host = gethostbyname(remhost.c_str());
   if ((host==0) || (host->h_addrtype!=AF_INET) || remhost.empty()) {
      EOUT("Cannot get host information for %s", remhost.c_str());
      close(fd);
      return -1;
   }

   struct sockaddr_in address;

   memset (&address, 0, sizeof (address));
   address.sin_family = AF_INET;
   memcpy(&address.sin_addr.s_addr, host->h_addr_list[0], host->h_length);
   address.sin_port = htons (remport);

   if (connect(fd, (struct sockaddr *) &address, sizeof (address)) < 0) {
      EOUT("Fail to connect to host %s port %d", remhost.c_str(), remport);
      close(fd);
      return -1;
   }
   return fd;
}

dabc::SocketClientAddon* dabc::SocketThread::CreateClientAddon(const std::string& serverid)
{
   if (serverid.empty()) return 0;

   size_t pos = serverid.find(':');
   if (pos == serverid.npos) return 0;

   std::string host = serverid.substr(0, pos);
   std::string service = serverid.substr(pos+1, serverid.length()-pos);

   SocketClientAddon* addon = 0;

   DOUT5("CreateClientAddon %s:%s", host.c_str(), service.c_str());

   struct addrinfo *info;
   struct addrinfo hints;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   if (getaddrinfo(host.c_str(), service.c_str(), &hints, &info)==0) {
      for (struct addrinfo *t = info; t; t = t->ai_next) {

         int sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);

         if (sockfd<=0) continue;

         addon = new SocketClientAddon(t, sockfd);
         break;
      }
      freeaddrinfo(info);
   }

   DOUT5("CreateClientAddon %s:%s done res = %p", host.c_str(), service.c_str(), addon);

   return addon;
}


void dabc::SocketThread::_Fire(const dabc::EventId& arg, int nq)
{
   DOUT5("SocketThread::_Fire %s nq:%d numq:%d waiting:%s", GetName(), nq, fNumQueues, DBOOL(fWaitFire));

   _PushEvent(arg, nq);

   if (fWaitFire && !fPipeFired) {
      write(fPipe[1], "w", 1);
      fPipeFired = true;

      #ifdef SOCKET_PROFILING
         fPipeCalled++;
      #endif
   }
}

bool dabc::SocketThread::WaitEvent(EventId& evnt, double tmout_sec)
{
   // first check, if we have already event, which must be processed

   #ifdef SOCKET_PROFILING
     fWaitCalls++;

     TimeStamp tm1 = dabc::Now();
   #endif


   #ifdef DABC_EXTRA_CHECKS
      unsigned sizebefore(0), sizeafter(0);
   #endif

   {
      dabc::LockGuard lock(ThreadMutex());

      #ifdef DABC_EXTRA_CHECKS
         sizebefore = _TotalNumberOfEvents();
      #endif

      // if we already have events in the queue,
      // check if we take them out or first check if new sockets events there

      if (_TotalNumberOfEvents()>0) {

         if (!fCheckNewEvents) return _GetNextEvent(evnt);

         // we have events in the queue, therefore do not wait - just check new events
         tmout_sec = 0.;
      }

      if (f_ufds==0) return false;

      fWaitFire = true;
   }

   // here we wait for next event from any socket, including pipe

   int numufds = 1;

   f_ufds[0].fd = fPipe[0];
   f_ufds[0].events = POLLIN;
   f_ufds[0].revents = 0;

   for(unsigned n=1; n<fWorkers.size(); n++) {
      if (!f_recs[n].use) continue;
      SocketAddon* addon = (SocketAddon*) fWorkers[n]->addon;

      if (addon->Socket()<=0) continue;

      short events = 0;

      if (addon->IsDoingInput())
         events |= POLLIN;

      if (addon->IsDoingOutput())
         events |= POLLOUT;

      if (events==0) continue;

      f_ufds[numufds].fd = addon->Socket();
      f_ufds[numufds].events = events;
      f_ufds[numufds].revents = 0;

      f_recs[numufds].indx = n; // this is for dereferencing of the value

      numufds++;
   }

   int tmout = tmout_sec < 0. ? -1 : int(tmout_sec*1000.);

   #ifdef SOCKET_PROFILING
     fWaitDone++;
     TimeStamp tm2 = dabc::Now();

     fFillTime += (tm2-tm1);
   #endif

//   DOUT2("SOCKETTHRD: start waiting %d", tmout);

//     DOUT0("SocketThread %s (%d) wait with timeout %d ms numufds %d", GetName(), entry_cnt, tmout, numufds);

   int poll_res = poll(f_ufds, numufds, tmout);


   #ifdef SOCKET_PROFILING
     TimeStamp tm3 = dabc::Now();
     fWaitTime += (tm3-tm2);
   #endif

   dabc::LockGuard lock(ThreadMutex());

//   DOUT2("SOCKETTHRD: did waiting %d numevents %u", tmout, _TotalNumberOfEvents());


   fWaitFire = false;

   // cleanup pipe in bigger steps
   if (fPipeFired) {
      char sbuf;
      read(fPipe[0], &sbuf, 1);
      fPipeFired = false;
   }

   bool isany = false;

   // if we really has any events, analyze all of them and push in the queue
   if (poll_res>0)
      for (int n=1; n<numufds;n++) {

         if (f_ufds[n].revents==0) continue;

         SocketAddon* addon = (SocketAddon*) fWorkers[f_recs[n].indx]->addon;
         Worker* worker = fWorkers[f_recs[n].indx]->work;

         if ((addon==0) || (worker==0)) {
            EOUT("Something went wrong - socket addon=%p worker = %p, something is gone", addon, worker);
            exit(543);
         }


         if (f_ufds[n].revents & (POLLERR | POLLHUP | POLLNVAL)) {
//            EOUT("Error on the socket %d", f_ufds[n].fd);
            _PushEvent(EventId(SocketAddon::evntSocketError, f_recs[n].indx), 0);
            addon->SetDoingInput(false);
            addon->SetDoingOutput(false);
            IncWorkerFiredEvents(worker);
            isany = true;
         }

         if (f_ufds[n].revents & (POLLIN | POLLPRI)) {
            _PushEvent(EventId(SocketAddon::evntSocketRead, f_recs[n].indx), 1);
            addon->SetDoingInput(false);
            IncWorkerFiredEvents(worker);
            isany = true;
         }

         if (f_ufds[n].revents & POLLOUT) {
            _PushEvent(EventId(SocketAddon::evntSocketWrite, f_recs[n].indx), 1);
            addon->SetDoingOutput(false);
            IncWorkerFiredEvents(worker);
            isany = true;
         }
      }

//      DOUT0("SocketThread %s (%d) did wait with res %d isany %s", GetName(), entry_cnt, poll_res, DBOOL(isany));

   // we put additional event to enable again sockets checking
   if (isany) {
      fCheckNewEvents = false;
      _PushEvent(evntEnableCheck, 1);
   }

   #ifdef DABC_EXTRA_CHECKS
      sizeafter = _TotalNumberOfEvents();
//      if (sizeafter-sizebefore > 1) DOUT0("Thread:%s before:%u after:%u diff:%u", GetName(), sizebefore, sizeafter, sizeafter - sizebefore);
   #endif

   return _GetNextEvent(evnt);
}

void dabc::SocketThread::ProcessExtraThreadEvent(const EventId& evid)
{
   if (evid.GetCode() == evntEnableCheck) {
     fCheckNewEvents = true;
     return;
   }

   dabc::Thread::ProcessExtraThreadEvent(evid);
}


void dabc::SocketThread::WorkersSetChanged()
{
   unsigned new_sz = fWorkers.size();

   if (new_sz > f_sizeufds) {

      delete[] f_ufds;
      delete[] f_recs;
      f_ufds = new pollfd [new_sz];
      f_recs = new ProcRec [new_sz];

      f_sizeufds = new_sz;
   }

   memset(f_ufds, 0, sizeof(pollfd) * f_sizeufds);
   memset(f_recs, 0, sizeof(ProcRec) * f_sizeufds);

   f_recs[0].use = true;
   f_recs[0].indx = 0;
   fIsAnySocket = false;

//   DOUT0("SocketThread %s WorkersNumberChanged  size %u", GetName(), fWorkers.size());

   for (unsigned indx=1;indx<fWorkers.size();indx++) {
      SocketAddon* addon = dynamic_cast<SocketAddon*> (fWorkers[indx]->addon);

      f_recs[indx].use = addon!=0;

      if (addon!=0) {
         fIsAnySocket = true;
//         DOUT0("Socket %d doing input=%s output=%s", addon->Socket(), DBOOL(addon->IsDoingInput()), DBOOL(addon->IsDoingOutput()));
      }
   }

   // any time new processor is added, check for new socket events
   fCheckNewEvents = fIsAnySocket;

//   DOUT0("SocketThread %s WorkersNumberChanged %u done", GetName(), fWorkers.size());

}
