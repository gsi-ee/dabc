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

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif


namespace dabc {


   /*! \brief Defines syntax of raw packet, transformed on the command channels
    *
    */

   struct SocketCmdPacket {

      uint32_t dabc_header;   ///< constant, which identifies dabc packet

      uint32_t data_kind;     ///< kind of data inside packet

      uint32_t data_timeout;  ///< timeout in milliseconds, 0 - not timeout at all

      uint32_t data_size;     ///< total length of the information in the packet

      uint32_t data_cmdsize;  ///< which part of data is for command in xml format

      uint32_t data_rawsize;  ///< which part of data at the end is raw data

      /** data here are depends from the kind specified */
   };


   class SocketCommandChannel;

   // ____________________________________________________________________________________

   /** \brief Client side of command connection between two nodes
    *
    * \ingroup dabc_all_classes
    */

   class SocketCommandClient : public Worker {
      protected:

         friend class SocketCommandChannel;

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
            stConnecting,  ///< initial state, worker waits until channel is connected
            stWorking,     ///< normal state when execution of different commands are done
            stClosing,     ///< socket connection going normally down
            stFailure      ///< failure is detected and connection will be close
         };

         enum ERecvState {
            recvInit,     // receiver  ready to accept new command
            recvHeader,   // command is assigned and executing
            recvData      // command is sends back, one need to release command only when it is replied
         };

         std::string        fRemoteHostName;    ///<  host name and port number
         double             fReconnectPeriod;   ///<  interval how often reconnect will be tried

         CommandsQueue      fCmds;              ///< commands to be submitted to the remote

         EState             fState;             ///< current state of the worker

         SocketCmdPacket    fSendHdr;           ///< header for send command
         std::string        fSendBuf;           ///< content of transported command
         SocketCmdPacket    fRecvHdr;           ///< buffer for receiving header
         char*              fRecvBuf;           ///< raw buffer for receiving command
         unsigned           fRecvBufSize;       ///< currently allocated size of recv buffer

         dabc::Command      fMainCmd;           ///< command which was send to other side for execution
         dabc::Command      fExeCmd;            ///< command which was received from remote for execution
         dabc::BinData      fLastBinData;       ///< keep last binary data of the command to allow transfer of data

         CommandsQueue      fSendQueue;         ///< queue to keep commands needed for sending

         ERecvState         fRecvState;         ///< state that happens with receiver (server)

         // these are fields, used to manage information about remote node
         bool               fRemoteObserver;   ///< if true, channel automatically used to update information from remote
         TimeStamp          fRemReqTime;       ///< last time when request was send to remote
         Hierarchy          fRemoteHierarchy;  ///< actual remote hierarchy
         std::string        fRemoteName;       ///< name of connection, appeared in the browser

         // indicate that this is special connection to master node
         // each time it is reconnected, it will append special command to register itself in remote master
         bool               fMasterConn;
         std::string        fClientNameSufix;  ///< name suffix, which will be append to name seen on the server side

         virtual void OnThreadAssigned();

         virtual int ExecuteCommand(Command cmd);

         virtual bool ReplyCommand(Command cmd);

         virtual void ProcessEvent(const EventId&);

         virtual double ProcessTimeout(double last_diff);

         bool EnsureRecvBuffer(unsigned strsize);

         /** \brief Method called, when complete packet (header + raw data) is received */
         void ProcessRecvPacket();

         /** \brief Submit command to send queue, if queue is empty start sending immediately */
         void SubmitCommandSend(dabc::Command cmd, bool asreply, double send_tmout = 0.);

         /** Send next command to the remote */
         void PerformCommandSend();

         /** \brief Called when connection must be closed due to the error */
         void CloseClient(bool iserr = false, const char* msg = 0);

         double ExtraClientTimeout() const { return 1.0; }
         double ExtraServerTimeout() const { return 0.1; }

         bool ExecuteCommandByItself(Command cmd);

      public:
         SocketCommandClient(Reference parent, const std::string& name,
                             SocketAddon* addon,
                             const std::string& hostname = "",
                             double reconnect = 0.);
         virtual ~SocketCommandClient();

         void AppendCmd(Command cmd);
   };

   class SocketCommandClientRef : public WorkerRef {

      DABC_REFERENCE(SocketCommandClientRef, WorkerRef, SocketCommandClient)
   };

   // ____________________________________________________________________________________


   /** \brief Provides command channel to the dabc process
    *
    * \ingroup dabc_all_classes
    */

   class SocketCommandChannel : public Worker {

      friend class SocketCommandClient;

      protected:
         int             fNodeId;        ///<  current node id
         int             fClientCnt;     ///<  counter for new clients
         Hierarchy       fHierarchy;     ///<  current hierarchy as seen by the channel
         bool            fUpdateHierarchy; ///< if true, channel will update internal hierarchy regularly
         std::string     fSelPath;       ///<  selected path, shown in hierarchy

         virtual int PreviewCommand(Command cmd);
         virtual int ExecuteCommand(Command cmd);

         /** \brief Provide string for connection to remote node */
         std::string GetRemoteNode(const std::string& url, int* nodeid = 0);

         /** \brief Provides name of worker, which should handle client side of the connection */
         std::string ClientWorkerName(const std::string& remnodename);

         SocketCommandClientRef ProvideWorker(const std::string& remnodename, double conn_tmout);

         virtual void OnThreadAssigned();

         /** timeout used in channel to update node hierarchy, which than can be requested from remote */
         virtual double ProcessTimeout(double last_diff);

      public:
         SocketCommandChannel(const std::string& name, SocketServerAddon* connaddon, Command cmd);
         virtual ~SocketCommandChannel();

         /** \brief As name said, command channel requires socket thread for the work */
         virtual std::string RequiredThrdClass() const { return typeSocketThread; }

         virtual void BuildHierarchy(HierarchyContainer* cont);
   };


}

#endif
