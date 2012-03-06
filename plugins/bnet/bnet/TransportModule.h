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

   /** Structure to store all relevant information for event buffer
    * Preserved in the queue until data is not transported
    */

   enum EventPartState { eps_Init, eps_Scheduled, eps_Ready };

   struct EventPartRec {
       EventPartState  state;     // actual state of the data
       bnet::EventId   evid;      // eventid
       dabc::Buffer    buf;       // buffer with data
       double          acq_tm;    // time when event was insertred, will be used to remove it

       EventPartRec() : state(eps_Init), evid(0), buf(), acq_tm(0.)  {}

       EventPartRec(const EventPartRec& src) : state(src.state), evid(src.evid), buf(src.buf), acq_tm(src.acq_tm)  {}

       ~EventPartRec() { reset(); }

       void reset() {  state = eps_Init; evid = 0; buf.Release(); acq_tm = 0.; }
   };


   struct EventBundleRec {
      bnet::EventId   evid;      // event identifier
      int             numbuf;    // number of buffers
      dabc::Buffer   *buf;       // array of buffers
      double          acq_tm;    // time when bundle was produced, will be used to remove it

      EventBundleRec() : evid(0), numbuf(0), buf(0), acq_tm(0.) {}

      ~EventBundleRec() { destroy(); }

      EventBundleRec(const EventBundleRec& src) :
         evid(src.evid), acq_tm(src.acq_tm)
      {
         allocate(src.numbuf);
         for (int n=0;n<src.numbuf;n++)
            buf[n] = src.buf[n];
      }

      void allocate(int _numbuf)
      {
         if (_numbuf == numbuf) return;
         destroy();
         if (_numbuf > 0) {
            numbuf = _numbuf;
            buf = new dabc::Buffer[numbuf];
         }
      }

      bool ready()
      {
         for (int n=0;n<numbuf;n++)
            if (buf[n].null()) return false;
         return true;
      }

      void destroy()
      {
         reset();
         if (buf!=0) {
            delete [] buf;
            buf = 0;
         }
         numbuf = 0;
      }

      void reset()
      {
         evid = 0;
         acq_tm = 0;
         for (int n=0;n<numbuf;n++) buf[n].Release();
      }
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


      int fRunningCmdId; // id of running command, 0 if no command is runs
      dabc::TimeStamp fCmdStartTime;  // time when command should finish its execution
      dabc::TimeStamp fCmdEndTime;  // time when command should finish its execution
      std::vector<bool> fCmdReplies; // indicates if cuurent cmd was replied
      dabc::Average fCmdTestLoop; // average time of test loop
      void*  fCmdAllResults;   // buffer where replies from all nodes collected
      int    fCmdResultsPerNode;  // how many data in replied is expected

      dabc::TimeStamp fConnStartTime;  // time when connection was starting

      int64_t         fSyncArgs[6];  // arguments for time sync
      int             fSlaveSyncNode;  // current slave node for time sync
      dabc::TimeStamp fSyncStartTime;  // time when connection was starting

      dabc::CommandsQueue  fCmdsQueue; // commands submitted for execution

      TransportCmd      fCurrentCmd; // currently executed command

      dabc::ModuleItem*  fReplyItem; // item is used to generate events from runnable


      // these all about data transfer ....

      bool                fAllToAllActive; // indicates that all-to-all transfer is running
      int                 fTestBufferSize;
      double              fTestStartTime, fTestStopTime; // start/stop time for data transfer
      int                 fSendSlotIndx, fRecvSlotIndx;
      uint64_t            fSendOverflowCnt, fRecvOverflowCnt;
      double              fSendBaseTime, fRecvBaseTime;
      bool                fDoSending, fDoReceiving;

      dabc::Ratemeter    fWorkRate;
      dabc::Ratemeter    fSendRate;
      dabc::Ratemeter    fRecvRate;

      dabc::CpuStatistic fCpuStat;

      dabc::Average fSendStartTime, fSendComplTime, fRecvComplTime, fOperBackTime;

      int64_t fSkipSendCounter;

      dabc::IntMatrix fSendOperCounter, fRecvOperCounter, fRecvSkipCounter;

      long fNumLostPackets, fTotalRecvPackets, fNumSendPackets, fNumComplSendPackets;

      // this all about events bookkeeping

      dabc::RecordsQueue<EventPartRec, false>  fEvPartsQueue;
      double    fEventLifeTime; // time how long event would be preserved in memory

      dabc::RecordsQueue<EventBundleRec, false>  fEvBundelsQueue;

      virtual void ProcessInputEvent(dabc::Port* port);
      virtual void ProcessOutputEvent(dabc::Port* port);
      virtual void ProcessTimerEvent(dabc::Timer* timer);
      virtual void ProcessUserEvent(dabc::ModuleItem* item, uint16_t evid);

      void ReleaseReadyEventParts();
      void ReadoutNextEvents(dabc::Port* port);
      EventPartRec* FindEventPartRec(bnet::EventId evid);

      EventBundleRec* FindEventBundleRec(bnet::EventId evid);
      void ProvideReceivedBuffer(bnet::EventId evid, int nodeid, dabc::Buffer& buf);
      void BuildReadyEvents(dabc::Port* port);

      void ProcessNextSlaveInputEvent();

      bool RequestMasterCommand(int cmdid, void* cmddata = 0, int cmddatasize = 0, void* allresults = 0, int resultpernode = 0);

      bool ProcessReplyBuffer(int nodeid, dabc::Buffer buf);

      void PrepareSyncLoop(int tgtnode);

      void ActivateAllToAll(double* buff);

      bool ProcessAllToAllAction();

      void PrepareAllToAllResults();

      void ShowAllToAllResults();

      void CompleteRunningCommand(int res = dabc::cmd_true);


      void ProcessSendCompleted(int recid, OperRec* rec);
      void ProcessRecvCompleted(int recid, OperRec* rec);

      bnet::EventId ExtractEventId(const dabc::Buffer& buf);


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
