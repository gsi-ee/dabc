#ifndef MBS_ClientTransport
#define MBS_ClientTransport

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#ifndef MBS_MbsTypeDefs
#include "mbs/MbsTypeDefs.h"
#endif

namespace mbs {

   class Device;
   class ClientTransport;

   class ClientIOProcessor : public dabc::SocketIOProcessor {

      friend class ClientTransport;

      enum EIOState { ioInit, ioRecvInfo, ioReady, ioRecvHeder, ioWaitBuffer, ioRecvBuffer, ioClosing,  ioError };

      public:
         enum EEvents { evDataInput = evntSocketLast + 1, evRecvInfo, evReactivate, evSendClose };

         ClientIOProcessor(ClientTransport* cl, int fd);
         virtual ~ClientIOProcessor();

         virtual void ProcessEvent(dabc::EventId);

         virtual void OnSendCompleted();
         virtual void OnRecvCompleted();

         virtual void OnConnectionClosed();
      protected:

         unsigned             ReadBufferSize();

         ClientTransport     *fTransport;
         mbs::TransportInfo   fServInfo; // data, send by transport server in the beginning
         EIOState             fState;
         bool                 fSwapping;
         mbs::BufferHeader    fHeader;
         char                 fSendBuf[12];
         dabc::Buffer        *fRecvBuffer;
   };


   class ClientTransport : public dabc::Transport,
                           protected dabc::MemoryPoolRequester {

      friend class ClientIOProcessor;

      public:
         ClientTransport(Device* dev, dabc::Port* port, int kind);
         virtual ~ClientTransport();

         int Kind() const { return fKind; }
         bool SetSocket(int fd);
         bool OpenSocket(const char* hostname, int port);

         virtual bool ProvidesInput() { return true; }
         virtual void PortChanged();

         virtual void StartTransport();
         virtual void StopTransport();

         virtual bool Recv(dabc::Buffer* &buf);
         virtual unsigned RecvQueueSize() const;
         virtual dabc::Buffer* RecvBuffer(unsigned) const;
         virtual bool Send(dabc::Buffer* mem) { return false; }
         virtual unsigned SendQueueSize() { return 0; }
         virtual unsigned MaxSendSegments() { return 0; }

      protected:

         virtual bool ProcessPoolRequest();

         bool HasPlaceInQueue();
         bool RequestBuffer(uint32_t sz, dabc::Buffer* &buf);
         void GetReadyBuffer(dabc::Buffer* buf);
         void SocketClosed(ClientIOProcessor* proc);

         int                  fKind; // values from EMbsServerKinds
         ClientIOProcessor   *fIOProcessor;

         dabc::Mutex          fMutex;
         dabc::BuffersQueue   fInpQueue;
         bool                 fRunning;
   };

}






#endif

