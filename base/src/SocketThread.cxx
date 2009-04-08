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
#include "dabc/SocketThread.h"

#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
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

#define ShowSocketError(info, err) EOUT(("SocketError %d:%s %s", err, SocketErr(err), info))

// _____________________________________________________________________

dabc::SocketProcessor::SocketProcessor(int fd) :
   dabc::WorkingProcessor(),
   fSocket(fd),
   fDoingInput(false),
   fDoingOutput(false)
{
}

dabc::SocketProcessor::~SocketProcessor()
{
   CloseSocket();
}

void dabc::SocketProcessor::SetSocket(int fd)
{
   CloseSocket();
   fSocket = fd;
}

int dabc::SocketProcessor::TakeSocket()
{
   int fd = fSocket;
   fSocket = -1;
   return fd;
}

void dabc::SocketProcessor::CloseSocket()
{
   if (fSocket<0) return;

   DOUT3(("~~~~~~~~~~~~~~~~ Close socket %d", fSocket));
   close(fSocket);
   fSocket = -1;
}

int dabc::SocketProcessor::TakeSocketError()
{
   if (Socket()<0) return -1;

   int myerrno = 753642;
   socklen_t optlen = sizeof(myerrno);

   int res = getsockopt(Socket(), SOL_SOCKET, SO_ERROR, &myerrno, &optlen);

   if ((res<0) || (myerrno == 753642)) return -1;

   return myerrno;
}

void dabc::SocketProcessor::ProcessEvent(dabc::EventId evnt)
{
    switch (GetEventCode(evnt)) {
       case evntSocketRead:
          break;

       case evntSocketWrite:
          break;

       case evntSocketError:
          OnSocketError(0, "When working");
          break;

       default:
          WorkingProcessor::ProcessEvent(evnt);
    }
}

void dabc::SocketProcessor::OnConnectionClosed()
{
   DOUT2(("Connection closed - destroy socket"));
   DestroyProcessor();
}

void dabc::SocketProcessor::OnSocketError(int errnum, const char* info)
{
   ShowSocketError(info, errnum);
   DestroyProcessor();
}

// _____________________________________________________________________

dabc::SocketIOProcessor::SocketIOProcessor(int fd) :
   SocketProcessor(fd),
   fSendUseMsg(true),
   fSendIOV(0),
   fSendIOVSize(0),
   fSendIOVFirst(0),
   fSendIOVNumber(0),
   fRecvUseMsg(true),
   fRecvIOV(0),
   fRecvIOVSize(0),
   fRecvIOVFirst(0),
   fRecvIOVNumber(0),
   fRecvSingle(false),
   fLastRecvSize(0)
{
   #ifdef SOCKET_PROFILING
       fSendOper = 0;
       fSendTime = 0.;
       fSendSize = 0;
       fRecvOper = 0;
       fRecvTime = 0.;
       fRecvSize = 0.;
   #endif
}

dabc::SocketIOProcessor::~SocketIOProcessor()
{
   #ifdef SOCKET_PROFILING
      DOUT1(("SocketIOProcessor::~SocketIOProcessor Send:%ld Recv:%ld", fSendOper, fRecvOper));
      if (fSendOper>0)
         DOUT1(("   Send time:%5.1f microsec sz:%7.1f", fSendTime*1e6/fSendOper, 1.*fSendSize/fSendOper));
      if (fRecvOper>0)
         DOUT1(("   Recv time:%5.1f microsec sz:%7.1f", fRecvTime*1e6/fRecvOper, 1.*fRecvSize/fRecvOper));
   #endif

   AllocateSendIOV(0);
   AllocateRecvIOV(0);
}

void dabc::SocketIOProcessor::AllocateSendIOV(unsigned size)
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

void dabc::SocketIOProcessor::AllocateRecvIOV(unsigned size)
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

bool dabc::SocketIOProcessor::StartSend(void* buf, size_t size, bool usemsg)
{
   if (fSendIOVNumber>0) {
      EOUT(("Current send operation not yet completed"));
      return false;
   }

   if (fSendIOVSize<1) AllocateSendIOV(8);

   fSendUseMsg = usemsg;
   fSendIOVFirst = 0;
   fSendIOVNumber = 1;
   fSendIOV[0].iov_base = buf;
   fSendIOV[0].iov_len = size;

   SetDoingOutput(true);
   FireEvent(evntSocketWrite);

   return true;
}

