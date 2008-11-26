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

   class Device;
   class ServerTransport;

   class ServerConnectProcessor : public dabc::SocketServerProcessor {
      public:
         ServerConnectProcessor(ServerTransport* tr, int serversocket, int portnum);

      protected:
         virtual void OnClientConnected(int fd);

         ServerTransport*  fTransport;
   };

   // _____________________________________________________________________

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
         virtual void ProcessEvent(dabc::EventId);

      protected:
         virtual double ProcessTimeout(double last_diff);


         ServerTransport*      fTransport;
         mbs::TransportInfo    fServInfo; // data, send by transport server in the beginning
         EIOState              fState;
         char                  f_sbuf[12]; // input buffer to get request
         dabc::Buffer*         fSendBuf;
         mbs::BufferHeader     fHeader;
   };

   // _________________________________________________________________

   class ServerTransport : public dabc::Transport {

      public:
         ServerTransport(Device* dev, dabc::Port* port,
                         int kind,
                         int serversocket,
                         int portnum,
                         uint32_t maxbufsize = 16*1024);
         virtual ~ServerTransport();

         int Kind() const { return fKind; }

         // here is call-backs from different processors
         void ProcessConnectionRequest(int fd);
         void SocketIOClosed(ServerIOProcessor* proc);
         dabc::Buffer* TakeFrontBuffer();
         void DropFrontBufferIfQueueFull();

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
         int                     fKind; // see EMbsServerKinds values
         dabc::Mutex             fMutex;
         dabc::BuffersQueue      fOutQueue;
         ServerConnectProcessor* fServerPort; // socket for connections handling
         ServerIOProcessor*      fIOSocket; // actual socket for I/O operation
         uint32_t                fMaxBufferSize; // maximum size of the buffer, which can be send over channel, used for old transports
         long                    fSendBuffers;
         long                    fDroppedBuffers;
   };

}

#endif
