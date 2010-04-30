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
#ifndef DABC_SocketThread
#define DABC_SocketThread

#ifndef DABC_WorkingProcessor
#include "dabc/WorkingProcessor.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

struct pollfd;

// #define SOCKET_PROFILING


namespace dabc {

   class SocketThread;

   class SocketProcessor : public WorkingProcessor {

      friend class SocketThread;

      public:

         enum ESocketEvents {
            evntSocketRead = evntFirstSystem,
            evntSocketWrite,
            evntSocketError,
            evntSocketStartConnect,
            evntSocketLast // from this event number one can add more socket system events
         };

         SocketProcessor(int fd = -1);

         virtual ~SocketProcessor();

         virtual const char* RequiredThrdClass() const { return typeSocketThread; }

         inline int Socket() const { return fSocket; }

         virtual void ProcessEvent(dabc::EventId);

         void CloseSocket();

         void SetSocket(int fd);
         int TakeSocket();
         int TakeSocketError();

      protected:

         virtual void OnConnectionClosed();
         virtual void OnSocketError(int errnum, const char* info);

         inline bool IsDoingInput() const { return fDoingInput; }
         inline bool IsDoingOutput() const { return fDoingOutput; }

         inline void SetFlags(bool isinp, bool isout)
         {
            fDoingInput = isinp;
            fDoingOutput = isout;
         }

         inline void SetDoingInput(bool on = true) { fDoingInput = on; }
         inline void SetDoingOutput(bool on = true) { fDoingOutput = on; }
         inline void SetDoingNothing() { SetFlags(false, false); }
         inline void SetDoingAll() { SetFlags(true, true); }

         int           fSocket;
         bool          fDoingInput;
         bool          fDoingOutput;
   };

   // ______________________________________________________________________________________


   class SocketIOProcessor : public SocketProcessor {

      public:
         SocketIOProcessor(int fd = 0);

         virtual ~SocketIOProcessor();

         virtual void ProcessEvent(EventId);

         bool StartSend(void* buf, size_t size, bool usemsg = false);
         bool StartRecv(void* buf, size_t size, bool usemsg = false, bool singleoper = false);
         bool LetRecv(void* buf, size_t size, bool singleoper = true);

         bool StartSend(Buffer* buf, bool usemsg = false);
         bool StartRecv(Buffer* buf, BufferSize_t datasize, bool usemsg = false);

         bool StartNetRecv(void* hdr, BufferSize_t hdrsize, Buffer* buf, BufferSize_t datasize, bool usemsg = true, bool singleoper = false);
         bool StartNetSend(void* hdr, BufferSize_t hdrsize, Buffer* buf, bool usemsg = true);

      protected:

         void AllocateSendIOV(unsigned size);
         void AllocateRecvIOV(unsigned size);

         virtual bool OnRecvProvideBuffer() { return false; }
         virtual void OnSendCompleted() {}
         virtual void OnRecvCompleted() {}

         inline bool IsDoingSend() const { return fSendIOVNumber>0; }
         inline bool IsDoingRecv() const { return fRecvIOVNumber>0; }

         ssize_t DoRecvBuffer(void* buf, ssize_t len);
         ssize_t DoRecvBufferHdr(void* hdr, ssize_t hdrlen, void* buf, ssize_t len);
         ssize_t DoSendBuffer(void* buf, ssize_t len);

         bool          fSendUseMsg;     // use sendmsg for transport
         struct iovec* fSendIOV;        // sending io vector for gather list
         unsigned      fSendIOVSize;    // total number of elements in send vector
         unsigned      fSendIOVFirst;   // number of element in send IOV where transfer is started
         unsigned      fSendIOVNumber;  // number of elements in current send operation
         // receiving data
         bool          fRecvUseMsg;     // use recvmsg for transport
         struct iovec* fRecvIOV;        // receive io vector for scatter list
         unsigned      fRecvIOVSize;    // number of elements in recv vector
         unsigned      fRecvIOVFirst;   // number of element in recv IOV where transfer is started
         unsigned      fRecvIOVNumber;  // number of elements in current recv operation
         bool          fRecvSingle;     // true if the only recv is allowed
         unsigned      fLastRecvSize;   // size of last recv opearation

#ifdef SOCKET_PROFILING
         long           fSendOper;
         double         fSendTime;
         long           fSendSize;
         long           fRecvOper;
         double         fRecvTime;
         long           fRecvSize;
#endif

   };