bool dabc::SocketIOProcessor::StartRecv(void* buf, size_t size, bool usemsg, bool singleoper)
{
   if (fRecvIOVNumber>0) {
      EOUT(("Current recv operation not yet completed"));
      return false;
   }

   if (fRecvIOVSize<1) AllocateRecvIOV(8);

   fRecvUseMsg = usemsg;
   fRecvIOVFirst = 0;
   fRecvIOVNumber = 1;
   fRecvIOV[0].iov_base = buf;
   fRecvIOV[0].iov_len = size;
   fRecvSingle = singleoper;

   SetDoingInput(true);
   FireEvent(evntSocketRead);

   return true;
}

bool dabc::SocketIOProcessor::LetRecv(void* buf, size_t size, bool singleoper)
{
   if (fRecvIOVNumber>0) {
      EOUT(("Current recv operation not yet completed"));
      return false;
   }

   if (fRecvIOVSize<1) AllocateRecvIOV(8);

   fRecvUseMsg = false;
   fRecvIOVFirst = 0;
   fRecvIOVNumber = 1;
   fRecvIOV[0].iov_base = buf;
   fRecvIOV[0].iov_len = size;
   fRecvSingle = singleoper;
   SetDoingInput(true);

   return true;
}

bool dabc::SocketIOProcessor::StartSend(Buffer* buf, bool usemsg)
{
   // this is simple version,
   // where only buffer itself without header is transported

   return StartNetSend(0, 0, buf, usemsg);
}

bool dabc::SocketIOProcessor::StartRecv(Buffer* buf, BufferSize_t datasize, bool usemsg)
{
   return StartNetRecv(0, 0, buf, datasize, usemsg, false);
}

bool dabc::SocketIOProcessor::StartNetRecv(void* hdr, BufferSize_t hdrsize, Buffer* buf, BufferSize_t datasize, bool usemsg, bool singleoper)
{
   // datasize==0 here really means that there is no data to get !!!!

   if (fRecvIOVNumber>0) {
      EOUT(("Current recv operation not yet completed"));
      return false;
   }

   if (buf==0) return false;

   if (fRecvIOVSize<=buf->NumSegments()) AllocateRecvIOV(buf->NumSegments()+1);

   fRecvUseMsg = usemsg;
   fRecvIOVFirst = 0;
   fRecvSingle = singleoper;

   int indx = 0;

   if ((hdr!=0) && (hdrsize>0)) {
      fRecvIOV[indx].iov_base = hdr;
      fRecvIOV[indx].iov_len = hdrsize;
      indx++;
   }

   for (unsigned nseg=0; nseg<buf->NumSegments(); nseg++) {
      BufferSize_t segsize = buf->GetDataSize(nseg);
      if (segsize>datasize) segsize = datasize;
      if (segsize==0) break;

      fRecvIOV[indx].iov_base = buf->GetDataLocation(nseg);
      fRecvIOV[indx].iov_len = segsize;
      indx++;

      datasize-=segsize;
   }

   fRecvIOVNumber = indx;

   SetDoingInput(true);
   FireEvent(evntSocketRead);

   return true;
}

bool dabc::SocketIOProcessor::StartNetSend(void* hdr, BufferSize_t hdrsize, Buffer* buf, bool usemsg)
{
   if (fSendIOVNumber>0) {
      EOUT(("Current send operation not yet completed"));
      return false;
   }

   if (buf==0) return false;

   if (fSendIOVSize<=buf->NumSegments()) AllocateSendIOV(buf->NumSegments()+1);

   fSendUseMsg = usemsg;
   fSendIOVFirst = 0;

   int indx = 0;

   if ((hdr!=0) && (hdrsize>0)) {
      fSendIOV[indx].iov_base = hdr;
      fSendIOV[indx].iov_len = hdrsize;
      indx++;
   }

   for (unsigned nseg=0; nseg<buf->NumSegments(); nseg++) {
      fSendIOV[indx].iov_base = buf->GetDataLocation(nseg);
      fSendIOV[indx].iov_len = buf->GetDataSize(nseg);
      indx++;
   }

   fSendIOVNumber = indx;

   SetDoingOutput(true);
   FireEvent(evntSocketWrite);

   return true;
}

