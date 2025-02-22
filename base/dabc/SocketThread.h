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

#ifndef DABC_SocketThread
#define DABC_SocketThread

#ifndef DABC_Thread
#include "dabc/Thread.h"
#endif

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#include <netdb.h>

struct pollfd;

// #define SOCKET_PROFILING


namespace dabc {

   class SocketThread;

   /** \brief Special addon class for handling of socket and socket events
    *
    * \ingroup dabc_all_classes
    *
    * Main aim is to provide asynchronous access to the sockets.
    * SocketAddon has two boolean variables which indicates if worker wants to send or receive
    * data from the socket. User should set these flags to true before starting of appropriate operation.
    * Thread will use these flags to check if such operation can be performed now. If yes,
    * event evntSocketRead or evntSocketWrite will be generated. User should implement ProcessEvent()
    * to react on this even and either read or write data on the sockets. Flag will be cleared, therefore
    * if operation should be continued, user should set flag again. In case of error evntSocketError
    * will be created and both flags will be cleared.
    */

   class SocketAddon : public WorkerAddon {

      friend class SocketThread;

      protected:

         int           fSocket{0};                     ///< socket handle
         bool          fDoingInput{false};             ///< true if input data are expected
         bool          fDoingOutput{false};            ///< true if data need to be send
         int           fIOPriority{0};                 ///< priority of socket I/O events, default 1
         bool          fDeliverEventsToWorker{false};  ///< if true, completion events will be delivered to the worker
         bool          fDeleteWorkerOnClose{false};    ///< if true, worker will be deleted when socket closed or socket in error

         void ProcessEvent(const EventId &) override;

         /** \brief Call method to indicate that object wants to read data from the socket.
          * When it will be possible, worker get evntSocketRead event */
         inline void SetDoingInput(bool on = true) { fDoingInput = on; }

         /** \brief Call method to indicate that worker wants to write data to the socket.
          * When it will be possible, worker get evntSocketWrite event */
         inline void SetDoingOutput(bool on = true) { fDoingOutput = on; }

         /** Generic error handler. Also invoked when socket is closed (msg == 0) */
         virtual void OnSocketError(int msg, const std::string &info);

         ssize_t DoRecvBuffer(void* buf, ssize_t len);
         ssize_t DoRecvBufferHdr(void* hdr, ssize_t hdrlen, void* buf, ssize_t len, void* srcaddr = nullptr, unsigned srcaddrlen = 0);
         ssize_t DoSendBuffer(void* buf, ssize_t len);
         ssize_t DoSendBufferHdr(void* hdr, ssize_t hdrlen, void* buf, ssize_t len, void* tgtaddr = nullptr, unsigned tgtaddrlen = 0);

         bool IsDeleteWorkerOnClose() const { return fDeleteWorkerOnClose; }
         void SetDeleteWorkerOnClose(bool on = true) { fDeleteWorkerOnClose = on; }

      public:

         enum ESocketEvents {
            evntSocketRead = Worker::evntFirstAddOn,
            evntSocketWrite,
            evntSocketError,
            evntSocketStartConnect,
            evntSocketLast,             ///< from this event number one can add more socket system events
            evntSocketRecvInfo  = Worker::evntFirstSystem,   ///< event delivered to worker when read is completed
            evntSocketSendInfo,         ///< event delivered to worker when write is completed
            evntSocketErrorInfo,        ///< event delivered to worker when error is detected
            evntSocketCloseInfo,         ///< event delivered to worker when socket is closed
            evntSocketLastInfo           ///< last system event, used by sockets
         };

         SocketAddon(int fd = -1);
         virtual ~SocketAddon();

         const char *ClassName() const override { return "SocketAddon"; }
         std::string RequiredThrdClass() const override { return typeSocketThread; }

         inline int Socket() const { return fSocket; }
         inline bool IsSocket() const { return Socket() >= 0; }

         inline bool IsDoingInput() const { return fDoingInput; }
         inline bool IsDoingOutput() const { return fDoingOutput; }

