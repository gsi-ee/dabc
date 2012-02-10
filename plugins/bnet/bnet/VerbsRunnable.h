#ifndef BNET_VerbsRunnable
#define BNET_VerbsRunnable

#include "bnet/TransportRunnable.h"

#include "bnet/defines.h"



#ifdef WITH_VERBS

#include "verbs/Context.h"
#include "verbs/ComplQueue.h"
#include "verbs/QueuePair.h"
#include "verbs/MemoryPool.h"


namespace bnet {


   #pragma pack(1)

   struct VerbsConnRec {
      uint32_t lid;
      uint32_t qp;
      uint32_t psn;
   };

#pragma pack()



   class VerbsRunnable : public TransportRunnable {
      protected:

         int  fNumLids;       // maximum number of lids used for communication
         bool fRelibaleConn; // is reliable connection will be used

         verbs::ContextRef  fIbContext;

         verbs::ComplQueue* fCQ;                 //!< completion queue, for a moment single
         verbs::QueuePair** fQPs[IBTEST_MAXLID]; //!< arrays of QueuePairs pointers, NumNodes X NumLids

         int               *fSendQueue[IBTEST_MAXLID];    // size of individual sending queue
         int               *fRecvQueue[IBTEST_MAXLID];    // size of individual receiving queue

         virtual bool ExecuteCreateQPs();
         virtual bool ExecuteConnectQPs();
         virtual bool ExecuteCloseQPs();

      public:
         VerbsRunnable();
         virtual ~VerbsRunnable();

         int NumLids() const { return fNumLids; }
         bool IsReliableConn() const { return fRelibaleConn; }

         // maximum size for commands
         virtual int ConnectionBufferSize() { return NumNodes() * NumLids() * sizeof(VerbsConnRec); }

         virtual bool Configure(dabc::Module* m, dabc::Command cmd);

         virtual void ResortConnections(void* src, void* tgt);

   };

}

#else

namespace bnet {
   typedef TransportRunnable VerbsRunnable;
}

#endif


#endif