ssize_t dabc::SocketIOProcessor::DoRecvBuffer(void* buf, ssize_t len)
{
   ssize_t res = recv(fSocket, buf, len, MSG_DONTWAIT | MSG_NOSIGNAL);

   if (res==0) OnConnectionClosed(); else
   if (res<0) {
     if (errno!=EAGAIN) OnSocketError(errno, "When recv()");
   }

   return res;
}

ssize_t dabc::SocketIOProcessor::DoSendBuffer(void* buf, ssize_t len)
{
   ssize_t res = send(fSocket, buf, len, MSG_DONTWAIT | MSG_NOSIGNAL);

   if (res==0) OnConnectionClosed(); else
   if (res<0) {
     if (errno!=EAGAIN) OnSocketError(errno, "When send()");
   }

   return res;
}


void dabc::SocketIOProcessor::ProcessEvent(dabc::EventId evnt)
{
    switch (GetEventCode(evnt)) {
       case evntSocketRead: {

          if (fRecvIOVNumber==0)
             if (!OnRecvProvideBuffer()) return; // nothing to recv

          #ifdef SOCKET_PROFILING
             fRecvOper++;
             TimeStamp_t tm1 = TimeStamp();
          #endif

          fLastRecvSize = 0;
          ssize_t res = 0;

//          DOUT1(("Socket %d fRecvIOV = %p fRecvIOVFirst = %u number %u iov: %p %u",
//                fSocket, fRecvIOV, fRecvIOVFirst, fRecvIOVNumber,
//                fRecvIOV[fRecvIOVFirst].iov_base, fRecvIOV[fRecvIOVFirst].iov_len));

          if (fRecvUseMsg) {

             struct msghdr msg;

             msg.msg_name = 0;
             msg.msg_namelen = 0;
             msg.msg_iov = &(fRecvIOV[fRecvIOVFirst]);
             msg.msg_iovlen = fRecvIOVNumber - fRecvIOVFirst;
             msg.msg_control = 0;
             msg.msg_controllen = 0;
             msg.msg_flags = 0;

             res = recvmsg(fSocket, &msg, MSG_DONTWAIT | MSG_NOSIGNAL);
          } else
             res = recv(fSocket, fRecvIOV[fRecvIOVFirst].iov_base, fRecvIOV[fRecvIOVFirst].iov_len, MSG_DONTWAIT | MSG_NOSIGNAL);

          #ifdef SOCKET_PROFILING
             TimeStamp_t tm2 = TimeStamp();
             fRecvTime += TimeDistance(tm1, tm2);
             if (res>0) fRecvSize += res;
          #endif

          if (res==0) {
             OnConnectionClosed();
             return;
          }

          if (res<0) {
             if (errno!=EAGAIN) OnSocketError(errno, "When recvmsg()");
             return;
          }

          fLastRecvSize = res;

          if (fRecvSingle) {
             // for datagram the only recv message is possible
             fRecvIOVFirst = 0;
             fRecvIOVNumber = 0;
             fRecvSingle = false;
             SetDoingInput(false);
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
                   if (res!=0) EOUT(("Internal error - length after recvmsg() not zero"));

                   fRecvIOVFirst = 0;
                   fRecvIOVNumber = 0;
                   SetDoingInput(false);

                   OnRecvCompleted();

                   return;
                }
             } else {
                rec->iov_len -= res;
                rec->iov_base = (char*)rec->iov_base + res;
                res = 0;
             }
          }

          break;
       }

       case evntSocketWrite: {

          if (fSendIOVNumber==0) return; // nothing to send

          #ifdef SOCKET_PROFILING
             fSendOper++;
             TimeStamp_t tm1 = TimeStamp();
          #endif

          ssize_t res = 0;

          if (fSendUseMsg) {

             struct msghdr msg;

             msg.msg_name = 0;
             msg.msg_namelen = 0;
             msg.msg_iov = &(fSendIOV[fSendIOVFirst]);
             msg.msg_iovlen = fSendIOVNumber - fSendIOVFirst;
             msg.msg_control = 0;
             msg.msg_controllen = 0;
             msg.msg_flags = 0;

             res = sendmsg(fSocket, &msg, MSG_DONTWAIT | MSG_NOSIGNAL);
          } else
             res = send(fSocket, fSendIOV[fSendIOVFirst].iov_base, fSendIOV[fSendIOVFirst].iov_len, MSG_DONTWAIT | MSG_NOSIGNAL);

          #ifdef SOCKET_PROFILING
             TimeStamp_t tm2 = TimeStamp();
             fSendTime += TimeDistance(tm1, tm2);
             if (res>0) fSendSize += res;
          #endif


          if (res==0) {
             OnConnectionClosed();
             return;
          }

          if (res<0) {
             if (errno!=EAGAIN) OnSocketError(errno, "When sendmsg()");
             return;
          }

          DOUT5(("Socket %d send %d bytes", Socket(), res));

          while (res>0) {
             struct iovec* rec = &(fSendIOV[fSendIOVFirst]);

             if (rec->iov_len <= (unsigned) res) {
                // just skip current rec, jump to next
                res -= rec->iov_len;
                fSendIOVFirst++;

                if (fSendIOVFirst==fSendIOVNumber) {
                   if (res!=0) EOUT(("Internal error - length after sendmsg() not zero"));

                   fSendIOVFirst = 0;
                   fSendIOVNumber = 0;
                   SetDoingOutput(false);

                   OnSendCompleted();

                   return;
                }
             } else {
                rec->iov_len -= res;
                rec->iov_base = (char*)rec->iov_base + res;
                res = 0;
             }
          }

          break;
       }
       default:
          SocketProcessor::ProcessEvent(evnt);
    }
}

