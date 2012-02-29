#ifndef BNET_TransportRunnable
#define BNET_TransportRunnable

#include "dabc/threads.h"

#include "dabc/statistic.h"

#include "dabc/Queue.h"

#include "dabc/Command.h"

#include "dabc/Module.h"

#include "dabc/MemoryPool.h"

#include "bnet/defines.h"



namespace bnet {

   typedef dabc::Queue<int> QueueInt;

   enum OperKind {
      kind_None,
      kind_Send,
      kind_Recv
   };

   enum OperSKind {
      skind_None,
      skind_SyncMasterSend,
      skind_SyncMasterRecv,
      skind_SyncSlaveSend,
      skind_SyncSlaveRecv
   };

   struct OperRec {
      OperKind      kind;       // kind of operation
      OperSKind     skind;      // special kind of operation for time sync (and more)
      double        oper_time;  // scheduled time of operation
      double        is_time;    // actual time when operation was executed
      double        compl_time; // actual time when operation was completed
      dabc::Buffer  buf;        // buffer for operation
      void*         header;     // pointer on the header, used for debug
      int           hdrsize;    // argument, specified when record was created
      int           tgtnode;    // id of node to submit
      int           tgtindx;    // second id for the target, LID number in case of IB
      int           repeatcnt;  // how many times operation should be repeated, used for time sync
      bool          err;        // is operation failed or not
      int*          queuelen;   // pointer on queue counter
      int           queuelimit; // limit for queue length

      OperRec() :
         kind(kind_None), skind(skind_None),
            oper_time(0.), is_time(0.), compl_time(0.),
            buf(), header(0), hdrsize(0), tgtnode(0), tgtindx(0), repeatcnt(0), err(false),
            queuelen(0) {}

      void SetRepeatCnt(int cnt) { repeatcnt = cnt; }

      void SetTarget(int node, int indx = 0) { tgtnode = node; tgtindx = indx; }

      void SetImmediateTime() { oper_time = 0; }

      void SetTime(double tm) { oper_time = tm; }

      inline bool IsQueueOk() { return queuelen==0 ? true : *queuelen < queuelimit; }

      inline void inc_queuelen() { if (queuelen) *queuelen++; }
      inline void dec_queuelen() { if (queuelen) *queuelen--; }
   };

   class TimeStamping {
      protected:
         double fTimeShift;
         double fTimeScale;
      public:
         TimeStamping() : fTimeShift(0.), fTimeScale(1.) {}
         ~TimeStamping() {}

         inline double get(const dabc::TimeStamp &tm) const { return tm()*fTimeScale + fTimeShift; }
         inline double get(double tm) const { return tm*fTimeScale + fTimeShift; }

         inline double operator()() const { return get(dabc::Now()); }
         inline double operator()(double tm) const { return get(tm); }
         inline double operator()(const dabc::TimeStamp &tm) const { return get(tm); }

         void ChangeShift(double shift);
         void ChangeScale(double koef);

         void GetCoeff(void* args) { ((double*) args)[0] = fTimeShift; ((double*) args)[1] = fTimeScale; }
         void SetCoeff(void* args) { fTimeShift = ((double*) args)[0]; fTimeScale = ((double*) args)[1]; }
   };

   class TransportModule;

   class TransportRunnable : public dabc::Runnable {
      protected:

         friend class TransportModule;

         dabc::Mutex fMutex; /** Main mutex for communication with the transport module */

         /** Condition can be (not necessary in all cases) used for waiting of
          * new requests from module */
         dabc::Condition fCondition;

         /** Item used to propogate events back to the module */
         dabc::ModuleItem* fReplyItem;


         int    fNodeId; // node number
         int    fNumNodes;   // total number of node
         int    fNumLids;
         bool*  fActiveNodes; // arrays with active node flags

