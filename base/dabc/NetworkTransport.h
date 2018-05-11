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

   /** \brief Network interface
    *
    * \ingroup dabc_all_classes
    *
    * Abstract interface for network transport, used by \ref dabc::NetworkTransport class
    */

   class NetworkInetrface {
      public:

         virtual ~NetworkInetrface() {}

         virtual void AllocateNet(unsigned fulloutputqueue, unsigned fullinputqueue) = 0;

         virtual void SubmitSend(uint32_t recid) = 0;
         virtual void SubmitRecv(uint32_t recid) = 0;
   };

   // ___________________________________________________________________

   /** \brief Network transport
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    * Base class to implement transport between modules on different nodes
    */

   class NetworkTransport : public Transport {
      public:

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

         enum ENetworkOperTypes {
            netot_Send     = 0x001U,
            netot_Recv     = 0x002U,
            netot_HdrSend  = 0x004U // use to send only network header without any additional data
         };

      protected:
         typedef Queue<uint32_t> NetIORecsQueue;

         NetworkInetrface*   fNet;

         uint32_t      fTransportId;

         bool          fUseAckn;
         unsigned      fInputQueueCapacity;    // capacity of input queue
         unsigned      fOutputQueueCapacity;   // capacity of output queue

         uint32_t      fNumRecs;
         uint32_t      fRecsCounter;
         NetIORec*     fRecs;
         int           fNumUsedRecs;

         unsigned      fOutputQueueSize; // number of output operations, submitted to the records
         unsigned      fAcknAllowedOper;
         NetIORecsQueue fAcknSendQueue;
         bool          fAcknSendBufBusy; // indicate if ackn sending is under way

         unsigned      fInputQueueSize; // total number of buffers, using for receiving : in device recv queue and input queue, not yet cleaned by user
         bool          fFirstAckn;      // indicates, if first ackn packet was send or not
         unsigned      fAcknReadyCounter; // counter of submitted to device recv packets

         BufferSize_t  fFullHeaderSize;  // total header size
         BufferSize_t  fInlineDataSize;  // part of the header, which can be used for inline data (in the end of header)

         bool          fStartBufReq;     ///< if true, request to memory pool was started and one should wait until it is finished

         uint32_t TakeRec(Buffer& buf, uint32_t kind = 0, uint32_t extras = 0);
         void ReleaseRec(uint32_t recid);

         void FillRecvQueue(Buffer* freebuf = 0, bool onlyfreebuf = false);
         bool CheckAcknReadyCounter(unsigned newitems = 0);
         void SubmitAllowedSendOperations();

         // methods inherited from the module
         virtual void OnThreadAssigned();
         virtual void ProcessInputEvent(unsigned port);
         virtual void ProcessOutputEvent(unsigned port);
         virtual void ProcessPoolEvent(unsigned pool);
         virtual void ProcessTimerEvent(unsigned timer);

         // methods inherited from transport
         virtual bool StartTransport();
         virtual bool StopTransport();

         virtual void TransportCleanup();

      public:
         virtual const char* ClassName() const { return "NetworkTransport"; }

         NetworkTransport(dabc::Command cmd, const PortRef& inpport, const PortRef& outport,
                          bool useackn, WorkerAddon* addon);
         virtual ~NetworkTransport();

         unsigned GetFullHeaderSize() const { return fFullHeaderSize; }

         /** \brief Provides name of memory pool, used by transport
          *
          * We made method public to use in some other places -
          * one should take care that same thread is used */
         std::string TransportPoolName() const { return PoolName(); }

         // methods should be available for NetworkAdons for back-calls
         void SetRecHeader(uint32_t recid, void* header);
         int PackHeader(uint32_t recid);

         unsigned NumRecs() const { return fNumRecs; }
         NetIORec* GetRec(unsigned id) const { return id < fNumRecs ? fRecs + id : 0; }

         void ProcessSendCompl(uint32_t recid);
         void ProcessRecvCompl(uint32_t recid);

         static void GetRequiredQueuesSizes(const PortRef& port, unsigned& input_size, unsigned& output_size);
         static bool Make(const ConnectionRequest& req, WorkerAddon* addon, const std::string &devthrdname = "");
   };


}

#endif