// ___________________________________________________________________

dabc::SocketServerProcessor::SocketServerProcessor(int serversocket, int portnum) :
   SocketConnectProcessor(serversocket),
   fServerPortNumber(portnum),
   fServerHostName()
{
   fServerHostName = dabc::SocketDevice::GetLocalHost(true);

//   dabc::SocketThread::DefineHostName();

   SetDoingInput(true);
   listen(Socket(), 10);
}

void dabc::SocketServerProcessor::ProcessEvent(dabc::EventId evnt)
{
    switch (GetEventCode(evnt)) {
       case evntSocketRead: {
          int connfd = accept(Socket(), 0, 0);
          if (connfd<0) {
             EOUT(("Error with accept"));
             return;
          }

          listen(Socket(), 10);

          if (!dabc::SocketThread::SetNonBlockSocket(connfd)) {
             EOUT(("Cannot set nonblocking flag for connected socket"));
             close(connfd);
             return;
          }

          DOUT3(("We get new connection with fd: %d", connfd));

          OnClientConnected(connfd);

          break;
       }

       default:
          dabc::SocketProcessor::ProcessEvent(evnt);
    }
}

void dabc::SocketServerProcessor::OnClientConnected(int fd)
{
   if (GetConnRecv()!=0) {
      Command* cmd = new Command("SocketConnect");
      cmd->SetStr("Type", "Server");
      cmd->SetInt("fd", fd);
      cmd->SetStr("ConnId", GetConnId());
      GetConnRecv()->Submit(cmd);
   } else {
      EOUT(("Method not implemented - socked will be closed"));
      close(fd);
   }
}

// _____________________________________________________________________

