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

#ifndef HADAQ_UDPTRANSPORT_H
#define HADAQ_UDPTRANSPORT_H

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_BuffersQueue
#include "dabc/BuffersQueue.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif


//#include <list>

#include "hadaq/HadaqTypeDefs.h"

//#define UDP_PAYLOAD_OFFSET 42
#define MAX_UDP_PAYLOAD 1472
//#define MESSAGES_PER_PACKET 243

namespace hadaq {

   // TODO: put here hadtu header structure of packet
   struct UdpDataPacket
      {
         uint32_t pktid;
         uint32_t lastreqid;
         uint32_t nummsg;
      // here all messages should follow
      };

      struct UdpDataPacketFull : public UdpDataPacket
      {
         uint8_t msgs[MAX_UDP_PAYLOAD - sizeof(struct UdpDataPacket)];
      };




   //class UdpDevice;

   // TODO: in DABC2 data socket should be inherited from dabc::SocketWorker
   //       or one should reorganize event loop to inherit from dabc::SocketIOWorker

   class UdpDataSocket : public dabc::SocketWorker,
                         public dabc::Transport,
                         protected dabc::MemoryPoolRequester {
      friend class UdpDevice;

      DABC_TRANSPORT(dabc::SocketWorker)

      protected:
         enum EUdpEvents { evntStartTransport = evntSocketLast + 1,
                           evntStopTransport,
                           evntConfirmCmd,
                           evntFillBuffer };

         enum EDaqState { daqInit,        // initial state
                          daqStarting,    // daq runs, but no any start daq message was seen
                          daqRuns,        // normal working mode
                          daqStopping,    // we did poke(stop_daq) and just waiting that command is performed
                          daqFails        // error state
                        };

         struct ResendInfo {

            uint32_t        pktid;    // id of packet
            dabc::TimeStamp lasttm;   // when request was send last time
            int             numtry;   // how many requests was send
            dabc::Buffer*   buf;      // buffer pointer
            unsigned        bufindx;  // index of buffer in the queue
            char*           ptr;      // target location

            ResendInfo(unsigned id = 0) :
               pktid(id),
               lasttm(),
               numtry(0),
               buf(0),
               bufindx(0),
               ptr(0) {}

            ResendInfo(const ResendInfo& src) :
               pktid(src.pktid),
               lasttm(src.lasttm),
               numtry(src.numtry),
               buf(src.buf),
               bufindx(src.bufindx),
               ptr(src.ptr) {}

         };


         // FIXME: one should not use pointers, only while device and transport are in same thread, one can do this
         //UdpDevice*         fDev;
         dabc::Mutex        fQueueMutex;
         dabc::BuffersQueue fQueue;

         unsigned           fReadyBuffers; // how many buffers in queue can be provided to user
         unsigned           fTransportBuffers; // how many buffers can be used for transport

         dabc::Buffer*      fTgtBuf;   // pointer on buffer, where next portion can be received, use pointer while it is buffer from the queue
         unsigned           fTgtBufIndx; // queue index of target buffer - use fQueue.Item(fReadyBuffers + fTgtBufIndx)
         unsigned           fTgtShift; // current shift in the buffer
         UdpDataPacket      fTgtHdr;   // place where data header should be received
         char*              fTgtPtr;   // location where next data should be received
         uint32_t           fTgtNextId; // expected id of next packet
         uint32_t           fTgtTailId; // first id of packet which can not be dropped on ROC side
         bool               fTgtCheckGap;  // true if one should check gaps

         char               fTempBuf[2000]; // buffer to recv packet when no regular place is available

         dabc::Queue<ResendInfo>  fResend;

         EDaqState          daqState;
         bool               daqCheckStop;

         //UdpDataPacketFull  fRecvBuf;

         unsigned           fBufferSize;
         unsigned           fTransferWindow;


         dabc::MemoryPoolRef fPool;  // reference on the pool, reference should help us to preserve pool as long as we are using it

//         dabc::TimeStamp    lastRequestTm;   // time when last request was send
//         bool               lastRequestSeen; // indicate if we seen already reply on last request
//         uint32_t           lastRequestId;   // last request id
//         uint32_t           lastSendFrontId; // last send id of front packet
//
//         unsigned           rocNumber;

         uint64_t           fTotalRecvPacket;
         uint64_t           fTotalResubmPacket;

         double             fFlushTimeout;  // after such timeout partially filled packed will be delivered
         //dabc::TimeStamp    fLastDelivery;  // time of last delivery

         //base::OperList     fStartList;

         //MessageFormat      fFormat;

         inline bool daqActive() const { return (daqState == daqRuns); }

         virtual bool ReplyCommand(dabc::Command cmd);

         virtual void ProcessPoolChanged(dabc::MemoryPool* pool) {}

         virtual bool ProcessPoolRequest();

         virtual double ProcessTimeout(double last_diff);

         void ConfigureFor(dabc::Port* port);

         void AddBuffersToQueue(bool checkanyway = true);
         bool CheckNextRequest(bool check_retrans = true);

         void AddDataPacket(int len, void* tgt);
         void CompressBuffer(dabc::Buffer& buf);
         void CheckReadyBuffers(bool doflush = false);

         void ResetDaq();

         //bool prepareForSuspend();

         virtual int GetParameter(const char* name);

         void setFlushTimeout(double tmout) { fFlushTimeout = tmout; }
         double getFlushTimeout() const {  return fFlushTimeout; }

      public:
         UdpDataSocket(dabc::Reference port, int fd);
         virtual ~UdpDataSocket();

         virtual void ProcessEvent(const dabc::EventId&);

         virtual bool ProvidesInput() { return true; }
         virtual bool ProvidesOutput() { return false; }

         virtual bool Recv(dabc::Buffer& buf);
         virtual unsigned  RecvQueueSize() const;
         virtual dabc::Buffer& RecvBuffer(unsigned indx) const;
         virtual bool Send(const dabc::Buffer& buf) { return false; }
         virtual unsigned SendQueueSize() { return 0; }
         virtual unsigned MaxSendSegments() { return 0; }

         virtual void StartTransport();
         virtual void StopTransport();
   };

}

#endif
