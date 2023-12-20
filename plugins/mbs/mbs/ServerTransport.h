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

#ifndef DABC_eventsapi
#include "dabc/eventsapi.h"
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
            oSendingEvents,    // sending events
            oSendingLastEvent, // waiting for completion of last event
            oSendingBuffer,
            oDoingClose,
            oError
         };

         OutputState           fState{oInit};

         int                   fKind{0}; // see EMbsServerKinds values
         mbs::TransportInfo    fServInfo; // data, send by transport server in the beginning
         char                  f_sbuf[12]; // input buffer to get request
         mbs::BufferHeader     fHeader;
         long                  fSendBuffers{0};
         long                  fDroppedBuffers{0};
         dabc::EventsIteratorRef fIter;
         mbs::EventHeader      fEvHdr;  // additional MBS event header (for non-MBS events)
         mbs::SubeventHeader   fSubHdr; // additional MBS subevent header (for non-MBS events)
         uint32_t              fEvCounter{0}; // special events counter
         uint32_t              fSubevId{0};  // full id of subevent
         bool                  fHasExtraRequest{false};

         // from addon
         void OnThreadAssigned() override;
         void OnSendCompleted() override;
         void OnRecvCompleted() override;

         void MakeCallback(unsigned sz);

         double ProcessTimeout(double last_diff) override;

         void OnSocketError(int err, const std::string &info) override;

         // IMPORTANT: do not provide addon here, set it directly in server
         dabc::WorkerAddon* Write_GetAddon()  override { return nullptr; }

      public:
         ServerOutputAddon(int fd, int kind, dabc::EventsIteratorRef& iter, uint32_t subid);
         virtual ~ServerOutputAddon();

         void FillServInfo(int32_t maxbytes, bool isnewformat);
         void SetServerKind(int kind) { fKind = kind; }

         // code from the DataOutput
         unsigned Write_Check() override;
         unsigned Write_Buffer(dabc::Buffer& buf) override;
         double Write_Timeout() override { return 0.1; }
   };


   // ===============================================================================

   /** \brief Server transport for different kinds of MBS server */

   class ServerTransport : public dabc::Transport {
      protected:

         int fKind{0};             ///< kind: stream or transport
         int fPortNum{0};          ///< used port number (only for info)
         int fSlaveQueueLength{0}; ///< queue length, used for slaves connections
         int fClientsLimit{0};     ///< maximum number of simultaneous clients
         int fDoingClose{0};       ///< 0 - normal, 1 - saw EOF, 2 - all clients are gone
         bool fBlocking{false};    ///< if true, server will block buffers until it can be delivered
         bool fDeliverAll{false};  ///< if true, server will try deliver all events when clients are there (default for transport)
         std::string fIterKind;    ///< iterator kind when non-mbs events should be delivered to clients
         uint32_t fSubevId{0};     ///< subevent id when non-MBS events are used
         unsigned fBufSize{0};    ///< maximal buffer size

         bool StartTransport() override;
         bool StopTransport() override;

         int ExecuteCommand(dabc::Command cmd) override;

         bool ProcessRecv(unsigned) override { return SendNextBuffer(); }
         bool ProcessSend(unsigned) override { return SendNextBuffer(); }
         bool ProcessBuffer(unsigned) override { return SendNextBuffer(); }

         bool SendNextBuffer();

         void ProcessConnectionActivated(const std::string &name, bool on) override;

      public:

         ServerTransport(dabc::Command cmd, const dabc::PortRef &outport,
                         int kind, int portnum,
                         dabc::SocketServerAddon *connaddon,
                         const dabc::Url& url);
         virtual ~ServerTransport();

   };
}

#endif