dabc::SocketClientProcessor::SocketClientProcessor(const struct addrinfo* serv_addr, int fd) :
   SocketConnectProcessor(fd),
   fRetry(1),
   fRetryTmout(-1)
{
   if (serv_addr==0) {
      EOUT(("Server address not specified"));
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

dabc::SocketClientProcessor::~SocketClientProcessor()
{
   free(fServAddr.ai_addr); fServAddr.ai_addr = 0;
   free(fServAddr.ai_canonname); fServAddr.ai_canonname = 0;
}

void dabc::SocketClientProcessor::SetRetryOpt(int nretry, double tmout)
{
   fRetry = nretry;
   fRetryTmout = tmout;
}

void dabc::SocketClientProcessor::OnThreadAssigned()
{
   FireEvent(evntSocketStartConnect);
}

void dabc::SocketClientProcessor::ProcessEvent(dabc::EventId evnt)
{
   switch (GetEventCode(evnt)) {
       case evntSocketWrite: {
          // we can check if connection established

          if (!IsDoingOutput()) return;

          int myerrno = TakeSocketError();

          if (myerrno==0) {
             DOUT5(("Connection done %7.1f", TimeStamp()*1e-3));
             int fd = TakeSocket();
             OnConnectionEstablished(fd);
             return;
          }

          ShowSocketError("Postponed connect", myerrno);

          break;
       }

       case evntSocketError: {
          if (!IsDoingOutput()) return;

          int myerrno = TakeSocketError();
          ShowSocketError("Doing connect", myerrno);

          break;
       }

       case evntSocketStartConnect: {
          // start next attemp for connection

          if (IsDoingOutput()) {
             EOUT(("Something wrong !!!!"));
             return;
          }

          if (Socket()<=0) {
             int fd = socket(fServAddr.ai_family, fServAddr.ai_socktype, fServAddr.ai_protocol);

             if (fd<=0)
                EOUT(("Cannot create socket of given addr"));
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
                DOUT5(("Connection in progress %7.1f", TimeStamp()*1e-3));
                SetDoingOutput(true);
                return;
             }

             ShowSocketError("When calling connection", errno);
          }

          break;
       }

       default:
          dabc::SocketProcessor::ProcessEvent(evnt);
          return;
   }

   SetDoingOutput(false);
   CloseSocket();

   if (--fRetry>0) {
      DOUT3(("Try connect after %5.1f s  n:%d", fRetryTmout, fRetry));

      ActivateTimeout(fRetryTmout > 0. ? fRetryTmout : 0.);
   } else {
      OnConnectionFailed();
   }
}

double dabc::SocketClientProcessor::ProcessTimeout(double)
{
   // activate connection start again

   FireEvent(evntSocketStartConnect);
   return -1.;
}

void dabc::SocketClientProcessor::OnConnectionEstablished(int fd)
{
   if (GetConnRecv()!=0) {
      Command* cmd = new Command("SocketConnect");
      cmd->SetStr("Type", "Client");
      cmd->SetInt("fd", fd);
      cmd->SetStr("ConnId", GetConnId());
      GetConnRecv()->Submit(cmd);
   } else {
      EOUT(("Connection established, but not processed - close socket"));
      close(fd);
   }

   DestroyProcessor();
}

void dabc::SocketClientProcessor::OnConnectionFailed()
{
   if (GetConnRecv()!=0) {
      Command* cmd = new Command("SocketConnect");
      cmd->SetStr("Type", "Error");
      cmd->SetStr("ConnId", GetConnId());
      GetConnRecv()->Submit(cmd);
   } else {
      EOUT(("Connection failed to establish, error not processed"));
   }

   DestroyProcessor();
}

// _______________________________________________________________________

dabc::SocketThread::SocketThread(Basic* parent, const char* name, int numqueues) :
   dabc::WorkingThread(parent, name, numqueues),
   fFireCounter(0),
   fPipeFired(false),
   fWaitFire(false),
   f_sizeufds(0),
   f_ufds(0),
   f_recs(0)
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
   ProcessorNumberChanged();

}

dabc::SocketThread::~SocketThread()
{
   // !!!!!! we should stop thread before destroying anything while
   // thread may use some structures in the MainLoop !!!!!!!!

   Stop(true);

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
     DOUT1(("Thrd:%s Wait called %ld done %ld ratio %5.3f %s  Pipe:%ld", GetName(), fWaitCalls, fWaitDone, (fWaitCalls>0 ? 100.*fWaitDone/fWaitCalls : 0.) ,"%", fPipeCalled));
     if (fWaitDone>0)
        DOUT1(("Aver times fill:%5.1f microsec wait:%5.1f microsec", fFillTime*1e6/fWaitDone, fWaitTime*1e6/fWaitDone));
   #endif
}

bool dabc::SocketThread::CompatibleClass(const char* clname) const
{
   if (WorkingThread::CompatibleClass(clname)) return true;
   return strcmp(clname, typeSocketThread) == 0;
}