         void CloseSocket();
         void SetSocket(int fd);

         int TakeSocket();
         int TakeSocketError();

         bool IsDeliverEventsToWorker() const { return fDeliverEventsToWorker; }
         void SetDeliverEventsToWorker(bool on = true) { fDeliverEventsToWorker = on; }

         /** \brief Method defines priority level for socket IO events.
          * Allowed values are from 0-maximal, 1-normal or 2-minimal */
         inline void SetIOPriority(int prior = 1) { fIOPriority = prior; }
   };


   class SocketAddonRef : public WorkerAddonRef {
      DABC_REFERENCE(SocketAddonRef, WorkerAddonRef, SocketAddon)

      bool IsSocket() const
         { return GetObject() ? GetObject()->IsSocket() : false; }
   };


   // ______________________________________________________________________

   /** \brief Socket addon for handling I/O events
    *
    * \ingroup dabc_all_classes
    */

   class SocketIOAddon : public SocketAddon {
      protected:
         bool          fDatagramSocket{false}; ///< indicate if socket is datagram and all operations should be finished with single call
         bool          fUseMsgOper{false};     ///< indicate if sendmsg, recvmsg operations should be used, it is must for the datagram sockets

         // sending data
         bool          fSendUseMsg{false};     ///< use sendmsg for transport
         struct iovec* fSendIOV{nullptr};        ///< sending io vector for gather list
         unsigned      fSendIOVSize{0};    ///< total number of elements in send vector
         unsigned      fSendIOVFirst{0};   ///< number of element in send IOV where transfer is started
         unsigned      fSendIOVNumber{0};  ///< number of elements in current send operation
         struct sockaddr_in fSendAddr;  ///< optional send address for next send operation
         bool          fSendUseAddr{false};    ///< if true, fSendAddr will be used

         // receiving data
         bool          fRecvUseMsg{false};     ///< use recvmsg for transport
         struct iovec* fRecvIOV{nullptr};        ///< receive io vector for scatter list
         unsigned      fRecvIOVSize{0};    ///< number of elements in recv vector
         unsigned      fRecvIOVFirst{0};   ///< number of element in recv IOV where transfer is started
         unsigned      fRecvIOVNumber{0};  ///< number of elements in current recv operation
         struct sockaddr_in fRecvAddr;  ///< source address of last receive operation
         unsigned      fLastRecvSize{0};   ///< size of last recv operation

#ifdef SOCKET_PROFILING
         long           fSendOper;
         double         fSendTime;
         long           fSendSize;
         long           fRecvOper;
         double         fRecvTime;
         long           fRecvSize;
#endif

         const char *ClassName() const override { return "SocketIOAddon"; }

         bool IsDatagramSocket() const { return fDatagramSocket; }

         void ProcessEvent(const EventId&) override;

         void AllocateSendIOV(unsigned size);
         void AllocateRecvIOV(unsigned size);

         inline bool IsDoingSend() const { return fSendIOVNumber > 0; }
         inline bool IsDoingRecv() const { return fRecvIOVNumber > 0; }

         /** \brief Method called when send operation is completed */
         virtual void OnSendCompleted()
         {
            if (IsDeliverEventsToWorker()) FireWorkerEvent(evntSocketSendInfo);
         }

         /** \brief Method called when receive operation is completed */
         virtual void OnRecvCompleted()
         {
            if (IsDeliverEventsToWorker()) FireWorkerEvent(evntSocketRecvInfo);
         }

         /** \brief Method provide address of last receive operation */
         struct sockaddr_in& GetRecvAddr() { return fRecvAddr; }

         /** \brief Method return size of last buffer read from socket. Useful
          * only for datagram sockets, which can reads complete packet at once  */
         unsigned GetRecvSize() const { return fLastRecvSize; }

      public:

         /** \brief Constructor of SocketIOAddon class */
         SocketIOAddon(int fd = 0, bool isdatagram = false, bool usemsg = true);

