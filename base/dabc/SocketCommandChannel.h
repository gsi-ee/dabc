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

#ifndef DABC_SocketCommandChannel
#define DABC_SocketCommandChannel

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef DABC_Queue
#include "dabc/Queue.h"
#endif


namespace dabc {

   class SocketCmdAddr;
   class SocketCmdRec;

   class SocketCmdRecList;

   /*! \brief Defines syntax of raw packet, transformed on the command channels
    *
    */

   struct SocketCmdPacket {
      /*! constant, which identifies dabc packet  */
      uint64_t dabc_header;

      /*! constant, which identifies specific application  */
      uint64_t app_header;

      /*! sequence number of source node */
      uint32_t source_id;

      /*! sequence number of target node */
      uint32_t target_id;

      /*! unique id of the request */
      uint32_t rec_id;

      /*! kind of the information in the packet */
      uint32_t data_kind;

      /*! length of the information in the packet */
      uint32_t data_len;

      /*! data here are depends from the kind specified */

   };

   // _________________________________________________________________________________

   enum { MAX_UDP_PAYLOAD = 1472 };
   enum { MAX_CMD_PAYLOAD = MAX_UDP_PAYLOAD - sizeof(SocketCmdPacket) };

   /** \brief %Addon implementing socket I/O for \ref SocketCommandChannel
    *
    * \ingroup dabc_all_classes
    */

   class SocketCommandAddon : public SocketAddon {
      protected:

         int  fPort;

         virtual void ProcessEvent(const EventId&);

         virtual void OnConnectionClosed() {}
         virtual void OnSocketError(int errnum, const std::string& info) {}

      public:
         SocketCommandAddon(int fd, int port);
         virtual ~SocketCommandAddon();

         bool SendData(void* hdr, unsigned hdrsize, void* data, unsigned datasize, void* addr, unsigned addrsize);
   };


   // __________________________________________________________________________________

   /** \brief Provides command channel to the dabc process
    *
    * \ingroup dabc_all_classes
    */

   class SocketCommandChannel : public Worker {

      friend class SocketCommandAddon;

      protected:

         uint64_t        fAppId;         ///< this should be unique id of the DABC application to let run several DABC on the node

         bool            fAcceptRequests; ///< indicates if any kind of requests will be accepted

         SocketCmdRecList* fRecs;        ///< these are active records which are now in processing
         SocketCmdRecList* fPending;     ///< these records are pending - no address resolved
         SocketCmdRecList* fProcessed;   ///< these records are processed - they are remain just certain time after that one can send back reply again

         CommandsQueue   fConnCmds;      ///< pending connection commands, normally submitted by manager

         uint32_t        fRecIdCounter;  ///< counter for records id to assign unique ids for subsequent records

         PointersVector  fAddrs;         ///< array with resolved addresses of other nodes

         int             fNodeId;        ///<  current node id

         bool            fCanSendDirect; ///<  variable indicates if one can send packet immediately

         double          fCleanupTm;     ///<  time left from the last cleanup

         long            fSendPackets;   ///< total number of send packets
         long            fRetryPackets;  ///< total number of send retries

         /*! \brief If available, sends next portion of data over socket */
         bool DoSendData(double* diff = 0);

         /*! \brief Find address for specified node */
         SocketCmdAddr* FindAddr(int nodeid);

         /*! \brief Close (destroys) address for specified node (if exists) */
         void CloseAddr(int nodeid);

         /** \brief Check if node address resolved and command can be submitted */
         int TryAddressResolve(int nodeid);

         /** \brief Set/update address of remote node */
         void RefreshAddr(int nodeid, void* addr, int addrlen);

         /** \brief Proof address of remote node */
         bool CheckAddr(int nodeid, void* addr, int addrlen);

         /*! \brief Remove record from main list, if specified - keep it for some time in postpone list */
         void CloseRec(SocketCmdRec* rec, double pospone_tm = -1.);

         void ErrorCloseRec(SocketCmdRec* rec);

         /*! \brief Analyze if there is data to send */
         double CheckSocketCanSend(bool activate_timeout = true);

         /*! \brief Checks if required by command state is achieved */
         bool CheckConnCommandStatus(Command cmd);

         /*! \brief Check status of all connection commands
          * When new command specified, all other commands should be replied and new will be add */
         void CheckAllConnCommands(Command newcmd = 0);

         /*! \brief Call of this method notifies application that node state is changed */
         void ProduceNodesStateNotification(int nodeid);

         virtual int PreviewCommand(Command cmd);
         virtual int ExecuteCommand(Command cmd);
         virtual bool ReplyCommand(Command cmd);

         virtual void OnThreadAssigned();
         virtual double ProcessTimeout(double);

      public:

         SocketCommandChannel(const std::string& name, SocketCommandAddon* addon);
         virtual ~SocketCommandChannel();

