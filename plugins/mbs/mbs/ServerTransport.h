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

#ifndef MBS_ServerTransport
#define MBS_ServerTransport

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef MBS_MbsTypeDefs
#include "mbs/MbsTypeDefs.h"
#endif


namespace mbs {

   /** \brief %Addon for output of server-side different kinds of MBS server */

   class ServerOutputAddon : public dabc::SocketIOAddon,
                             public dabc::DataOutput {
      protected:

         enum OutputState {
            oInit,             // when object created
            oInitReq,          // get request before init block was send
            oWaitingReq,       // waiting request from client
            oWaitingReqBack,   // waiting request from client and need to make call-back
            oWaitingBuffer,
            oSendingEvents,
            oSendingBuffer,
            oDoingClose,
            oError
         };

         OutputState           fState;

         int                   fKind; // see EMbsServerKinds values
         mbs::TransportInfo    fServInfo; // data, send by transport server in the beginning
         char                  f_sbuf[12]; // input buffer to get request
         mbs::BufferHeader     fHeader;
         long                  fSendBuffers;
         long                  fDroppedBuffers;
         dabc::Reference       fIter;
         mbs::EventHeader      fEvHdr;  // additional MBS event header (for non-MBS events)
         mbs::SubeventHeader   fSubHdr; // additional MBS subevent header (for non-MBS events)
         uint32_t              fEvCounter; // special events counter
         uint32_t              fSubevId;  // full id of subevent

         // from addon
         virtual void OnThreadAssigned();
         virtual void OnSendCompleted();
         virtual void OnRecvCompleted();

         void MakeCallback(unsigned sz);

         virtual double ProcessTimeout(double last_diff);

         virtual void OnConnectionClosed();
         virtual void OnSocketError(int errnum, const std::string& info);

      public:
         ServerOutputAddon(int fd, int kind, dabc::Reference& iter, uint32_t subid);
         virtual ~ServerOutputAddon();

         void FillServInfo(int32_t maxbytes, bool isnewformat);
         void SetServerKind(int kind) { fKind = kind; }

         // code from the DataOutput
         virtual unsigned Write_Check();
         virtual unsigned Write_Buffer(dabc::Buffer& buf);
         virtual double Write_Timeout() { return 0.1; }
   };


   // ===============================================================================

   /** \brief Server transport for different kinds of MBS server */

   class ServerTransport : public dabc::Transport {
      protected:

         int fKind;             ///< kind of transport, stream or transport
         int fSlaveQueueLength; ///< queue length, used for slaves connections
         int fClientsLimit;     ///< maximum number of simultaneous clients
         int fDoingClose;       ///< 0 - normal, 1 - saw EOF, 2 - all clients are gone
         bool fBlocking;        ///< if true, server will block buffers until it can be delivered
         bool fDeliverAll;      ///< if true, server will try deliver all events when clients are there (default for transport)
         std::string fIterKind; ///< iterator kind when non-mbs events should be delivered to clients
         uint32_t fSubevId;     ///< subevent id when non-MBS events are used

         virtual bool StartTransport();
         virtual bool StopTransport();

         virtual int ExecuteCommand(dabc::Command cmd);

         virtual bool ProcessRecv(unsigned port) { return SendNextBuffer(); }
         virtual bool ProcessSend(unsigned port) { return SendNextBuffer(); }

         bool SendNextBuffer();

         void ProcessConnectionActivated(const std::string& name, bool on);

      public:

         ServerTransport(dabc::Command cmd, const dabc::PortRef& outport,
                         int kind, int portnum,
                         dabc::SocketServerAddon* connaddon,
                         const dabc::Url& url);
         virtual ~ServerTransport();

   };
}

#endif
