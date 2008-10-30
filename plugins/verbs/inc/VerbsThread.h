#ifndef DABC_VerbsThread
#define DABC_VerbsThread

#ifndef DABC_WorkingProcessor
#include "dabc/WorkingProcessor.h"
#endif

#include <map>
#include <stdint.h>

struct ibv_comp_channel;
struct ibv_wc;
struct ibv_ah;

#define VERBS_USING_PIPE

namespace dabc {
    
   class VerbsDevice; 
   class VerbsThread;
   class VerbsMemoryPool;
   
   class VerbsTimeoutProcessor;
   class VerbsProtocolProcessor;
   
   class VerbsQP;
   class VerbsCQ;
   class Port;
   class Command;
    
   class VerbsProcessor : public WorkingProcessor {
      
      friend class SocketThread;

      protected:

         VerbsQP      *fQP;    

      public:

         enum EVerbsProcEvents { 
            evntVerbsSendCompl = evntFirstUser,
            evntVerbsRecvCompl,
            evntVerbsError,
            evntVerbsLast // from this event user can specified it own events
         };

         VerbsProcessor(VerbsQP* qp);
         
         virtual ~VerbsProcessor();
         
         virtual const char* RequiredThrdClass() const { return "VerbsThread"; }
         
         void SetQP(VerbsQP* qp);

         inline VerbsQP* QP() const { return fQP; }
         
         virtual void ProcessEvent(dabc::EventId);

         virtual void VerbsProcessSendCompl(uint32_t) {}
         virtual void VerbsProcessRecvCompl(uint32_t) {}
         virtual void VerbsProcessOperError(uint32_t) {}
         
         void CloseQP();
   }; 
   
   // ______________________________________________________________
   
   class VerbsConnectProcessor : public VerbsProcessor {
      protected:
         enum { ServQueueSize = 100, ServBufferSize = 256 };
         VerbsCQ           *fCQ;
         VerbsMemoryPool   *fPool;
         bool               fUseMulti;
         uint16_t           fMultiLID;
         struct ibv_ah     *fMultiAH;
         int                fConnCounter;

         VerbsThread* thrd() const { return (VerbsThread*) ProcessorThread(); }

      public:
         VerbsConnectProcessor(VerbsThread* thrd);
         virtual ~VerbsConnectProcessor();
         
         VerbsCQ* CQ() const { return fCQ; }
         int NextConnCounter() { return fConnCounter++; }

         virtual void VerbsProcessSendCompl(uint32_t);
         virtual void VerbsProcessRecvCompl(uint32_t);
         virtual void VerbsProcessOperError(uint32_t);
         
         bool TrySendConnRequest(VerbsProtocolProcessor* proc);

   };

   // _____________________________________________________________
   
   class VerbsProtocolProcessor : public VerbsProcessor {
      public:
         bool            fServer;
         String          fPortName;
         Command        *fCmd;
         VerbsQP        *fPortQP;
         VerbsCQ        *fPortCQ;

         int             fConnType; // UD, RC, UC
         String          fThrdName;
         double          fTimeout;
         String          fConnId;
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
         VerbsMemoryPool *fPool;
         
         bool            fConnected;

      public:
         VerbsProtocolProcessor(VerbsThread* thrd,
                                VerbsQP* hs_qp,
                                bool server,
                                const char* portname,
                                Command* cmd);
         virtual ~VerbsProtocolProcessor();

         virtual void VerbsProcessSendCompl(uint32_t);
         virtual void VerbsProcessRecvCompl(uint32_t);
         virtual void VerbsProcessOperError(uint32_t);
         
         virtual void OnThreadAssigned();
         virtual double ProcessTimeout(double last_diff);
         
         bool StartHandshake(bool dorecv, void* connrec);

         VerbsThread* thrd() const { return (VerbsThread*) ProcessorThread(); }
   };


   // ______________________________________________________________
    
   class VerbsThread : public WorkingThread {
       
      typedef std::map<uint32_t, uint32_t>  ProcessorsMap;
       
      enum { LoopBackQueueSize = 10 };
 
      protected:
         enum EWaitStatus { wsWorking, wsWaiting, wsFired };
       
         VerbsDevice             *fDevice;           
         struct ibv_comp_channel *fChannel;
      #ifdef VERBS_USING_PIPE
         int                      fPipe[2];
      #else
         VerbsQP                 *fLoopBackQP;
         VerbsMemoryPool         *fLoopBackPool;
         int                      fLoopBackCnt;
         VerbsTimeoutProcessor   *fTimeout; // use to produce timeouts
      #endif
         VerbsCQ                 *fMainCQ; 
         long                     fFireCounter;
         EWaitStatus              fWaitStatus; 
         ProcessorsMap            fMap; // for 1000 elements one need about 50 nanosec to get value from it
         int                      fWCSize; // size of array 
         struct ibv_wc*           fWCs; // list of event completion
         VerbsConnectProcessor   *fConnect;  // connection initiation 
         long                     fFastModus; // makes polling over MainCQ before starting wait - defines pooling number
      public:
         // list of all events for all kind of socket processors
         
         VerbsThread(VerbsDevice* dev, Basic* parent, const char* name = "VerbsThread");
         virtual ~VerbsThread();
         
         void CloseThread();

         struct ibv_comp_channel* Channel() const { return fChannel; }

         VerbsCQ* MakeCQ();
         
         VerbsDevice* GetDevice() const { return fDevice; }
         
         void FireDoNothingEvent();
         
         bool DoServer(Command* cmd, Port* port, const char* portname);
         bool DoClient(Command* cmd, Port* port, const char* portname);
         void FillServerId(String& id);

         VerbsConnectProcessor* Connect() const { return fConnect; }
         
         VerbsProtocolProcessor* FindProtocol(const char* connid);
         
         bool IsFastModus() const { return fFastModus > 0; }
      
         virtual int ExecuteCommand(Command* cmd);

         virtual const char* ClassName() const { return "VerbsThread"; }

         virtual bool CompatibleClass(const char* clname) const;
         
         static const char* StatusStr(int code);
   
      protected:
      
         bool StartConnectProcessor();
      
         virtual dabc::EventId WaitEvent(double tmout);
      
         virtual void _Fire(dabc::EventId arg, int nq);
         
         virtual void ProcessorNumberChanged();
   };
}

#endif
