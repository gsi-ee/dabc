// $Id$

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

#ifndef DABC_NetworkTransport
#define DABC_NetworkTransport

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#ifndef DABC_Queue
#include "dabc/Queue.h"
#endif

namespace dabc {

   class MemoryPool;

   class NetworkTransport : public Transport,
                            public MemoryPoolRequester {

      typedef Queue<uint32_t> NetIORecsQueue;

      protected:

         enum ENetworkOperTypes {
            netot_Send     = 0x001U,
            netot_Recv     = 0x002U,
            netot_HdrSend  = 0x004U // use to send only network header without any additional data
         };

         struct NetIORec {
            bool     used;
            uint32_t kind;
            uint32_t extras;  // remake with use of unions
            Buffer   buf;
            void*    header;
            void*    inlinebuf;

            NetIORec() : used(false), kind(0), extras(0), buf(), header(0), inlinebuf(0) {}
         };


#pragma pack(1)

         struct NetworkHeader {
            uint32_t chkword;
            uint32_t kind;
            uint32_t typid;
            uint32_t size;
         };

#pragma pack(0)

         uint32_t      fTransportId;

         bool          fIsInput;
         bool          fIsOutput;

         bool          fUseAckn;
         unsigned      fInputQueueLength;
         unsigned      fOutputQueueLength;

         // FIXME: transport should use worker mutex
         Mutex         fMutex;
         uint32_t      fNumRecs;
         uint32_t      fRecsCounter;
         NetIORec*     fRecs;
         int           fNumUsedRecs;

         unsigned      fOutputQueueSize;
         unsigned      fAcknAllowedOper;
         NetIORecsQueue fAcknSendQueue;
         bool          fAcknSendBufBusy; // indicate if ackn sending is under way

         unsigned      fInputQueueSize; // total number of buffers, using for receiving : in device recv queue and input queue, not yet cleaned by user
         NetIORecsQueue fInputQueue;   // buffers, which already delivered by device
         bool          fFirstAckn;      // indicates, if first ackn packet was send or not
         unsigned      fAcknReadyCounter; // counter of submitted to device recv packets

         BufferSize_t  fFullHeaderSize;  // total header size
         BufferSize_t  fInlineDataSize;  // part of the header, which can be used for inline data (in the end of header)

         NetworkTransport(Reference port, bool useakn);
         virtual ~NetworkTransport();

         /** \brief Internal method, should be called already in constructor of inherited class */
         void InitNetworkTransport(Object*);

         /** \brief Reimplementation from base class, invoked when port is confirm assignment */
         virtual void PortAssigned();

         /** \brief Inherited method, used to remove all references */
         virtual void CleanupFromTransport(Object* obj);

         /** \brief Call from inherited class, cleanup transport */
         virtual void CleanupTransport();

         /** \brief Internal method, used to cleanup queue from all buffers */
         void CleanupNetTransportQueue();

         uint32_t _TakeRec(const Buffer& buf, uint32_t kind = 0, uint32_t extras = 0);
         void _ReleaseRec(uint32_t recid);
         void SetRecHeader(uint32_t recid, void* header);

         void FillRecvQueue(Buffer* freebuf = 0);
         bool CheckAcknReadyCounter(unsigned newitems = 0);
         void _SubmitAllowedSendOperations();

         // this method must be reimplemented in inherited classes
         virtual void _SubmitRecv(uint32_t recid) = 0;
         virtual void _SubmitSend(uint32_t recid) = 0;

         /** Prepare network record to be send via transport
          * Some records do not contains buffers or buffer content is send as inline data
          * Returns 0 - failure, 1 - only header should be send, 2 - header and buffer should be send */
         int PackHeader(uint32_t recid);

      public:

         inline int GetId() const { return fTransportId; }
         // these methods must be called by appropriate device threads when
         // operation was completed
         void ProcessSendCompl(uint32_t recid);
         void ProcessRecvCompl(uint32_t recid);

         virtual bool ProvidesInput() { return fIsInput; }
         virtual bool ProvidesOutput() { return fIsOutput; }

         virtual bool Recv(Buffer &buf);
         virtual unsigned RecvQueueSize() const;
         virtual Buffer& RecvBuffer(unsigned indx) const;
         virtual bool Send(const Buffer&);
         virtual unsigned SendQueueSize();
   };
}

#endif
