#ifndef BNET_TransportModule
#define BNET_TransportModule

#include "dabc/ModuleAsync.h"


#include "dabc/timing.h"
#include "dabc/statistic.h"
#include "dabc/threads.h"
#include "dabc/CommandsQueue.h"

#include "bnet/defines.h"
#include "dabc/BnetRunnable.h"
#include "bnet/Schedule.h"
#include "bnet/Structures.h"

struct ScheduleEntry {
    int    node;
    int    lid;
    double time;
};

namespace bnet {

   class TransportCmd : public dabc::Command {

      DABC_COMMAND(TransportCmd, "TransportCmd");

      TransportCmd(int id, double timeout = - 1.) :
         dabc::Command(CmdName())
      {
         SetInt("CmdId", id);
         if (timeout>0.) SetDouble("CmdTimeout", timeout);
      }

      int GetId() const { return GetInt("CmdId"); }
      double GetTimeout() { return GetDouble("CmdTimeout", 5.); }

      void SetSyncArg(int nrepeat, bool dosync, bool doscale)
      {
         SetInt("NRepeat", nrepeat);
         SetBool("DoSync", dosync);
         SetBool("DoScale", doscale);
      }

      int GetNRepeat() const { return GetInt("NRepeat", 1); }
      bool GetDoSync() const { return GetBool("DoSync", false); }
      bool GetDoScale() const { return GetBool("DoScale", false); }
   };

   enum MasterPacketKind {
      mpk_Null = 0,
      mpk_SubevSizes = 1,    // data with subevents sizes
      mpk_EvSchedule = 2,    // data with event schedule - where event should be build
      mpk_SchedSlot = 3      // definition of base time for next schedules
   };

   struct MasterPacketHeader {
       uint32_t kind;  // kind of data - see MasterPacketKind
       uint32_t len;   // length of the data - including header
       void* rawdata() { return (char*) this + sizeof(MasterPacketHeader); }
   };


   class TransportModule : public dabc::ModuleAsync {
      protected:

      int                 fNodeNumber;
      int                 fNumNodes;
      int                 fNumLids;

      int                 fCmdBufferSize; // size of buffer to send/receive command information
      int                 fCmdDataSize;
      char*               fCmdDataBuffer;

      void*               fCollectBuffer; // buffer to collect results from all nodes
      int                 fCollectBufferSize;

      dabc::Reference     fRunnableRef;  // reference on runnable
      BnetRunnable*       fRunnable;    // runnable where scheduled transfer is implemented
      dabc::PosixThread*  fRunThread;   // special thread where runnable is executed
      EventHandlingRef    fEventHandling;


      /** Name of the file with the schedule */
      std::string         fTestScheduleFile;

      double*            fResults;

      double             fCmdDelay;

      int               *fSendQueue[bnet::MAXLID];    // size of individual sending queue
      int               *fRecvQueue[bnet::MAXLID];    // size of individual receiving queue
      long               fTotalSendQueue;
      long               fTotalRecvQueue;
      long               fTestNumBuffers;

      TimeStamping       fStamping;     // copy of stamping from runnable

      /** array indicating active nodes in the system,
       *  Accumulated in the beginning by the master and distributed to all other nodes.
       *  Should be used in the most tests */
      std::vector<bool>  fActiveNodes;

      /** Schedule, loaded from the file */
      IBSchedule  fPreparedSch;

      /** Send and receive schedule, used in all-to-all tests */
      IBSchedule  fSendSch;
      IBSchedule  fRecvSch;
      bool        fTimeScheduled; // main switch, indicate if all transfer runs according the time or just first - first out


      int               fRunningCmdId; // id of running command, 0 if no command is runs
      dabc::TimeStamp   fCmdStartTime;  // time when command should finish its execution
      dabc::TimeStamp   fCmdEndTime;  // time when command should finish its execution
      std::vector<bool> fCmdReplies; // indicates if cuurent cmd was replied
      dabc::Average     fCmdTestLoop; // average time of test loop
      void*             fCmdAllResults;   // buffer where replies from all nodes collected
      int               fCmdResultsPerNode;  // how many data in replied is expected

      dabc::TimeStamp   fConnStartTime;  // time when connection was starting

      int64_t           fSyncArgs[6];  // arguments for time sync
      int               fSlaveSyncNode;  // current slave node for time sync
      dabc::TimeStamp   fSyncStartTime;  // time when connection was starting

      dabc::CommandsQueue  fCmdsQueue; // commands submitted for execution

      TransportCmd      fCurrentCmd; // currently executed command

      dabc::ModuleItem*  fReplyItem; // item is used to generate events from runnable


      // these all about data transfer ....