         /** \brief Destructor of SocketIOAddon class */
         virtual ~SocketIOAddon();

         /** \brief Set destination address for all send operations,
          * \details Can be only specified for datagram sockets */
         void SetSendAddr(const std::string &host = "", int port = 0);

         bool StartSend(const void* buf, unsigned size,
                        const void* buf2 = nullptr, unsigned size2 = 0,
                        const void* buf3 = nullptr, unsigned size3 = 0);
         bool StartRecv(void* buf, size_t size);

         bool StartRecvHdr(void* hdr, unsigned hdrsize, void* buf, size_t size);

         bool StartSend(const Buffer& buf);
         bool StartRecv(Buffer& buf, BufferSize_t datasize);

         bool StartNetRecv(void* hdr, unsigned hdrsize, Buffer& buf, BufferSize_t datasize);
         bool StartNetSend(void* hdr, unsigned hdrsize, const Buffer& buf);

         /** \brief Method should be used to cancel all running I/O operation of the socket.
          * Should be used for instance when worker want to be deleted */
         void CancelIOOperations();
   };

   // ______________________________________________________________________

   /** \brief Socket addon for handling connection events
    *
    * \ingroup dabc_all_classes
    */

   class SocketConnectAddon : public SocketAddon {
      protected:
         WorkerRef fConnRcv;
         std::string fConnId;

      public:
         SocketConnectAddon(int fd) :
            SocketAddon(fd),
            fConnRcv(),
            fConnId()
         {}

         void ObjectCleanup() override
         {
            fConnRcv.Release();
            SocketAddon::ObjectCleanup();
         }

         virtual ~SocketConnectAddon() {}

         /** Set connection handler. By default, worker will handle connection from addon */
         void SetConnHandler(const WorkerRef& rcv, const std::string &connid)
         {
            fConnRcv = rcv;
            fConnId = connid;
         }

         const char *ClassName() const override { return "SocketConnectAddon"; }

   };

   // ________________________________________________________________

   /** \brief Socket addon for handling connection requests on server side
    *
    * \ingroup dabc_all_classes
    *
    * this object establish server socket, which waits for new connection
    * of course, we do not want to block complete thread for such task :-)
    */

   class SocketServerAddon : public SocketConnectAddon {
      protected:
         std::string fServerHostName;
         int  fServerPortNumber{0};
         int  fAcceptErrors{0};
         int  ai_family{0};
         int  ai_socktype{0};
         int  ai_protocol{0};
         socklen_t ai_addrlen{0};
         struct sockaddr_storage ai_addr;

         virtual void OnClientConnected(int fd);

         bool RecreateSocket();

      public:
         SocketServerAddon(int serversocket, const char *hostname = nullptr, int portnum = -1, struct addrinfo *info = nullptr);
         virtual ~SocketServerAddon() {}

         void ProcessEvent(const EventId&) override;

         std::string ServerHostName() { return fServerHostName; }
         int ServerPortNumber() const { return fServerPortNumber; }
         std::string ServerId() { return dabc::format("%s:%d", fServerHostName.c_str(), fServerPortNumber); }

         const char *ClassName() const override { return "SocketServerAddon"; }

   };

   // ______________________________________________________________

   /** \brief Socket addon for handling connection on client side
    *
    * \ingroup dabc_all_classes
    *
    */

   class SocketClientAddon : public SocketConnectAddon {
      public:
         SocketClientAddon(const struct addrinfo* serv_addr, int fd = -1);
         virtual ~SocketClientAddon();

         void SetRetryOpt(int nretry, double tmout = 1.);

         void ProcessEvent(const EventId&) override;

         double ProcessTimeout(double) override;

         const char *ClassName() const override { return "SocketClientAddon"; }

      protected:
         virtual void OnConnectionEstablished(int fd);
         virtual void OnConnectionFailed();

         void OnThreadAssigned() override;

         struct addrinfo fServAddr; // own copy of server address
         int             fRetry{0};
         double          fRetryTmout{0};
   };