bool dabc::SocketThread::SetNonBlockSocket(int fd)
{
   int opts = fcntl(fd, F_GETFL);
   if (opts < 0) {
      EOUT(("fcntl(F_GETFL) failed"));
      return false;
   }
   opts = (opts | O_NONBLOCK);
   if (fcntl(fd, F_SETFL,opts) < 0) {
      EOUT(("fcntl(F_SETFL) failed"));
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

     DOUT1(("GetAddrInfo %s:%s res = %d", (myhostname ? myhostname : "---"), service, n));

     if (n < 0) {
        EOUT(("Cannot get addr info for service %s:%s", (myhostname ? myhostname : "localhost"), service));
        continue;
     }

     for (struct addrinfo *t = info; t; t = t->ai_next) {

        sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
        if (sockfd >= 0) {

           int opt = 1;

           setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

           if (!bind(sockfd, t->ai_addr, t->ai_addrlen)) break;
           close(sockfd);
           sockfd = -1;
        }
     }

     freeaddrinfo(info);

     if (sockfd<0) {
       DOUT3(("Cannot bind socket to service %s", service));
       continue;
     }

     if (dabc::SocketThread::SetNonBlockSocket(sockfd)) return sockfd;

     EOUT(("Cannot set nonblocking flag for server socket"));
     close(sockfd);
   }

   EOUT(("Cannot bind server socket to port %d or find its in range %d:%d", firsttest, portmin, portmax));
   portnum = -1;
   return -1;
}

std::string dabc::SocketThread::DefineHostName()
{
   char hostname[1000];
   if (gethostname(hostname, sizeof(hostname)))
      EOUT(("ERROR gethostname"));
   else
      DOUT3(( "gethostname = %s", hostname));
   return std::string(hostname);
}

dabc::SocketServerProcessor* dabc::SocketThread::CreateServerProcessor(int nport, int portmin, int portmax)
{
   int fd = dabc::SocketThread::StartServer(nport, portmin, portmax);

   if (fd<0) return 0;

   return new SocketServerProcessor(fd, nport);
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

   if (getaddrinfo(host, service, &hints, &info)==0) {
      for (struct addrinfo *t = info; t; t = t->ai_next) {

         int sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);

         if (sockfd > 0){
            if (connect(sockfd, t->ai_addr, t->ai_addrlen)==0){
               if (dabc::SocketThread::SetNonBlockSocket(sockfd))
                  return sockfd;
               else
                  EOUT(("Cannot set non-blocking flag for client socket"));
            }
         }
         close(sockfd);
         sockfd = -1;
      }
      freeaddrinfo(info);
   }

   return -1;
}

int dabc::SocketThread::StartMulticast(const char* host, int port, bool isrecv)
{
   if ((host==0) || (strlen(host)==0)) {
      host = "224.0.0.15";
      EOUT(("Multicast address not specified, use %s", host));
   }

   if (port<=0) {
      port = 4576;
      EOUT(("Multicast port not specified, use %d", port));
   }

   struct hostent *server_host_name = gethostbyname(host);
   if (server_host_name==0) {
      EOUT(("Cannot get host information for %s", host));
      return -1;
   }

   int socket_descriptor = socket (PF_INET, SOCK_DGRAM, 0);
   if (socket_descriptor<0) {
      EOUT(("Cannot create datagram socket"));
      return -1;
   }

   if (isrecv) {

      struct sockaddr_in sin;
      struct ip_mreq command;

      memset (&sin, 0, sizeof (sin));
      sin.sin_family = PF_INET;
      sin.sin_addr.s_addr = htonl (INADDR_ANY);
      sin.sin_port = htons (port);
      // Allow to use same port by many processes
      int loop = 1;
      if (setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR,
                     &loop, sizeof (loop)) < 0) {
          EOUT(("Cannot setsockopt SO_REUSEADDR"));
      }
      if(bind(socket_descriptor, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
         EOUT(("Fail to bind socket with port %d", port));
         close(socket_descriptor);
         return -1;
      }

      // Allow to receive broadcast to this port
      loop = 1;
      if (setsockopt (socket_descriptor, IPPROTO_IP, IP_MULTICAST_LOOP,
                      &loop, sizeof (loop)) < 0) {
         EOUT(("Fail setsockopt IP_MULTICAST_LOOP"));
         close(socket_descriptor);
         return -1;
      }

      // Join the multicast group
      command.imr_multiaddr.s_addr = inet_addr (host);
      command.imr_interface.s_addr = htonl (INADDR_ANY);
      if (command.imr_multiaddr.s_addr == (in_addr_t)-1) {
         EOUT(("%s is not valid address", host));
         close(socket_descriptor);
         return -1;
      }
      if (setsockopt(socket_descriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                     &command, sizeof (command)) < 0) {
          EOUT(("File setsockopt IP_ADD_MEMBERSHIP"));
          close(socket_descriptor);
          return -1;
      }
   } else {
     struct sockaddr_in address;

     memset (&address, 0, sizeof (address));
     address.sin_family = AF_INET;
     address.sin_addr.s_addr = inet_addr (host);
     address.sin_port = htons (port);

     if (connect(socket_descriptor, (struct sockaddr *) &address,
           sizeof (address)) < 0) {
        EOUT(("Fail to connect to host %s port %d", host, port));
        close(socket_descriptor);
        return -1;
     }
  }

  return socket_descriptor;
}

