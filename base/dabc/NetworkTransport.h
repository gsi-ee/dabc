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
#ifndef DABC_NetworkTransport
#define DABC_NetworkTransport

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
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
            uint64_t extras;  // remake with use of unions
            Buffer*  buf;
            void*    header;
            void*    usrheader;
         };


#pragma pack(1)

         struct NetworkHeader {
            uint32_t chkword;
            uint32_t kind;
            uint32_t typid;
            uint32_t hdrsize;
            uint32_t size;
         };

#pragma pack(0)

         uint32_t      fTransportId;

         bool          fIsInput;
         bool          fIsOutput;

         MemoryPool*   fPool;
         bool          fUseAckn;
         unsigned      fInputQueueLength;
         unsigned      fOutputQueueLength;

         Mutex         fMutex;
         uint32_t      fNumRecs;
         uint32_t      fRecsCounter;
         NetIORec*     fRecs;
         int           fNumUsedRecs;

         unsigned      fOutputQueueSize;
         unsigned      fAcknAllowedOper;
         NetIORecsQueue fAcknSendQueue;
         bool          fAcknSendBufBusy; // indicate if ackn sending is under way

         BufferSize_t  fInputBufferSize; // size of buffer, specified by user to be preselected from memory pool
         unsigned      fInputQueueSize; // total number of buffers, using for receieving : in device recv queue and input queue, not yet cleaned by user
         NetIORecsQueue fInputQueue;   // buffers, which already delivered by device
         bool          fFirstAckn;      // indicates, if first ackn packet was send or not
         unsigned      fAcknReadyCounter; // counter of submitted to device recv packets

         BufferSize_t  fFullHeaderSize;  // total header size
         BufferSize_t  fUserHeaderSize;  // user part of the header (in the end)

         NetworkTransport(Device* device);
         virtual ~NetworkTransport();

         void InitNetworkTransport(Port *port, bool useackn);
         void CleanupNetworkTransport();

         virtual void PortChanged();

         virtual void CleanupTransport();

         uint32_t _TakeRec(Buffer* buf, uint32_t kind = 0, uint64_t extras = 0);
         void _ReleaseRec(uint32_t recid);
         void SetRecHeader(uint32_t recid, void* header);

         void FillRecvQueue(Buffer* freebuf = 0);
         bool CheckAcknReadyCounter(unsigned newitems = 0);
         void _SubmitAllowedSendOperations();

         // this method must be reimplemented in inherited classes
         virtual void _SubmitRecv(uint32_t recid) = 0;
         virtual void _SubmitSend(uint32_t recid) = 0;

         int PackHeader(uint32_t recid);

      public:

         inline int GetId() const { return fTransportId; }
         // these methods must be called by appropriate device threads when
         // operation was completed
         void ProcessSendCompl(uint32_t recid);
         void ProcessRecvCompl(uint32_t recid);

         virtual bool ProvidesInput() { return fIsInput; }
         virtual bool ProvidesOutput() { return fIsOutput; }

         virtual bool Recv(Buffer* &buf);
         virtual unsigned RecvQueueSize() const;
         virtual Buffer* RecvBuffer(unsigned indx) const;
         virtual bool Send(Buffer*);
         virtual unsigned SendQueueSize();
   };
}

#endif
