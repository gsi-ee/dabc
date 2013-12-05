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
            ioRecvHeader,     // receiving buffer header
            ioWaitBuffer,
            ioRecvBuffer,
            ioComplBuffer,   // at this stage buffer must be completed
            ioClosing,
            ioError
         };

         enum EEvents {
            evDataInput = evntSocketLast,
            evReactivate
         };

         mbs::TransportInfo   fServInfo; // data, send by transport server in the beginning
         EIOState             fState;
         bool                 fSwapping;
         bool                 fSpanning;  //!< when true, MBS could deliver spanned events

         mbs::BufferHeader    fHeader;
         char                 fSendBuf[12];

         int                  fKind; // values from EMbsServerKinds

         bool                 fPendingStart;

         dabc::Buffer         fSpanBuffer;  //!< buffer rest, which should be copied and merged into next buffer


         // this is part from SocketAddon

         virtual void ObjectCleanup();

         virtual void OnThreadAssigned();

         virtual double ProcessTimeout(double last_diff);

         virtual void ProcessEvent(const dabc::EventId&);

         virtual void OnSendCompleted();
         virtual void OnRecvCompleted();

         virtual void OnConnectionClosed();
         virtual void OnSocketError(int errnum, const std::string& info);

         void SubmitRequest();
         void MakeCallback(unsigned sz);


         unsigned ReadBufferSize();
         bool IsDabcEnabledOnMbsSide(); // indicates if new format is enabled on mbs side

      public:

         ClientTransport(int fd, int kind);
         virtual ~ClientTransport();

         int Kind() const { return fKind; }


         // this is interface from DataInput

         virtual unsigned Read_Size();
         virtual unsigned Read_Start(dabc::Buffer& buf);
         virtual unsigned Read_Complete(dabc::Buffer& buf);
         virtual double Read_Timeout() { return 0.1; }

   };

}

#endif