void dabc::SocketThread::CloseMulticast(int handle, const char* host, bool isrecv)
{
   if (handle<0) return;

   if (isrecv) {

      struct ip_mreq command;

      command.imr_multiaddr.s_addr = inet_addr (host);
      command.imr_interface.s_addr = htonl (INADDR_ANY);

      // Remove socket from multicast group
      if (setsockopt (handle, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                      &command, sizeof (command)) < 0 ) {
         EOUT(("Fail setsockopt:IP_DROP_MEMBERSHIP"));
      }
   }

   if (close(handle) < 0)
      EOUT(("Error in socketclose"));
}


int dabc::SocketThread::StartUdp(int& portnum, int portmin, int portmax)
{

   int fd = socket(PF_INET, SOCK_DGRAM, 0);
   if (fd<0) return -1;

   struct sockaddr_in m_addr;
   int numtests = 1; // at least test value of portnum
   if ((portmin>0) && (portmax>0) && (portmin<=portmax)) numtests+=(portmax-portmin+1);

   if (SetNonBlockSocket(fd))
      for(int ntest=0;ntest<numtests;ntest++) {
         if ((ntest==0) && (portnum<0)) continue;
         if (ntest>0) portnum = portmin - 1 + ntest;

         memset(&m_addr, 0, sizeof(m_addr));
         m_addr.sin_family = AF_INET;
         m_addr.sin_port = htons(portnum);

         if (!bind(fd, (struct sockaddr *)&m_addr, sizeof(m_addr))) return fd;
      }

   close(fd);
   return -1;
}

int dabc::SocketThread::ConnectUdp(int fd, const char* remhost, int remport)
{
   if (fd<0) return fd;

   struct hostent *host = gethostbyname(remhost);
   if ((host==0) || (host->h_addrtype!=AF_INET)) {
      EOUT(("Cannot get host information for %s", remhost));
      close(fd);
      return -1;
   }

   struct sockaddr_in address;

   memset (&address, 0, sizeof (address));
   address.sin_family = AF_INET;
   memcpy(&address.sin_addr.s_addr, host->h_addr_list[0], host->h_length);
   address.sin_port = htons (remport);

   if (connect(fd, (struct sockaddr *) &address, sizeof (address)) < 0) {
      EOUT(("Fail to connect to host %s port %d", remhost, remport));
      close(fd);
      return -1;
   }
   return fd;
}


dabc::SocketClientProcessor* dabc::SocketThread::CreateClientProcessor(const char* serverid)
{
   if (serverid==0) return 0;

   const char* service = strchr(serverid, ':');
   if (service==0) return 0;

   std::string host;
   host.assign(serverid, service - serverid);
   service++;

   SocketClientProcessor* proc = 0;

   DOUT5(("CreateClientProcessor %s:%s", service, serverid));

   struct addrinfo *info;
   struct addrinfo hints;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   if (getaddrinfo(host.c_str(), service, &hints, &info)==0) {
      for (struct addrinfo *t = info; t; t = t->ai_next) {

         int sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);

         if (sockfd<=0) continue;

         proc = new SocketClientProcessor(t, sockfd);
         break;
      }
      freeaddrinfo(info);
   }

   DOUT5(("CreateClientProcessor %s:%s done res = %p", service, serverid, proc));

   return proc;
}