         /** Short story about each record:
          * It is preallocated descriptor about any kind of transport operation.
          * In the beginning all records are free and their ids collected in fFreeRecs queue.
          * fFreeRecs queue is fully used by module thread and does not requires any locking.
          * At any time module can reserve and prepare record and submit it to fSubmRecs, protected by fMutex.
          * Runnable can copy (using mutex) records id from submitted list to fAcceptedRecs queue.
          * From this moment runnable is fully controls record and perform each operation according specified time,
          * marking recid in fRunningRecs vector. Transport implementation (IB or socket) should inform
          * runnable when operation is completed and than record is moved to fCompletedRecs.
          * Once completed, record will be moved to fReplyRecs queue (using mutex) which is accessible also
          * by the module thread again. TODO: probably one need to signal module that new record is there.
          * Module (using mutex) extracts
          *
          *
          * */

         int              fNumRecs;        // number of records
         OperRec*         fRecs;          // operation records
         dabc::Queue<int> fFreeRecs;      // list of free records - used fully in module thread
         dabc::Queue<int> fSubmRecs;      // list of submitted records - shared between module and transport threads
         dabc::Queue<int> fAcceptedRecs;  // list of accepted records - used only in transport thread
         int fNumRunningRecs; // number of submitted records
         std::vector<bool> fRunningRecs;  // list of running records - used only in transport thread
         dabc::Queue<int> fCompletedRecs; // list of completed records - used only in transport thread
         dabc::Queue<int> fReplyedRecs;   // list of replies - shared between runnable and module
         bool             fReplySignalled;  // indicate if reply event was generated after last access to reply queue from module
         int              fSegmPerOper;     // maximal allowed number of segments in operation (1 for header + rest for buffer)
         int              fHeaderBufSize;   // size of buffer for header

         dabc::Command    fCmd; // command which could be executed in the runnable loop
         enum CommandState { state_None, state_Submitted, state_Executed, state_Returned };
         CommandState     fCmdState;  // state of the command
         bool             fMainLoopActive;

         std::vector<int> fSendQueue;  // running send queue, NumLids() * NumNodes()
         std::vector<int> fRecvQueue;  // running recv queue, NumLids() * NumNodes()

         dabc::MemoryPool fHeaderPool;  // list of headers for transport operations

         dabc::Thread_t fModuleThrd;  // id of module thread, used for debugging
         dabc::Thread_t fTransportThrd;  // id of transport thread, used for debugging

         TimeStamping  fStamping;   // source of global time stamps

         dabc::Average fLoopTime;  // average loop time, for statistic
         int          fLoopMaxCnt;  // time when loop too big (more than 1 ms)

         int          fSendQueueLimit; // limit for send operations in send queue
         int          fRecvQueueLimit; // limit for recv operations in recv queue


         // this variables block used by time synchronization
         bool   fDoTimeSync;
         bool   fDoScaleSync;
         int    fNumSyncCycles;
         DoublesVector fSync_m_to_s, fSync_s_to_m, fSyncTimes;
         double fSyncSetTimeScale, fSyncSetTimeShift, fSyncSendTime, fSyncMaxCut;
         bool   fSyncResetTimes;
         int    fSyncCycle;
         int    fSyncSlaveRec; // prepeared record for sending reply buffer by slave
         int    fSyncMasterRec; // prepeared record for sending request buffer by master
         bool   fSyncRecvDone; // indicates that next recv operation (slave or master) is done
         bool   fSyncSendDone; // indicates that next send operation (slave or master) is done

         int&   SendQueue(int indx, int node) { return fSendQueue[indx*NumNodes() + node]; }
         int&   RecvQueue(int indx, int node) { return fRecvQueue[indx*NumNodes() + node]; }

         #define CheckModuleThrd() \
            if (dabc::PosixThread::Self() != fModuleThrd) { \
               EOUT(("%s called from wrong thread - expected module thread", __func__)); \
               exit(1); \
            }

         #define CheckTransportThrd() \
            if (dabc::PosixThread::Self() != fTransportThrd) { \
               EOUT(("%s called from wrong thread - expected transport thread", __func__)); \
               exit(1); \
            }

         enum RunnableCommands {
            cmd_None,       // no any command
            cmd_Exit,       // exit from transport thread
            cmd_ActiveNodes,// set flags which nodes are active and which are not
            cmd_CreateQP,   // create connection handles - queue pair in IB
            cmd_ConnectQP,  // create connection handles - queue pair in IB
            cmd_ConfigSync, // configure time sync parameters
            cmd_GetSync,    // return sync coefficients from runnable
            cmd_CloseQP,     // create connection handles - queue pair in IB
            cmd_ResetStat,   // reset statistic
            cmd_GetStat      // return statistic
         };

