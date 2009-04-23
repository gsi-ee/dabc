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

#ifndef NXYTER_Sorter
#include "nxyter/Sorter.h"
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
                           evntFillBuffer,
                           evntCheckRequest };

         enum EDaqState { daqInit, daqStarting, daqRuns, daqStopping, daqStopped, daqFails };


         struct ResendPkt {

            ResendPkt(uint32_t id = 0) :
               pktid(id), addtm(0.), lasttm(0.), numtry(0) {}

            uint32_t pktid;  // id of packet
            double   addtm;  // last resend time
            double   lasttm;  // last resend time
            int      numtry;

         };


         UdpDevice*         fDev;
         dabc::Mutex        fQueueMutex;
         dabc::BuffersQueue fQueue;

         bool               daqActive_;
         EDaqState          daqState;

         UdpDataPacketFull  fRecvBuf;

         unsigned           fBufferSize;

         dabc::MemoryPool  *fPool;


         unsigned rocNumber;

         unsigned transferWindow;      // maximum value for credits

         uint32_t lastRequestId;   // last request id
         double   lastRequestTm;   // time when last request was send
         bool     lastRequestSeen; // indicate if we seen already reply on last request
         uint32_t lastSendFrontId; // last send id of front packet

         UdpDataPacketFull* ringBuffer; // ring buffer for data
         unsigned      ringCapacity; // capacity of ring buffer
         unsigned      ringHead;     // head of ring buffer (first free place)
         uint32_t      ringHeadId;   // expected packet id for head buffer
         unsigned      ringTail;     // tail of ring buffer (last valid packet)
         unsigned      ringSize;     // number of items in ring buffer


         dabc::Mutex     dataMutex_;    // locks access to received data
//         pthread_cond_t  dataCond_;     // condition to synchronize data consumer
         unsigned        dataRequired_; // specifies how many messages required consumer
         unsigned        consumerMode_; // 0 - no consumer, 1 - waits condition, 2 - call back to controller
         bool            fillingData_;  // true, if consumer thread fills data from ring buffer
         uint8_t readBuf_[1800];       // intermediate buffer for getNextData()
         unsigned        readBufSize_;
         unsigned        readBufPos_;

         nxyter::Sorter* sorter_;

         std::list<ResendPkt>   packetsToResend;

         uint64_t fTotalRecvPacket;
         uint64_t fTotalResubmPacket;


         virtual bool _ProcessReply(dabc::Command* cmd);

         virtual void ProcessPoolChanged(dabc::MemoryPool* pool) {}

         virtual bool ProcessPoolRequest();

         virtual double ProcessTimeout(double last_diff);

         void ConfigureFor(dabc::Port* port);

         void TryToFillOutputBuffer();
         void CheckNextRequest();

         void KnutresetDaq();
         bool KnutstartDaq(unsigned trWin = 40);
         void KnutsendDataRequest(UdpDataRequestFull* pkt);
         bool Knut_checkDataRequest(UdpDataRequestFull* req, double curr_tm, bool check_retrans);
         void KnutaddDataPacket(UdpDataPacketFull* p, unsigned l);
         bool Knut_checkAvailData(unsigned num_msg);
         bool KnutfillData(void* buf, unsigned& sz);

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