         /*! \brief Analyze data received from the socket */
         void ProcessRecvData(SocketCmdPacket* hdr, void* data, int len, void* addr, int addrlen);

         static SocketCommandChannel* CreateChannel(const std::string& objname = "");

   };

   // ____________________________________________________________________________________


   /*! \brief Defines syntax of raw packet, transformed on the command channels
    *
    */

   struct SocketCmdPacketNew {

      uint32_t dabc_header;   ///< constant, which identifies dabc packet

      uint32_t data_kind;     ///< kind of data inside packet

      uint32_t data_timeout;  ///< timeout in milliseconds, 0 - not timeout at all

      uint32_t data_size;     ///< length of the information in the packet


      /** data here are depends from the kind specified */
   };



   // ____________________________________________________________________________________

   /** \brief Client side of command connection between two nodes
    *
    * \ingroup dabc_all_classes
    */

   class SocketCommandClient : public Worker {
      protected:

         enum EEVent {
            evntActivate = SocketAddon::evntSocketLastInfo + 1
         };

         enum {
            headerDabc = 123707321
         };

         enum ECmdDataKindNew {
            kindCommand    = 1,    ///< command                   (client -> server)
            kindReply      = 2,    ///< reply                     (server -> client)
            kindCancel     = 3,    ///< cancel cmd execution      (server -> client)
            kindDisconnect = 4     ///< close of connection       (client -> server)
         };


         enum EState {
            stConnecting,  // initial state, worker waits until channel is connected
            stWaitCmd,     // waiting for any command (client - via append, server - via socket)
            stSending,     // sending data (client or server)
            stWaitReply,   // client waiting reply from server
            stExecuting,   // server waits until command is executed
            stReceiving,   // receiving data (client or server)
            stClosing,     // socket connection going normally down
            stFailure      // failure is detected and connection will be close
         };


         bool         fIsClient;                 ///< true - client side, false - server side

         int          fRemoteNodeId;             ///<  remote node id (if available)
         std::string  fRemoteHostName;           ///<  host name and port number
         bool         fConnected;                ///<  true when connection is established

         CommandsQueue fCmds;                    ///< commands to be submitted to the server

         EState        fState;                   ///< current state of the worker

         SocketCmdPacketNew fSendHdr;            ///< header for send command
         std::string        fSendBuf;            ///< content of transported command
         SocketCmdPacketNew fRecvHdr;            ///< buffer for receiving header
         char*              fRecvBuf;            ///< raw buffer for receiving command
         unsigned           fRecvBufSize;        ///< currently allocated size of recv buffer

         dabc::Command      fCurrentCmd;         ///< currently executed command

         virtual int ExecuteCommand(Command cmd);

         virtual bool ReplyCommand(Command cmd);

         virtual void ProcessEvent(const EventId&);

         virtual double ProcessTimeout(double last_diff);

         bool EnsureRecvBuffer(unsigned strsize);

         /** \brief Method called, when complete packet (header + raw data) is received */
         void ProcessRecvPacket();

         /** \brief Sends content of current command via socket */
         void SendCurrentCommand(double send_tmout = 0.);

         /** \brief Called when connection must be closed due to the error */
         void CloseClient(bool iserr = false, const char* msg = 0);

         double ExtraClientTimeout() const { return 1.0; }
         double ExtraServerTimeout() const { return 0.1; }

      public:
         SocketCommandClient(Reference parent, const std::string& name, SocketAddon* addon,
                            int remnode = -1, const std::string& hostname = "");
         virtual ~SocketCommandClient();

         void AppendCmd(Command cmd);

         bool IsClient() const { return fIsClient; }
         bool IsServer() const { return !fIsClient; }

   };

   class SocketCommandClientRef : public WorkerRef {

      DABC_REFERENCE(SocketCommandClientRef, WorkerRef, SocketCommandClient)
   };

   // ____________________________________________________________________________________


   /** \brief Provides command channel to the dabc process
    *
    * \ingroup dabc_all_classes
    */

   class SocketCommandChannelNew : public Worker {
      protected:
         int             fNodeId;        ///<  current node id
         int             fClientCnt;     ///<  counter for new clients

         virtual int PreviewCommand(Command cmd);
         virtual int ExecuteCommand(Command cmd);

         /** \brief Provides name of worker, which should handle client side of the connection */
         std::string ClientWorkerName(int remnode, const std::string& remnodename);

      public:
         SocketCommandChannelNew(const std::string& name, SocketServerAddon* connaddon);
         virtual ~SocketCommandChannelNew();

         /** \brief As name said, command channel rerquires socket thread for the work */
         virtual std::string RequiredThrdClass() const { return typeSocketThread; }

         static SocketCommandChannelNew* CreateChannel(const std::string& objname = "");

   };



}

#endif
