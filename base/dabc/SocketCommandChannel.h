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

         /** \brief As name said, command channel requires socket thread for the work */
         virtual std::string RequiredThrdClass() const { return typeSocketThread; }
   };


}

#endif
