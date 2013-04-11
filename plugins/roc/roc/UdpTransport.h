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

#ifndef ROC_UDPTRANSPORT_H
#define ROC_UDPTRANSPORT_H

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef DABC_BuffersQueue
#include "dabc/BuffersQueue.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#ifndef ROC_UdpBoard
#include "roc/UdpBoard.h"
#endif

#include <list>

namespace roc {

   class UdpDevice;

   class UdpSocketAddon : public dabc::SocketAddon,
                          public dabc::DataInput {

      friend class UdpDevice;

      protected:
         UdpDevice*  fDev;

         enum EDaqState {
            daqInit,        // initial state
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


         unsigned           fBufferSize;       // when 0, first buffer will be used for basis
         unsigned           fTransferWindow;   // how many UDP buffers can be requested at the same time
         dabc::BuffersQueue fQueue;

         unsigned           fReadyBuffers;     // how many buffers in queue can be provided to user
         unsigned           fTransportBuffers; // how many buffers can be used for transport

         dabc::Buffer*      fTgtBuf;   // pointer on buffer, where next portion can be received, use pointer while it is buffer from the queue
         unsigned           fTgtBufIndx; // queue index of target buffer - use fQueue.Item(fReadyBuffers + fTgtBufIndx)
         unsigned           fTgtShift;     // current shift in the buffer
         unsigned           fTgtZeroShift; // initial shift in the buffer
         UdpDataPacket      fTgtHdr;   // place where data header should be received
         char*              fTgtPtr;   // location where next data should be received
         uint32_t           fTgtNextId; // expected id of next packet
         uint32_t           fTgtTailId; // first id of packet which can not be dropped on ROC side
         bool               fTgtCheckGap;  // true if one should check gaps

         char               fTempBuf[2000]; // buffer to recv packet when no regular place is available

         dabc::Queue<ResendInfo>  fResend;

         EDaqState          daqState;
         bool               daqCheckStop;

         UdpDataPacketFull  fRecvBuf;

         dabc::TimeStamp    lastRequestTm;   // time when last request was send
         bool               lastRequestSeen; // indicate if we seen already reply on last request
         uint32_t           lastRequestId;   // last request id
         uint32_t           lastSendFrontId; // last send id of front packet

         unsigned           rocNumber;

         uint64_t           fTotalRecvPacket;
         uint64_t           fTotalResubmPacket;

         double             fFlushTimeout;  // after such timeout partially filled packed will be delivered
         dabc::TimeStamp    fLastDelivery;  // time of last delivery

         base::OperList     fStartList;

         int                fFormat;

         bool               fMbsHeader;    // inserts dummy MBS header in the beginning
         int                fBufCounter;   // counter used in MBS header

         virtual double ProcessTimeout(double last_diff);
         virtual void ProcessEvent(const dabc::EventId&);
         virtual long Notify(const std::string& cmd, int);

         inline bool daqActive() const { return (fDev!=0) && (daqState == daqRuns); }

         void ResetDaq();
         void AddBufferToQueue(dabc::Buffer& buf);
         bool CheckNextRequest(bool check_retrans = true);
         void AddDataPacket(int len, void* tgt);
         void FinishTgtBuffer();
         void CheckReadyBuffers(bool doflush = false);
         void CompressBuffer(dabc::Buffer& buf);

         void ReplyTransport(dabc::Command cmd, bool res);

         void MakeCallback(unsigned arg);

         void setFlushTimeout(double tm) { fFlushTimeout = tm; }
         bool prepareForSuspend();

      public:
         UdpSocketAddon(UdpDevice* dev, int fd, int queuesize, int fmt, bool mbsheader, unsigned rocid);
         virtual ~UdpSocketAddon();


         virtual unsigned Read_Size() { return dabc::di_DfltBufSize; }
         virtual unsigned Read_Start(dabc::Buffer& buf);
         virtual unsigned Read_Complete(dabc::Buffer& buf);
         virtual double Read_Timeout() { return 0.1; }
   };


}

#endif
