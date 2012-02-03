#ifndef IbTestWorkerModule_H
#define IbTestWorkerModule_H

#include "dabc/ModuleSync.h"

#include "dabc/timing.h"

#include "dabc/statistic.h"

#include "IbTestDefines.h"

#include "IbTestSchedule.h"

#ifdef WITH_VERBS
#include "verbs/Context.h"
#include "verbs/ComplQueue.h"
#include "verbs/QueuePair.h"
#include "verbs/MemoryPool.h"
#else
namespace verbs {
   class ComplQueue;
   class QueuePair;
   class MemoryPool;
}
#endif


#pragma pack(1)

struct IbTestConnRec {
   uint32_t lid;
   uint32_t qp;
   uint32_t psn;
};

#pragma pack()

class DoublesVector : public std::vector<double> {
   public:
      DoublesVector() : std::vector<double>() {}

      void Sort();

      double Mean(double max_cut = 1.);
      double Dev(double max_cut = 1.);
      double Max();
      double Min();
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
};

struct ScheduleEntry {
    int    node;
    int    lid;
    double time;
};


class IbTestWorkerModule : public dabc::ModuleSync {

   protected:
      int                 fNodeNumber;
      int                 fNumNodes;
      int                 fNumLids;

      int                 fCmdBufferSize; // size of buffer to send/receive command information
      int                 fCmdDataSize;
      char*               fCmdDataBuffer;

      /** \brief symbolic name of global test which should be performed. Can be:
       *     "TimeSync" - long test of the time synchronisation stability
       *
       *     */
      std::string         fTestKind;

      /** Size of allocated memory pool in MiBytes, used in all-to-all tests */
      long                fTestPoolSize;

      /** Name of the file with the schedule */
      std::string         fTestScheduleFile;

      double* fResults;

      double fCmdDelay;

#ifdef WITH_VERBS

      verbs::ContextRef  fIbContext;

#endif
      verbs::ComplQueue* fCQ;                 //!< completion queue, for a moment single
      verbs::QueuePair** fQPs[IBTEST_MAXLID]; //!< arrays of QueuePairs pointers, NumNodes X NumLids

      verbs::MemoryPool* fPool;         //!< memory pool for tests
      int                fBufferSize;   //!< requested size of buffers in the pool (actual size can be bigger)
      int               *fSendQueue[IBTEST_MAXLID];    // size of individual sending queue
      int               *fRecvQueue[IBTEST_MAXLID];    // size of individual receiving queue
      long               fTotalSendQueue;
      long               fTotalRecvQueue;
      long               fTotalNumBuffers;

      TimeStamping       fStamping;
      double*           fSyncTimes;

      verbs::ComplQueue  *fMultiCQ;     // completion queue of multicast group
      verbs::QueuePair   *fMultiQP;     // connection to multicastgroup
      verbs::MemoryPool  *fMultiPool;   // memory pool of multicast group
      int                fMultiBufferSize;  //!< requested size of buffers in the mcast pool (actual size can be bigger)
      int                fMultiRecvQueueSize; // maximal number of items in multicast recieve queue
      int                fMultiRecvQueue;  // current number of items in multicast recieve queue
      int                fMultiSendQueueSize; // maximal size of send queue
      int                fMultiSendQueue; // current size of send queue
      int                fMultiKind;     // 0 - nothing, 1 - receiver, 10 -sender, 11 - both

      dabc::Ratemeter*   fRecvRatemeter;
      dabc::Ratemeter*   fSendRatemeter;
      dabc::Ratemeter*   fWorkRatemeter;

      double             fTrendingInterval;   //!< interval (in seconds) for send/recv rate trending

      /** array indicating active nodes in the system,
       *  Accumulated in the beginning by the master and distributed to all other nodes.
       *  Should be used in the most tests */
      std::vector<bool>  fActiveNodes;

      /** Schedule, loaded from the file */
      IbTestSchedule  fPreparedSch;

      /** Send and receive schedule, used in all-to-all tests */
      IbTestSchedule  fSendSch;
      IbTestSchedule  fRecvSch;


