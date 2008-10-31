#ifndef VERBS_Processor
#define VERBS_Processor

#ifndef DABC_WorkingProcessor
#include "dabc/WorkingProcessor.h"
#endif

#include <stdint.h>

struct ibv_ah;

namespace dabc {
   class Port;
}

namespace verbs {

   class QueuePair;
   class ComplQueue;
   class MemoryPool;
   class Thread;

   class Processor : public dabc::WorkingProcessor {

      protected:

         QueuePair      *fQP;

      public:

         enum EVerbsProcEvents {
            evntVerbsSendCompl = evntFirstUser,
            evntVerbsRecvCompl,
            evntVerbsError,
            evntVerbsLast // from this event user can specified it own events
         };

         Processor(QueuePair* qp);

         virtual ~Processor();

         virtual const char* RequiredThrdClass() const;

         void SetQP(QueuePair* qp);

         inline QueuePair* QP() const { return fQP; }

         virtual void ProcessEvent(dabc::EventId);

         virtual void VerbsProcessSendCompl(uint32_t) {}
         virtual void VerbsProcessRecvCompl(uint32_t) {}
         virtual void VerbsProcessOperError(uint32_t) {}

         void CloseQP();
   };

   // _____________________________________________________________

   class ProtocolProcessor : public Processor {
      public:
         bool            fServer;
         dabc::String    fPortName;
         dabc::Command  *fCmd;
         QueuePair      *fPortQP;
         ComplQueue     *fPortCQ;

         int             fConnType; // UD, RC, UC
         dabc::String    fThrdName;
         double          fTimeout;
         dabc::String    fConnId;
         double          fLastAttempt;

         int             fKindStatus;

         // address data of UD port
         uint32_t        fRemoteLID;
         uint32_t        fRemoteQPN;

         // address data of tranport on other side
         uint32_t        fTransportQPN;
         uint32_t        fTransportPSN;
         struct ibv_ah  *f_ah;

         bool            fConnectedHS;
         MemoryPool     *fPool;

         bool            fConnected;

      public:
         ProtocolProcessor(Thread* thrd,
                           QueuePair* hs_qp,
                           bool server,
                           const char* portname,
                           dabc::Command* cmd);
         virtual ~ProtocolProcessor();

         virtual void VerbsProcessSendCompl(uint32_t);
         virtual void VerbsProcessRecvCompl(uint32_t);
         virtual void VerbsProcessOperError(uint32_t);

         virtual void OnThreadAssigned();
         virtual double ProcessTimeout(double last_diff);

         bool StartHandshake(bool dorecv, void* connrec);

         Thread* thrd() const { return (Thread*) ProcessorThread(); }
   };

   // ______________________________________________________________

   class ConnectProcessor : public Processor {
      protected:
         enum { ServQueueSize = 100, ServBufferSize = 256 };
         ComplQueue        *fCQ;
         MemoryPool        *fPool;
         bool               fUseMulti;
         uint16_t           fMultiLID;
         struct ibv_ah     *fMultiAH;
         int                fConnCounter;

         Thread* thrd() const { return (Thread*) ProcessorThread(); }

      public:
         ConnectProcessor(Thread* thrd);
         virtual ~ConnectProcessor();

         ComplQueue* CQ() const { return fCQ; }
         int NextConnCounter() { return fConnCounter++; }

         virtual void VerbsProcessSendCompl(uint32_t);
         virtual void VerbsProcessRecvCompl(uint32_t);
         virtual void VerbsProcessOperError(uint32_t);

         bool TrySendConnRequest(ProtocolProcessor* proc);

   };


}

#endif
