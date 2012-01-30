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

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef DABC_BuffersQueue
#include "dabc/BuffersQueue.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#ifndef MBS_MbsTypeDefs
#include "mbs/MbsTypeDefs.h"
#endif

namespace mbs {

   // TODO: implement following syntax for connection to mbs servers
   //   mbs://r4-3/transport, mbs://r4-3/stream, mbs://r4-3/eventserver:6009

   class ClientTransport : public dabc::SocketIOWorker,
                           public dabc::Transport,
                           protected dabc::MemoryPoolRequester {

      DABC_TRANSPORT(dabc::SocketIOWorker)

      protected:

         enum EIOState { ioInit, ioRecvInfo, ioReady, ioRecvHeder, ioWaitBuffer, ioRecvBuffer, ioClosing,  ioError };

         mbs::TransportInfo   fServInfo; // data, send by transport server in the beginning
         EIOState             fState;
         bool                 fSwapping;
         mbs::BufferHeader    fHeader;
         char                 fSendBuf[12];
         dabc::Buffer         fRecvBuffer;

         int                  fKind; // values from EMbsServerKinds

         dabc::Mutex          fMutex;
         dabc::BuffersQueue   fInpQueue;
         bool                 fRunning;

         virtual void CleanupTransport();

         virtual void CleanupFromTransport(Object* obj);

         virtual bool ProcessPoolRequest();

         bool HasPlaceInQueue();
         bool RequestBuffer(uint32_t sz, dabc::Buffer &buf);

         unsigned ReadBufferSize();
         bool IsDabcEnabledOnMbsSide(); // indicates if new format is enabled on mbs side

      public:

         enum EEvents { evDataInput = evntSocketLast,
                        evRecvInfo,
                        evReactivate,
                        evSendClose };

         ClientTransport(dabc::Reference port, int kind, int fd);
         virtual ~ClientTransport();

         virtual void ProcessEvent(const dabc::EventId&);

         virtual void OnSendCompleted();
         virtual void OnRecvCompleted();

         virtual void OnConnectionClosed();

         int Kind() const { return fKind; }

         virtual bool ProvidesInput() { return true; }
         virtual void PortAssigned();

         virtual void StartTransport();
         virtual void StopTransport();

         virtual bool Recv(dabc::Buffer&);
         virtual unsigned RecvQueueSize() const;
         virtual dabc::Buffer& RecvBuffer(unsigned) const;
         virtual bool Send(const dabc::Buffer&) { return false; }
         virtual unsigned SendQueueSize() { return 0; }
         virtual unsigned MaxSendSegments() { return 0; }

   };

}

#endif