   // ______________________________________________________________________


   class SocketConnectProcessor : public SocketProcessor {
      protected:
         WorkingProcessor* fConnRcv;
         std::string fConnId;

      public:
         SocketConnectProcessor(int fd) :
            SocketProcessor(fd),
            fConnRcv(0),
            fConnId()
         {
         }

         void SetConnHandler(WorkingProcessor* rcv, const char* connid)
         {
            fConnRcv = rcv;
            fConnId = connid;
         }

         WorkingProcessor* GetConnRecv() const { return fConnRcv; }
         const char* GetConnId() const { return fConnId.c_str(); }
   };

   // ________________________________________________________________


   // this object establish server socket, which waits for new connection
   // of course, we do not want to block complete thread for such task :-)

   class SocketServerProcessor : public SocketConnectProcessor {
      public:
         SocketServerProcessor(int serversocket, int portnum = -1);

         virtual void ProcessEvent(dabc::EventId);

         int ServerPortNumber() const { return fServerPortNumber; }
         const char* ServerHostName() { return fServerHostName.c_str(); }
         std::string ServerId() { return dabc::format("%s:%d", ServerHostName(), ServerPortNumber()); }

      protected:

         virtual void OnClientConnected(int fd);

         int  fServerPortNumber;
         std::string fServerHostName;
   };

   // ______________________________________________________________

   class SocketClientProcessor : public SocketConnectProcessor {
      public:
         SocketClientProcessor(const struct addrinfo* serv_addr, int fd = -1);
         virtual ~SocketClientProcessor();

         void SetRetryOpt(int nretry, double tmout = 1.);

         virtual void ProcessEvent(dabc::EventId);

         virtual double ProcessTimeout(double);

      protected:
         virtual void OnConnectionEstablished(int fd);
         virtual void OnConnectionFailed();

         virtual void OnThreadAssigned();

         struct addrinfo fServAddr; // own copy of server address
         int             fRetry;
         double          fRetryTmout;
   };

   // ______________________________________________________________


   class SocketThread : public WorkingThread {

      public:

         // list of all events for all kind of socket processors

         SocketThread(Basic* parent, const char* name = "SocketThrd", int numqueues = 3);
         virtual ~SocketThread();

         virtual const char* ClassName() const { return typeSocketThread; }
         virtual bool CompatibleClass(const char* clname) const;

         static bool SetNonBlockSocket(int fd);
         static int StartServer(int& nport, int portmin=-1, int portmax=-1);
         static int StartUdp(int& nport, int portmin=-1, int portmax=-1);
         static std::string DefineHostName();
         static int StartClient(const char* host, int nport);
         static int StartMulticast(const char* host, int port, bool isrecv = true);
         static void CloseMulticast(int handle, const char* host, bool isrecv = true);
         static int ConnectUdp(int fd, const char* remhost, int remport);


         static SocketServerProcessor* CreateServerProcessor(int nport, int portmin=-1, int portmax=-1);

         static SocketClientProcessor* CreateClientProcessor(const char* servid);

      protected:

         struct ProcRec {
             bool      use;  // indicates if processor is used for poll
             uint32_t  indx; // index for dereference of processor from ufds structure
         };

         virtual dabc::EventId WaitEvent(double tmout);

         virtual void _Fire(dabc::EventId arg, int nq);

         virtual void ProcessorNumberChanged();

         int            fPipe[2];
         long           fFireCounter;
         long           fPipeFired;  // indicate if somthing was written in pipe
         bool           fWaitFire;
         unsigned       f_sizeufds;  // size of the structure, which was allocated
         pollfd        *f_ufds;      // list of file descriptors for poll call
         ProcRec       *f_recs;      // identify used processors

#ifdef SOCKET_PROFILING
         long           fWaitCalls;
         long           fWaitDone;
         long           fPipeCalled;
         double         fWaitTime;
         double         fFillTime;
#endif
   };
}

#endif