   // ______________________________________________________________

   /** \brief Special thread class for handling sockets
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    */

   class SocketThread : public Thread {
      protected:
         enum ESocketEvents {
            evntEnableCheck = evntLastThrd+1,  ///< event to enable again checking sockets for new events
            evntLastSocketThrdEvent            ///< last event, which can be used by socket
         };

         struct ProcRec {
             bool      use;  ///< indicates if processor is used for poll
             uint32_t  indx; ///< index for dereference of processor from ufds structure
         };

         int            fPipe[2] = {0,0};  ///< array with i/o pipes handles
         long           fPipeFired{0};       ///< indicate if something was written in pipe
         bool           fWaitFire{false};        ///< indicates if pipe firing is awaited
         int            fScalerCounter{0};   ///< variable used to test time to time sockets even if there are events in the queue
         unsigned       f_sizeufds{0};       ///< size of the structure, which was allocated
         pollfd        *f_ufds{nullptr};           ///< list of file descriptors for poll call
         ProcRec       *f_recs{nullptr};           ///< identify used processors
         bool           fIsAnySocket{false};     ///< indicates that at least one socket processors in the list
         bool           fCheckNewEvents{false};  ///< flag indicate if sockets should be checked for new events even if there are already events in the queue
         int            fBalanceCnt{0};      ///< counter for balancing of input events

#ifdef SOCKET_PROFILING
         long           fWaitCalls;
         long           fWaitDone;
         long           fPipeCalled;
         double         fWaitTime;
         double         fFillTime;
#endif

         bool WaitEvent(EventId&, double tmout) override;

         void _Fire(const EventId& evnt, int nq) override;

         void WorkersSetChanged() override;

         void ProcessExtraThreadEvent(const EventId& evid) override;

      public:

         // list of all events for all kind of socket processors

         SocketThread(Reference parent, const std::string &name, Command cmd);
         virtual ~SocketThread();

         const char *ClassName() const override { return typeSocketThread; }
         bool CompatibleClass(const std::string &clname) const override;

         static bool SetNonBlockSocket(int fd);
         static bool SetNoDelaySocket(int fd);

         static int StartClient(const std::string &host, int nport, bool nonblocking = true);

         /** \brief Wrapper for send method, should be used for blocking sockets */
         static int SendBuffer(int fd, void* buf, int len);

         /** \brief Wrapper for recv method, should be used for blocking sockets */
         static int RecvBuffer(int fd, void* buf, int len);


         /** \brief Create datagram (udp) socket
          * \returns socket descriptor or negative value in case of failure */
         static int CreateUdp();

         /** \brief Bind UDP socket to specified port
          * \param[in] fd - socket descriptor, created with \ref CreateUdp method
          * \param[in] nport - port number to bind with
          * \param[in] portmin, portmax - values range for possible port numbers
          * \returns port number, which was bind or negative value in case of error */
         static int BindUdp(int fd, int nport, int portmin=-1, int portmax=-1);

         /** \brief Close datagram (udp) socket */
         static void CloseUdp(int fd);

         /** \brief Attach datagram socket to multicast group to make receiving */
         static bool AttachMulticast(int handle, const std::string &addr);

         /** \brief Detach datagram socket from multicast group */
         static void DettachMulticast(int handle, const std::string &addr);

         /** \brief Return current host name. If configured, uses sockethost value from XML file */
         static std::string DefineHostName(bool force = true);

         static int ConnectUdp(int fd, const std::string &remhost, int remport);

         /** \brief Create handle for server-side connection
          * If hostname == 0, any available address will be selected
          * If hostname == "", configured hostname or just $HOST variable will be used
          * If hostname is not empty, only selected host will be tried to bin
          * One could bind such connection to specified port or try to choose from available ports  */
         static SocketServerAddon* CreateServerAddon(const std::string &host, int nport, int portmin=-1, int portmax=-1);

         static SocketClientAddon* CreateClientAddon(const std::string &servid, int dflt_port = -1);
   };
}

#endif