      bool                fAllToAllActive; // indicates that all-to-all transfer is running
      int                 fTestBufferSize;
      double              fTestStartTime, fTestStopTime; // start/stop time for data transfer
      uint64_t            fSendTurnCnt, fRecvTurnCnt; // overflow counter of the schedules
      int                 fSendSlotIndx, fRecvSlotIndx;
      bool                fDoSending, fDoReceiving;
      int                 fSendQueueLimit, fRecvQueueLimit; // limits for queue sizes

      dabc::Ratemeter    fWorkRate;
      dabc::Ratemeter    fSendRate;
      dabc::Ratemeter    fRecvRate;

      dabc::CpuStatistic fCpuStat;

      dabc::Average fSendStartTime, fSendComplTime, fRecvComplTime, fOperBackTime;

      int64_t fSkipSendCounter;

      dabc::IntMatrix fSendOperCounter, fRecvOperCounter, fRecvSkipCounter;

      long fNumLostPackets, fTotalRecvPackets, fNumSendPackets, fNumComplSendPackets;

      // this all about events bookkeeping

      // Sender part
      dabc::RecordsQueue<EventPartRec, false>  fEvPartsQueue;
      double         fEventLifeTime;   // time how long event would be preserved in memory
      bool           fIsFirstEventId;  // true if first id was obtained
      bnet::EventId  fFirstEventId;    // id of the first event
      bnet::EventId  fLastEventId;     // id of the last event

      // Receiver part
      dabc::RecordsQueue<EventBundleRec, false>  fEvBundelsQueue;

      // Master part
      dabc::RecordsQueue<EventMasterRec, false>  fEvMasterQueue;

      dabc::RecordsQueue<ScheduleTurnRec, false> fSchTurnsQueue;

      virtual void ProcessInputEvent(dabc::Port* port);
      virtual void ProcessOutputEvent(dabc::Port* port);
      virtual void ProcessTimerEvent(dabc::Timer* timer);
      virtual void ProcessUserEvent(dabc::ModuleItem* item, uint16_t evid);

      void ReleaseReadyEventParts();
      void ReadoutNextEvents(dabc::Port* port);
      EventPartRec* FindEventPartRec(bnet::EventId evid);
      EventPartRec* GetFirstReadyPartRec();

      EventBundleRec* FindEventBundleRec(bnet::EventId evid);
      bool ProvideReceivedBuffer(bnet::EventId evid, int nodeid, dabc::Buffer& buf);
      void BuildReadyEvents(dabc::Port* port);

      void ProcessNextSlaveInputEvent();

      bool RequestMasterCommand(int cmdid, void* cmddata = 0, int cmddatasize = 0, void* allresults = 0, int resultpernode = 0);

      bool ProcessReplyBuffer(int nodeid, dabc::Buffer buf);

      void PrepareSyncLoop(int tgtnode);

      void ActivateAllToAll(double* buff);

      bool SubmitTransfersWithoutSchedule();

      bool ProcessAllToAllAction();

      void PrepareAllToAllResults();

      void ShowAllToAllResults();

      void CompleteRunningCommand(int res = dabc::cmd_true);


      void ProcessSendCompleted(int recid, OperRec* rec);
      void ProcessRecvCompleted(int recid, OperRec* rec);

      bnet::EventId ExtractEventId(const dabc::Buffer& buf);

      // generates schedule turns just basing on precalculated rate
      void AutomaticProduceScheduleTurn(bool for_first_time);


      // ===================================================================================

      int Node() const { return fNodeNumber; }
      int NumNodes() const { return fNumNodes; }
      int NumLids() const { return fNumLids; }
      bool IsSlave() const { return Node() > 0; }
      bool IsMaster() const { return Node()==0; }

      void AllocResults(int size);
      void AllocCollResults(int sz);


      inline int SendQueue(int lid, int node) const { return (lid>=0) && (lid<NumLids()) && (node>=0) && (node<NumNodes()) && fSendQueue[lid] ? fSendQueue[lid][node] : 0; }
      inline int RecvQueue(int lid, int node) const { return (lid>=0) && (lid<NumLids()) && (node>=0) && (node<NumNodes()) && fRecvQueue[lid] ? fRecvQueue[lid][node] : 0; }

      inline long TotalSendQueue() const { return fTotalSendQueue; }
      inline long TotalRecvQueue() const { return fTotalRecvQueue; }
      inline long TestNumBuffers() const { return fTestNumBuffers; }

      bool SubmitToRunnable(int recid, OperRec* rec);

      int PreprocessTransportCommand(dabc::Buffer& buf);

      int ProcessAskQueue(void* tgt);

      bool ProcessCleanup(int32_t* pars);

      public:

      TransportModule(const char* name, dabc::Command cmd);

      virtual ~TransportModule();

      virtual int ExecuteCommand(dabc::Command cmd);

      virtual void BeforeModuleStart();

      virtual void AfterModuleStop();
   };

}

#endif
