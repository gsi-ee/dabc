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

#include <netinet/in.h>
#include <sys/socket.h>


#include "hadaq/HadaqTypeDefs.h"



#define DEFAULT_MTU 63 * 1024

namespace hadaq {





   class UdpDataSocket : public dabc::SocketWorker,
                         public dabc::Transport,
                         protected dabc::MemoryPoolRequester {
      friend class UdpDevice;

      DABC_TRANSPORT(dabc::SocketWorker)

      protected:




         struct sockaddr_in    fSockAddr;
         size_t             fMTU;
         dabc::Mutex        fQueueMutex;
         dabc::BuffersQueue fQueue;


         /*
          * Schema of the buffer pointer meanings:
          *   fTgtBuf.SegmentPtr()  - begin of dabc buffer segment
          *                                  ^
          *                            previous events length
          *                                  v
          *    fCurrentEvent   - begin of event header
          *                                  ^
          *                            sizeof(hadaq::Event) (if fBuildFullEvent) or 0
          *                                  v
          *    fTgtPtr        - begin of current subevent
          *                                  ^
          *                              fTgtShift
          *                                  v
          *    (fTgtPtr +  fTgtShift) - next position to write data
          *                                  ^
          *                            fTgtBuf.SegmentSize() -  (fTgtPtr +  fTgtShift)
          *                                  v
          *    fEndPtr                - end of buffer segment
          */


         dabc::Buffer       fTgtBuf;   // pointer on buffer, where next portion can be received, use pointer while it is buffer from the queue
         unsigned           fTgtShift; // current shift in the buffer
         char*              fTgtPtr;   // location of next subevent header data to be received
         char*              fEndPtr;   // end of current buffer
         char*              fTempBuf; // buffer to recv packet when no regular place is available

         unsigned           fBufferSize;

         hadaq::Event*      fCurrentEvent; // points to begin of current event structure


         dabc::MemoryPoolRef fPool;  // reference on the pool, reference should help us to preserve pool as long as we are using it


         uint64_t           fTotalRecvPacket;
         uint64_t           fTotalDiscardPacket;
         uint64_t           fTotalRecvMsg;
         uint64_t           fTotalDiscardMsg;
         uint64_t           fTotalRecvBytes;
         uint64_t           fTotalRecvEvents;

         //double             fFlushTimeout;  // after such timeout partially filled packed will be delivered

         /* if true, we will produce full hadaq events with subevent for direct use.
          * otherwise, produce subevent stream for consequtive event builder module.
          */
         bool fBuildFullEvent;

         virtual bool ReplyCommand(dabc::Command cmd);

         virtual void ProcessPoolChanged(dabc::MemoryPool* pool) {}

         virtual bool ProcessPoolRequest();

         virtual double ProcessTimeout(double last_diff);

         void ConfigureFor(dabc::Port* port);


         virtual int GetParameter(const char* name);

//         void setFlushTimeout(double tmout) { fFlushTimeout = tmout; }
//         double getFlushTimeout() const {  return fFlushTimeout; }

         int OpenUdp(int& portnum, int portmin, int portmax, int & rcvBufLenReq);

         /* use recvfrom() as in hadaq nettrans::recvGeneric*/
         ssize_t DoUdpRecvFrom(void* buf, size_t len, int flags=0);

         /* copy next received unit from udp buffer to dabc buffer*/
         void ReadUdp();

         /* Switch to next dabc buffer, put old one into receive queue
          * copyspanning will copy a spanning hadtu fragment from old to new buffers*/
         void NewReceiveBuffer(bool copyspanning=false);

         /*
          * Finalize current event structure and set up new event header after the current target pointer
          */
         void NextEvent();

      public:
         UdpDataSocket(dabc::Reference port);
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
