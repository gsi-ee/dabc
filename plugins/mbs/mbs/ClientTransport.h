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

#ifndef MBS_ClientTransport
#define MBS_ClientTransport

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

   /** \brief Client transport for different kinds of MBS server */

   class ClientTransport : public dabc::SocketIOAddon,
                           public dabc::DataInput {

      protected:

         enum EIOState {
            ioInit,          // initial state
            ioRecvInfo,      // receiving server info
            ioReady,         // ready for work
            ioRecvHeader,    // receiving buffer header
            ioWaitBuffer,
            ioRecvBuffer,
            ioComplBuffer,   // at this stage buffer must be completed
            ioClosing,       // sending message to server that client is closing
            ioError,         // error detected
            ioClosed         // socket was closed by server
         };

         enum EEvents {
            evDataInput = evntSocketLast,
            evReactivate
         };

         mbs::TransportInfo   fServInfo; // data, send by transport server in the beginning
         EIOState             fState{ioInit};
         bool                 fSwapping{false};
         bool                 fSpanning{false};  //!< when true, MBS could deliver spanned events

         mbs::BufferHeader    fHeader;
         char                 fSendBuf[12];

         int                  fKind{0}; // values from EMbsServerKinds

         bool                 fPendingStart{false};

         dabc::Buffer         fSpanBuffer;  //!< buffer rest, which should be copied and merged into next buffer


         // this is part from SocketAddon

         void ObjectCleanup() override;

         void OnThreadAssigned() override;

         double ProcessTimeout(double last_diff) override;

         void ProcessEvent(const dabc::EventId&) override;

         void OnSendCompleted() override;
         void OnRecvCompleted() override;

         void OnSocketError(int err, const std::string &info) override;

         void SubmitRequest();
         void MakeCallback(unsigned sz);

         unsigned ReadBufferSize();
         bool IsDabcEnabledOnMbsSide(); // indicates if new format is enabled on mbs side

         dabc::WorkerAddon* Read_GetAddon() override { return this; }

      public:

         ClientTransport(int fd, int kind);
         virtual ~ClientTransport();

         int Kind() const { return fKind; }

         // this is interface from DataInput

         unsigned Read_Size() override;
         unsigned Read_Start(dabc::Buffer& buf) override;
         unsigned Read_Complete(dabc::Buffer& buf) override;
         double Read_Timeout() override { return 0.1; }

   };

}

#endif

