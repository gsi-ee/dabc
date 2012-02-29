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

         bool fRelibaleConn; // is reliable connection will be used

         verbs::ContextRef  fIbContext;

         verbs::ComplQueue* fCQ;                 //!< completion queue, for a moment single
         verbs::QueuePair** fQPs[BNET_MAXLID]; //!< arrays of QueuePairs pointers, NumNodes X NumLids

         int               *fSendQueue[BNET_MAXLID];    // size of individual sending queue
         int               *fRecvQueue[BNET_MAXLID];    // size of individual receiving queue

         struct ibv_recv_wr  *f_rwr; // field for receive config, allocated dynamically
         struct ibv_send_wr  *f_swr; // field for send config, allocated dynamically
         struct ibv_sge      *f_sge; // memory segment description, used for both send/recv

         verbs::PoolRegistryRef   fHeaderReg;  // verbs registry for headers pool
         verbs::PoolRegistryRef   fMainReg;    // verbs registry for main data pool

         virtual bool ExecuteCreateQPs(void* args, int argssize);
         virtual bool ExecuteConnectQPs(void* args, int argssize);
         virtual bool ExecuteCloseQPs();

         virtual bool DoPrepareRec(int recid);

         virtual bool DoPerformOperation(int recid);
         virtual int DoWaitOperation(double waittime, double fasttime);

      public:
         VerbsRunnable();
         virtual ~VerbsRunnable();

         bool IsReliableConn() const { return fRelibaleConn; }

         // maximum size for commands
         virtual int ConnectionBufferSize() { return NumNodes() * NumLids() * sizeof(VerbsConnRec); }

         virtual bool Configure(dabc::Module* m, dabc::MemoryPool* pool, dabc::Command cmd);

         virtual void ResortConnections(void* buf);
   };

}

#else

namespace bnet {
   typedef TransportRunnable VerbsRunnable;
}

#endif


#endif
