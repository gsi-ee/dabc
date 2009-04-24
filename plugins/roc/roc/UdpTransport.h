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

#ifndef ROC_UdpTransport
#define ROC_UdpTransport

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#ifndef DABC_CommandClient
#include "dabc/CommandClient.h"
#endif

#ifndef ROC_UdpBoard
#include "roc/UdpBoard.h"
#endif

#include <list>

namespace roc {

   class UdpDevice;

   class UdpDataSocket : public dabc::SocketIOProcessor,
                         public dabc::Transport,
                         protected dabc::MemoryPoolRequester,
                         protected dabc::CommandClientBase {
      friend class UdpDevice;
      protected:
         enum EUdpEvents { evntStartDaq = evntSocketLast + 1,
                           evntStopDaq,
                           evntConfirmCmd,
                           evntFillBuffer };

         enum EDaqState { daqInit, daqStarting, daqRuns, daqStopping, daqStopped, daqFails };


         struct ResendInfo {

            uint32_t pktid;    // id of packet
            double   lasttm;   // when request was send last time
            int      numtry;   // how many requests was send
            dabc::Buffer* buf; // buffer pointer
            unsigned bufindx;  // index of buffer in the queue
            char*         ptr; // target location

            ResendInfo(unsigned id = 0) :
               pktid(id),
               lasttm(0.),
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


         UdpDevice*         fDev;
         dabc::Mutex        fQueueMutex;
         dabc::BuffersQueue fQueue;

         unsigned           fReadyBuffers; // how many buffers in queue can be provided to user
         unsigned           fTransportBuffers; // how many buffers can be used for transport

         dabc::Buffer*      fTgtBuf;   // pointer on buffer, where next portion can be received
         unsigned           fTgtBufIndx; // queue index of target buffer - use fQueue.Item(fReadyBuffers + fTgtBufIndx)
         unsigned           fTgtShift; // current shift in the buffer
         UdpDataPacket      fTgtHdr;   // place where data header should be received
         char*              fTgtPtr;   // location where next data should be received
         uint32_t           fTgtNextId; // expected id of next packet
         uint32_t           fTgtTailId; // first id of packet which can not be dropped on ROC side
         bool               fTgtCheckGap;  // true if one should check gaps

         char               fTempBuf[2000]; // buffer to recv packet when no regular place is available

         dabc::Queue<ResendInfo>  fResend;

         bool               daqActive_;
         EDaqState          daqState;

         UdpDataPacketFull  fRecvBuf;

         unsigned           fBufferSize;
         unsigned           fTransferWindow;

         dabc::MemoryPool  *fPool;

         double   lastRequestTm;   // time when last request was send
         bool     lastRequestSeen; // indicate if we seen already reply on last request
         uint32_t lastRequestId;   // last request id
         uint32_t lastSendFrontId; // last send id of front packet

         unsigned rocNumber;

         uint64_t fTotalRecvPacket;
         uint64_t fTotalResubmPacket;


         virtual bool _ProcessReply(dabc::Command* cmd);

         virtual void ProcessPoolChanged(dabc::MemoryPool* pool) {}

         virtual bool ProcessPoolRequest();

         virtual double ProcessTimeout(double last_diff);

         void ConfigureFor(dabc::Port* port);

         void AddBuffersToQueue(bool checkanyway = false);
         bool CheckNextRequest(bool check_retrans = true);

         void AddDataPacket(int len, void* tgt);
         void CompressBuffer(dabc::Buffer* buf);

         void ResetDaq();

      public:
         UdpDataSocket(UdpDevice* dev, int fd);
         virtual ~UdpDataSocket();

         virtual void ProcessEvent(dabc::EventId evnt);

         virtual bool ProvidesInput() { return true; }
         virtual bool ProvidesOutput() { return false; }

         virtual bool Recv(dabc::Buffer* &buf);
         virtual unsigned  RecvQueueSize() const;
         virtual dabc::Buffer* RecvBuffer(unsigned indx) const;
         virtual bool Send(dabc::Buffer* buf) { return false; }
         virtual unsigned SendQueueSize() { return 0; }
         virtual unsigned MaxSendSegments() { return 0; }

         virtual void StartTransport();
         virtual void StopTransport();
   };

}

#endif
