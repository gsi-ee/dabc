#ifndef VERBS_BnetRunnable
#define VERBS_BnetRunnable

#include "dabc/bnetdefs.h"

#include "dabc/BnetRunnable.h"

#include "verbs/Context.h"
#include "verbs/ComplQueue.h"
#include "verbs/QueuePair.h"
#include "verbs/MemoryPool.h"

namespace verbs {

   #pragma pack(1)

   /** \brief Structure with information, required for connection establishing */

   struct VerbsConnRec {
      uint32_t lid;
      uint32_t qp;
      uint32_t psn;
   };

  #pragma pack()


   /** \brief Special runnable for BNET over InfiniBand
    *
    * Not yet functional */

   class BnetRunnable : public bnet::BnetRunnable {
      protected:

         bool fRelibaleConn; // is reliable connection will be used

         verbs::ContextRef  fIbContext;

         verbs::ComplQueue* fCQ;                 ///< completion queue, for a moment single
         verbs::QueuePair** fQPs[bnet::MAXLID]; ///< arrays of QueuePairs pointers, NumNodes X NumLids

         int               *fSendQueue[bnet::MAXLID];    // size of individual sending queue
         int               *fRecvQueue[bnet::MAXLID];    // size of individual receiving queue

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
         BnetRunnable(const std::string& name);
         virtual ~BnetRunnable();

         bool IsReliableConn() const { return fRelibaleConn; }

         // maximum size for commands
         virtual int ConnectionBufferSize() { return NumNodes() * NumLids() * sizeof(VerbsConnRec); }

         virtual bool Configure(const dabc::ModuleRef& m, const dabc::EventId& ev, dabc::MemoryPool* pool, int numrecs);

         virtual void ResortConnections(void* buf);
   };

}

#endif