void dabc::SocketThread::_Fire(dabc::EventId arg, int nq)
{
   DOUT5(("SocketThread::_Fire %s nq:%d numq:%d waiting:%s", GetName(), nq, fNumQueues, DBOOL(fWaitFire)));

   _PushEvent(arg, nq);
   fFireCounter++;

   if (fWaitFire && !fPipeFired) {
      write(fPipe[1], "w", 1);
      fPipeFired = true;

      #ifdef SOCKET_PROFILING
         fPipeCalled++;
      #endif
   }
}

dabc::EventId dabc::SocketThread::WaitEvent(double tmout_sec)
{
   // first check, if we have already event, which must be processed

   #ifdef SOCKET_PROFILING
     fWaitCalls++;

     TimeStamp_t tm1 = TimeStamp();
   #endif

   {
      dabc::LockGuard lock(fWorkMutex);

      if (fFireCounter>0) {
         fFireCounter--;
         return _GetNextEvent();
      }

      if (f_ufds==0) return NullEventId;

      fWaitFire = true;
   }

   // here we wait for next event from any socket, including pipe

   int numufds = 1;

   f_ufds[0].fd = fPipe[0];
   f_ufds[0].events = POLLIN;
   f_ufds[0].revents = 0;

   for(unsigned n=1; n<f_sizeufds; n++) {
      if (!f_recs[n].use) continue;
      SocketProcessor* proc = (SocketProcessor*) fProcessors[n];

      if (proc->Socket()<=0) continue;

      short events = 0;

      if (proc->IsDoingInput())
         events = events | POLLIN;

      if (proc->IsDoingOutput())
         events = events | POLLOUT;

      if (events==0) continue;

      f_ufds[numufds].fd = proc->Socket();
      f_ufds[numufds].events = events;
      f_ufds[numufds].revents = 0;

      f_recs[numufds].indx = n; // this is for dereferencing of the value

      numufds++;
   }

   int tmout = tmout_sec < 0 ? -1 : int(tmout_sec*1000.);

   #ifdef SOCKET_PROFILING
     fWaitDone++;
     TimeStamp_t tm2 = TimeStamp();

     fFillTime += TimeDistance(tm1, tm2);
   #endif

   int poll_res = poll(f_ufds, numufds, tmout);

   #ifdef SOCKET_PROFILING
     TimeStamp_t tm3 = TimeStamp();
     fWaitTime += TimeDistance(tm2, tm3);
   #endif

   dabc::LockGuard lock(fWorkMutex);

   fWaitFire = false;

   // cleanup pipe in bigger steps
   if (fPipeFired) {
      char sbuf;
      read(fPipe[0], &sbuf, 1);
      fPipeFired = false;
   }

   // if we really has any events, analyse all of them and push in the queue
   if (poll_res>0)
      for (int n=1; n<numufds;n++) {

         if (f_ufds[n].revents==0) continue;

         if (f_ufds[n].revents & (POLLERR | POLLHUP | POLLNVAL)) {
//            EOUT(("Error on the socket %d", f_ufds[n].fd));
            _PushEvent(CodeEvent(SocketProcessor::evntSocketError, f_recs[n].indx), 0);
            fFireCounter++;
         }

         if (f_ufds[n].revents & (POLLIN | POLLPRI)) {
            _PushEvent(CodeEvent(SocketProcessor::evntSocketRead, f_recs[n].indx), 1);
            fFireCounter++;
         }

         if (f_ufds[n].revents & POLLOUT) {
            _PushEvent(CodeEvent(SocketProcessor::evntSocketWrite, f_recs[n].indx), 1);
            fFireCounter++;
         }
      }

   if (fFireCounter==0) return NullEventId;
   fFireCounter--;
   return _GetNextEvent();
}

void dabc::SocketThread::ProcessorNumberChanged()
{
   unsigned new_sz = fProcessors.size();

   if (new_sz>f_sizeufds) {

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

   for (unsigned indx=1;indx<fProcessors.size();indx++) {
      SocketProcessor* proc = dynamic_cast<SocketProcessor*> (fProcessors[indx]);

      f_recs[indx].use = proc!=0;
   }

   DOUT5(("ProcessorNumberChanged finished"));
}