         int NodeId() const { return fNodeId; }
         int NumNodes() const { return fNumNodes; }
         int NumLids() const { return fNumLids; }
         bool IsActiveNode(int id) const { return true; }
         int NumRunningRecs() const { return fNumRunningRecs; }


         /*************** Methods, used from other threads ******************/

         /** Method should be used from module thread to submit new command to the runnable,
          * returned is recid which is handle of the command */
         bool SubmitCmd(int cmdid, void* args = 0, int argssize = 0);

         /** Method should be used from module thread to submit command and wait for its execution */
         bool ExecuteCmd(int cmdid, void* args = 0, int argssize = 0);

         /** Method which is only allow to call from the runnable thread itself */
         virtual bool ExecuteCreateQPs(void* args, int argssize) { return false; }
         virtual bool ExecuteConnectQPs(void* args, int argssize) { return false; }
         virtual bool ExecuteConfigSync(int* args);
         virtual bool ExecuteCloseQPs() { return false; }

         /** Method called in transport thread to actually execute command */
         bool ExecuteTransportCommand(int cmdid, void* args, int argssize);

         void PrepareSpecialKind(int& recid);
         void ProcessSpecialKind(int recid);

         virtual void RunnableCancelled() {}

         virtual bool DoPrepareRec(int recid) { return false; }

         virtual bool DoPerformOperation(int recid) { return false; }
         virtual int DoWaitOperation(double waittime, double fasttime) { return false; }

         inline void PerformOperation(int recid, double tm)
         {
            if (DoPerformOperation(recid)) {
               fRunningRecs[recid] = true;
               GetRec(recid)->is_time = tm;
               GetRec(recid)->inc_queuelen();
               fNumRunningRecs++;
            } else {
               GetRec(recid)->err = true;
               GetRec(recid)->is_time = tm;
               fCompletedRecs.Push(recid);
            }
         }

      public:

         // dabc::Mutex& mutex() { return fMutex; }

         TransportRunnable();
         virtual ~TransportRunnable();

         void SetNodeId(int id, int num, int nlids, dabc::ModuleItem* reply) { fNodeId = id; fNumNodes = num; fNumLids = nlids; fReplyItem = reply; }

         void SetThreadsIds(dabc::Thread_t modid, dabc::Thread_t trid) { fModuleThrd = modid; fTransportThrd = trid; }

         virtual bool Configure(dabc::Module* m, dabc::MemoryPool* pool, dabc::Command cmd);

         virtual void* MainLoop();

         // =================== Interface commands to the runnable ===================

         // size required for storing all connection handles of local node
         virtual int ConnectionBufferSize() { return 1024; }

         virtual void ResortConnections(void*) { }

         bool SetActiveNodes(void* data, int datasize);
         bool CreateQPs(void* recs, int recssize);
         bool ConnectQPs(void* recs, int recssize);
         bool ConfigSync(bool master, int nrepeat, bool dosync = false, bool doscale = false);
         bool ConfigQueueLimits(int send_limit, int recv_limit);
         bool GetSync(TimeStamping& stamp);
         bool ResetStatistic();
         bool GetStatistic(double& mean_loop, double& max_loop, int& long_cnt);

         bool CloseQPs();
         bool StopRunnable();

         bool IsFreeRec() const { return !fFreeRecs.Empty(); }

         /** Method is preparing all necessary structures for submitting send/recv operation
          * Method executed in caller (module) thread
          * Return value is record id, -1 - failure    */
         virtual int PrepareOperation(OperKind kind, int hdrsize, dabc::Buffer buf = dabc::Buffer());

         OperRec* GetRec(int recid) { return (recid>=0) && (recid<=fNumRecs) ? fRecs + recid : 0; }

         virtual bool ReleaseRec(int recid);

         /** Method used to submit record into runnable queue */
         bool SubmitRec(int recid);

         /** Method to get next completion record id
          * TODO: probably one could use queue with producer/consumer, in this case one could accept many items at once */
         int GetCompleted(bool resetsignalled = true);
   };

}

#endif
