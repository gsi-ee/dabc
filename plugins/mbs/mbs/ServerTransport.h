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
#ifndef MBS_ServerTransport
#define MBS_ServerTransport

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

#ifndef MBS_MbsTypeDefs
#include "mbs/MbsTypeDefs.h"
#endif


namespace mbs {

   class ServerTransport;

   class ServerIOProcessor : public dabc::SocketIOProcessor {

      friend class ServerTransport;

      enum EEvents { evMbsDataOutput = evntSocketLast };
      enum EIOState { ioInit, ioReady, ioWaitingReq, ioWaitingBuffer, ioSendingBuffer, ioDoingClose };

      public:
         ServerIOProcessor(ServerTransport* tr, int fd);
         virtual ~ServerIOProcessor();

         void SendInfo(int32_t maxbytes, bool ifnewformat);

         virtual void OnSendCompleted();
         virtual void OnRecvCompleted();

         virtual void OnConnectionClosed();
         virtual void OnSocketError(int errnum, const char* info);

         inline void FireDataOutput() { FireEvent(evMbsDataOutput); }

      protected:
         virtual double ProcessTimeout(double last_diff);
         virtual void ProcessEvent(dabc::EventId);

         ServerTransport*      fTransport;
         mbs::TransportInfo    fServInfo; // data, send by transport server in the beginning
         EIOState              fState;
         char                  f_sbuf[12]; // input buffer to get request
         mbs::BufferHeader     fHeader;
         dabc::BuffersQueue    fSendQueue; // small (size=2) intermediate queue to get buffers from transport
         long                  fSendBuffers;
         long                  fDroppedBuffers;
   };

   // _________________________________________________________________

   class ServerTransport : public dabc::Transport,
                           public dabc::SocketServerProcessor {

      enum EEvents { evntNewBuffer = evntSocketLast,
                     evntMbsServerLast };

      public:
         ServerTransport(dabc::Port* port, const std::string& thrdname,
                         int kind, int serversocket, int portnum,
                         uint32_t maxbufsize = 16*1024, int scale = 1);
         virtual ~ServerTransport();

         int Kind() const { return fKind; }

         // here is call-backs from different processors
         void SocketIOClosed(ServerIOProcessor* proc);
         void MoveFrontBuffer(ServerIOProcessor* callproc);

         // this is normal transport functionality
         virtual bool ProvidesInput() { return false; }
         virtual bool ProvidesOutput() { return true; }
         virtual void PortChanged();

         virtual bool Recv(dabc::Buffer* &buf) { return false; }
         virtual unsigned RecvQueueSize() const { return 0; }
         virtual dabc::Buffer* RecvBuffer(unsigned) const { return 0; }
         virtual bool Send(dabc::Buffer* mem);
         virtual unsigned SendQueueSize();
         virtual unsigned MaxSendSegments() { return 9999; }
      protected:

         virtual void HaltTransport();

         virtual void OnClientConnected(int fd);

         virtual void ProcessEvent(dabc::EventId);

         int                     fKind; // see EMbsServerKinds values
         dabc::Mutex             fMutex;
         dabc::BuffersQueue      fOutQueue;
         std::vector<ServerIOProcessor*> fIOSockets; // all connected I/O sockets
         uint32_t                fMaxBufferSize; // maximum size of the buffer, which can be send over channel, used for old transports
         int                     fScale;        // scale factor for stream server
         int                     fScaleCounter; // counter for scaler
   };

}

#endif