      int Node() const { return fNodeNumber; }
      int NumNodes() const { return fNumNodes; }
      int NumLids() const { return fNumLids; }
      bool IsSlave() const { return Node() > 0; }
      bool IsMaster() const { return Node()==0; }

      void AllocResults(int size);

      int GetExclusiveIndx(verbs::MemoryPool* pool = 0);
      void* GetPoolBuffer(int indx, verbs::MemoryPool* pool = 0);
      void ReleaseExclusive(int indx, verbs::MemoryPool* pool = 0);

      inline int SendQueue(int lid, int node) const { return (lid>=0) && (lid<NumLids()) && (node>=0) && (node<NumNodes()) && fSendQueue[lid] ? fSendQueue[lid][node] : 0; }
      inline int RecvQueue(int lid, int node) const { return (lid>=0) && (lid<NumLids()) && (node>=0) && (node<NumNodes()) && fRecvQueue[lid] ? fRecvQueue[lid][node] : 0; }
      int NodeSendQueue(int node) const;
      int NodeRecvQueue(int node) const;

      inline long TotalSendQueue() const { return fTotalSendQueue; }
      inline long TotalRecvQueue() const { return fTotalRecvQueue; }
      inline long TotalNumBuffers() const { return fTotalNumBuffers; }

      bool Pool_Post(bool issend, int bufindx, int lid, int nremote, int size = 0);
      bool Pool_Post_Mcast(bool issend, int bufindx, int size = 0);
      verbs::ComplQueue* Pool_CQ_Check(bool &iserror, double waittime);
      int Pool_Check(int &bufindx, int& lid, int &nremote, double waittime, double fasttime = 0.);
      int Pool_Check_Mcast(int &bufindx, double waittime, double fasttime = 0.);
      bool IsMulticastSupported();

      bool CreateQPs(void* data);
      bool ConnectQPs(void* data);
      bool CloseQPs();

      bool SlaveTimeSync(int64_t* cmddata);
      bool CreateCommPool(int64_t* cmddata);

      int PreprocessSlaveCommand(dabc::Buffer& buf);
      bool ExecuteSlaveCommand(int cmdid);
      void SlaveMainLoop();


      bool MasterCollectActiveNodes();

      bool MasterCommandRequest(int cmdid, void* cmddata = 0, int cmddatasize = 0, void* allresults = 0, int resultpernode = 0);

      bool CalibrateCommandsChannel(int nloop = 10);

      bool MasterConnectQPs();

      bool MasterCloseConnections();

      bool MasterCallExit();

      bool MasterCreatePool(int numbuffers, int buffersize);

      bool MasterTestGPU(int bufsize, int testtime, bool testwrite, bool testread);

      bool MasterTimeSync(bool dosynchronisation, int numcycles, bool doscaling = false);

      bool MasterTiming();

      bool ExecuteTestGPU(double* arguments);

      bool ExecuteTiming(double* arguments);

      bool ExecuteAllToAll(double* arguments);

      bool MasterAllToAll(int pattern,
                          int bufsize,
                          double duration,
                          int datarate,
                          int max_sending_queue,
                          int max_recieving_queue,
                          bool fromperfmtest = false);

      bool MasterInitMulticast(int mode,
                               int bufsize = 2048,
                               int send_queue = 5,
                               int recv_queue = 15,
                               int firstnode = -1,
                               int lastnode = -1);

      void ProcessAskQueue();

      void MasterCleanup(int mainnode);
      void ProcessCleanup(int64_t* pars);

      void PerformTestGPU();
      void PerformTimingTest();
      void PerformSingleRouteTest();
      void PerformNormalTest();

   public:
      IbTestWorkerModule(const char* name, dabc::Command cmd);

      virtual ~IbTestWorkerModule();

      virtual int ExecuteCommand(dabc::Command cmd);

      virtual void MainLoop();

      virtual void BeforeModuleStart();

      virtual void AfterModuleStop();
};


#endif
