#ifndef DABC_VerbsTransport
#define DABC_VerbsTransport

#ifndef DABC_NetworkTransport
#include "dabc/NetworkTransport.h"
#endif

#ifndef DABC_VerbsThread
#include "dabc/VerbsThread.h"
#endif

struct ibv_mr;
struct ibv_recv_wr;
struct ibv_send_wr;
struct ibv_sge; 

namespace dabc {
    
   class VerbsQP;
   class VerbsCQ;
   class VerbsDevice;
   class VerbsMemoryPool;
   class VerbsPoolRegistry;
   
   class VerbsTransport : public NetworkTransport,
                          public VerbsProcessor {

      friend class VerbsDevice;

      enum EVerbsTransportEvents { 
         evntVerbsSendRec = evntVerbsLast,
         evntVerbsRecvRec,
         evntVerbsPool
      };
       
      protected:  
         VerbsDevice         *fVerbs;
         VerbsCQ             *fCQ;          
         VerbsPoolRegistry   *fPoolReg;
         struct ibv_recv_wr  *f_rwr; // field for recieve configs, allocated dynamically
         struct ibv_send_wr  *f_swr; // field for send configs, allocated dynamically
         struct ibv_sge      *f_sge; // memory segement description, used for both send/recv
         VerbsMemoryPool     *fHeadersPool; // polls with headers
         unsigned            fSegmPerOper;
         bool                fFastPost; 
       
         virtual void _SubmitSend(uint32_t recid);
         virtual void _SubmitRecv(uint32_t recid);
    
         virtual bool ProcessPoolRequest();
     
         virtual void ProcessEvent(EventId evnt);
       
      public:
         VerbsTransport(VerbsDevice* dev, VerbsCQ* cq, VerbsQP* qp, Port* port);
         virtual ~VerbsTransport();
         
         virtual unsigned MaxSendSegments() { return fSegmPerOper - 1; }

         virtual void VerbsProcessSendCompl(uint32_t);
         virtual void VerbsProcessRecvCompl(uint32_t);
         virtual void VerbsProcessOperError(uint32_t);
   };
}

#endif
