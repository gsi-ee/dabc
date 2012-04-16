#include "bnet/TransportModule.h"

#include "bnet/defines.h"

#include <vector>
#include <math.h>

#include "dabc/timing.h"
#include "dabc/logging.h"
#include "dabc/Queue.h"
#include "dabc/Manager.h"
#include "dabc/PoolHandle.h"
#include "dabc/Timer.h"
#include "dabc/Port.h"

bnet::TransportModule::TransportModule(const char* name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fStamping(),
   fCmdsQueue(),
   fEvPartsQueue()
{
   fNodeNumber = cmd.Field("NodeNumber").AsInt(0);
   fNumNodes = cmd.Field("NumNodes").AsInt(1);

   fNumLids = Cfg("TestNumLids", cmd).AsInt(1);
   if (fNumLids>bnet::MAXLID) fNumLids = bnet::MAXLID;

   int nports = cmd.Field("NumPorts").AsInt(1);

   fTestNumBuffers = Cfg(dabc::xmlNumBuffers, cmd).AsInt(1000);

   fEventsQueueSize = Cfg("TestEventQueue", cmd).AsInt(100);
   fEventLifeTime = Cfg(names::EventLifeTime(), cmd).AsDouble(2.);

   DOUT0(("Test num buffers = %d", fTestNumBuffers));

   CreatePoolHandle("BnetCtrlPool");

   CreatePoolHandle("BnetDataPool");

   for (int n=0;n<nports;n++) {
//      DOUT0(("Create IOport %d", n));
      CreateIOPort(FORMAT(("Port%d", n)), FindPool("BnetCtrlPool"), 1, 1);
   }

   CreateInput("DataInput", FindPool("BnetDataPool"), 10);

   CreateOutput("DataOutput", FindPool("BnetDataPool"), 10);

   fActiveNodes.resize(NumNodes());
   for (int n=0;n<NumNodes();n++)
      fActiveNodes[n] = true;

   fTestScheduleFile = Cfg("TestSchedule",cmd).AsStdStr();
   if (!fTestScheduleFile.empty())
      fPreparedSch.ReadFromFile(fTestScheduleFile);

   fResults = 0;

   fCmdDelay = 0.;

   for (int n=0;n<bnet::MAXLID;n++) {
      fSendQueue[n] = 0;
      fRecvQueue[n] = 0;
   }
   fTotalSendQueue = 0;
   fTotalRecvQueue = 0;

   for (int lid=0; lid < NumLids(); lid++) {
      fSendQueue[lid] = new int[NumNodes()];
      fRecvQueue[lid] = new int[NumNodes()];
      for (int n=0;n<NumNodes();n++) {
         fSendQueue[lid][n] = 0;
         fRecvQueue[lid][n] = 0;
      }
   }

   fRunnableRef = dabc::mgr.CreateObject("verbs::BnetRunnable", "bnet-run");
   fRunThread = 0;
   fEventHandling = dabc::mgr.CreateObject("TestEventHandling", "bnet-evnt");

   if (fRunnableRef.null()) {
      EOUT(("Cannot create runnable!!!"));
      exit(8);
   }

   if (fEventHandling.null()) {
      EOUT(("Cannot create event handling!!!"));
      exit(9);
   }

   fReplyItem = CreateUserItem("Reply");

   fRunnable = (bnet::BnetRunnable*) fRunnableRef();

   fRunnable->SetNodeId(fNodeNumber, fNumNodes, fNumLids, fReplyItem);

   int numrecs = 1000;

   double delay = Cfg(names::SubmitPreTime()).AsDouble(0.005);
   delay *= 1000 * 20; // in extreme case we need 20 operations per milliseconds
   while ((numrecs < delay) && (numrecs<10000)) numrecs+=500;

   // call Configure before runnable has own thread - no any synchronization problems
   fRunnable->Configure(this, FindPool("BnetDataPool")->Pool(), numrecs);

   fCmdBufferSize = 256*1024;
   fCmdDataBuffer = new char[fCmdBufferSize];

   fCollectBuffer = 0;
   fCollectBufferSize = 0;

   // new members

   fRunningCmdId = 0;
   fCmdEndTime.Reset();  // time when command should finish its execution
   fCmdAllResults = 0;   // buffer where replies from all nodes collected
   fCmdResultsPerNode = 0;  // how many data in replied is expected

   fTestControlKind = 0;
   fDoDataReceiving = false;
   fDoEventBuilding = false;
   fAllToAllActive = false;
   fTestStartTime = 0.;
   fTestStopTime = 0.;

   CreateTimer("Timer", 0.1); // every 100 milisecond

   CreateTimer("AllToAll"); //timer for all-to-all test, shooted individually

   if (IsMaster()) {
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_WAIT, 0.3)); // master wait 0.3 seconds
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_ACTIVENODES)); // first collect active nodes
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_TEST)); // test connection
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_MEASURE)); // reset first measurement

      for (int n=0;n<10;n++) fCmdsQueue.PushD(TransportCmd(BNET_CMD_TEST)); // test connection
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_MEASURE)); // do real measurement of test loop

      fCmdsQueue.PushD(TransportCmd(BNET_CMD_CREATEQP));
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_CONNECTQP));
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_CONNECTDONE));
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_TEST)); // test that slaves are there
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_MEASURE)); // reset single measurement

      fCmdsQueue.PushD(TransportCmd(BNET_CMD_WAIT, 1.)); // master wait 1 second

      TransportCmd cmd_sync1(BNET_CMD_TIMESYNC, 5.);
      cmd_sync1.SetSyncArg(200, true, false);
      fCmdsQueue.PushD(cmd_sync1);
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_EXECSYNC));

      fCmdsQueue.PushD(TransportCmd(BNET_CMD_WAIT, 2.)); // master wait 2 seconds

      TransportCmd cmd_sync2(BNET_CMD_TIMESYNC, 5.);
      cmd_sync2.SetSyncArg(200, true, true);
      fCmdsQueue.PushD(cmd_sync2);
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_EXECSYNC));
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_GETSYNC));

      if (Cfg("TestKind", cmd).AsStdStr() == "TimeSync") {
         fCmdsQueue.PushD(TransportCmd(BNET_CMD_WAIT, 10.)); // master wait 10 seconds
      } else {
         fCmdsQueue.PushD(TransportCmd(BNET_CMD_ALLTOALL)); // configure and start all to all execution

         fCmdsQueue.PushD(TransportCmd(BNET_CMD_WAIT, Cfg("TestTime").AsInt(10) + 3)); // master wait 10 seconds

         fCmdsQueue.PushD(TransportCmd(BNET_CMD_GETRUNRES));
         TransportCmd cmd_coll1(BNET_CMD_COLLECT);
         cmd_coll1.SetInt("SetSize", 14*sizeof(double));
         fCmdsQueue.PushD(cmd_coll1);

         fCmdsQueue.PushD(TransportCmd(BNET_CMD_SHOWRUNRES));

         fCmdsQueue.PushD(TransportCmd(BNET_CMD_ASKQUEUE));
         fCmdsQueue.PushD(TransportCmd(BNET_CMD_CLEANUP));
         fCmdsQueue.PushD(TransportCmd(BNET_CMD_WAIT, 1.)); // master waits 1 second to let all operations run
         fCmdsQueue.PushD(TransportCmd(BNET_CMD_ASKQUEUE));
         fCmdsQueue.PushD(TransportCmd(BNET_CMD_CLEANUP));
      }

      TransportCmd cmd_sync3(BNET_CMD_TIMESYNC, 5.);
      cmd_sync3.SetSyncArg(200, false, false);
      fCmdsQueue.PushD(cmd_sync3);
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_EXECSYNC));

      fCmdsQueue.PushD(TransportCmd(BNET_CMD_CLOSEQP)); // close connections
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_TEST)); // test that slaves are there

      fCmdsQueue.PushD(TransportCmd(BNET_CMD_EXIT)); // master wait 1 seconds
   }
}

bnet::TransportModule::~TransportModule()
{
   DOUT2(("Calling ~TransportModule destructor name:%s", GetName()));

   delete fRunThread; fRunThread = 0;
   fRunnableRef.Release();
   fRunnable = 0;

   if (fCmdDataBuffer) {
      delete [] fCmdDataBuffer;
      fCmdDataBuffer = 0;
   }

   for (int n=0;n<bnet::MAXLID;n++) {
      if (fSendQueue[n]) delete [] fSendQueue[n];
      if (fRecvQueue[n]) delete [] fRecvQueue[n];
   }

   AllocCollResults(0);

   AllocResults(0);
}

void bnet::TransportModule::AllocCollResults(int sz)
{
   if ((sz>fCollectBufferSize) || (sz==0)) {
      free(fCollectBuffer);
      fCollectBuffer = 0;
      fCollectBufferSize = 0;
   }

   if (sz>fCollectBufferSize) {
      fCollectBuffer = malloc(sz);
      fCollectBufferSize = sz;
   }

   if (fCollectBufferSize>0)
      memset(fCollectBuffer, 0, fCollectBufferSize);
}

void bnet::TransportModule::AllocResults(int size)
{
   if (fResults!=0) {
      delete[] fResults;
      fResults = 0;
   }

   if (size>0) {
      fResults = new double[size];
      for (int n=0;n<size;n++) fResults[n] = 0.;
   }
}

void bnet::TransportModule::BeforeModuleStart()
{
   DOUT2(("IbTestWorkerModule starting"));

   int special_thrd = Cfg("SpecialThread").AsBool(false) ? 0 : -1;

   fRunThread = new dabc::PosixThread(special_thrd); // create thread with specially-allocated CPU

   fRunThread->Start(fRunnable);

   // set threads id to be able check correctness of calling
   fRunnable->SetThreadsIds(dabc::PosixThread::Self(), fRunThread->Id());
}

void bnet::TransportModule::AfterModuleStop()
{
   DOUT0(("bnet::TransportModule finished"));
}

bool bnet::TransportModule::SubmitToRunnable(int recid, OperRec* rec)
{
   if ((rec->tgtnode>=NumNodes()) || (rec->tgtindx>=NumLids())) {
      EOUT(("Wrong record tgt lid:%d node:%d", rec->tgtindx, rec->tgtnode));
      exit(6);
   }

   switch (rec->kind) {
      case kind_Send:
         fSendQueue[rec->tgtindx][rec->tgtnode]++;
         fTotalSendQueue++;
         break;
      case kind_Recv:
         fRecvQueue[rec->tgtindx][rec->tgtnode]++;
         fTotalRecvQueue++;
         break;
      default:
         EOUT(("Wrong operation kind"));
         break;
   }


   if (!fRunnable->SubmitRec(recid)) {
      EOUT(("Cannot submit operation to lid %d node %d", rec->tgtindx, rec->tgtnode));
      return false;
   }

   return true;
}

int bnet::TransportModule::PreprocessTransportCommand(dabc::Buffer& buf)
{
   if (buf.null()) return BNET_CMD_EXIT;

   dabc::Pointer ptr(buf);
   CommandMessage msg;

   ptr.copyto(&msg, sizeof(CommandMessage));

   if (msg.magic!=BNET_CMD_MAGIC) {
       EOUT(("Magic id is not correct"));
       return BNET_CMD_EXIT;
   }

   int cmdid = msg.cmdid;

   fCmdDataSize = msg.cmddatasize;
   if (fCmdDataSize > fCmdBufferSize) {
      EOUT(("command data too big for buffer!!!"));
      fCmdDataSize = fCmdBufferSize;
   }

   ptr.shift(sizeof(CommandMessage));
   ptr.copyto(fCmdDataBuffer, fCmdDataSize);

   msg.node = dabc::mgr.NodeId();
   msg.cmddatasize = 0;
   int sendpacketsize = sizeof(CommandMessage);

   bool cmd_res(false);

   switch (cmdid) {
      case BNET_CMD_TEST:
         cmd_res = true;
         break;

      case BNET_CMD_EXIT:
         WorkerSleep(0.1);
         DOUT2(("Propose worker to stop"));
         // Stop(); // stop module

         fRunnable->StopRunnable();
         fRunThread->Join();

         dabc::mgr.StopApplication();
         cmd_res = true;
         break;

      case BNET_CMD_ACTIVENODES: {
         uint8_t* buff = (uint8_t*) fCmdDataBuffer;

         if (fCmdDataSize!=NumNodes()) {
            EOUT(("Argument size mismatch"));
            cmd_res = false;
         }

         fActiveNodes.clear();
         for (int n=0;n<fCmdDataSize;n++)
            fActiveNodes.push_back(buff[n]>0);

         cmd_res = fRunnable->SetActiveNodes(fCmdDataBuffer, fCmdDataSize);

         break;
      }

      case BNET_CMD_COLLECT:
         if ((msg.getresults > 0) && (fResults!=0)) {
            msg.cmddatasize = msg.getresults;
            sendpacketsize += msg.cmddatasize;
            ptr.copyfrom(fResults, msg.getresults);
            cmd_res = true;
         }
         break;

      case BNET_CMD_CREATEQP: {
         int ressize = msg.getresults;
         cmd_res = fRunnable->CreateQPs(fCmdDataBuffer, ressize);
         ptr.copyfrom(fCmdDataBuffer, ressize);
         msg.cmddatasize = ressize;
         sendpacketsize += ressize;
         break;
      }

      case BNET_CMD_CONNECTQP:
         cmd_res = fRunnable->ConnectQPs(fCmdDataBuffer, fCmdDataSize);
         break;

      case BNET_CMD_CLEANUP:
         cmd_res = ProcessCleanup((int32_t*)fCmdDataBuffer);
         break;

      case BNET_CMD_CLOSEQP:
         cmd_res = fRunnable->CloseQPs();
         break;

      case BNET_CMD_ASKQUEUE:
         msg.cmddatasize = ProcessAskQueue(fCmdDataBuffer);
         ptr.copyfrom(fCmdDataBuffer, msg.cmddatasize);
         sendpacketsize += msg.cmddatasize;
         cmd_res = true;
         break;

      case BNET_CMD_TIMESYNC:
         // here we just copy arguments for sync command
         if (sizeof(fSyncArgs) != fCmdDataSize) {
            EOUT(("time sync buffer not match!!!"));
            exit(5);
         }
         memcpy(fSyncArgs, fCmdDataBuffer, sizeof(fSyncArgs));
         cmd_res = true;
         break;

      case BNET_CMD_GETSYNC:
         fRunnable->GetSync(fStamping);
         cmd_res = true;
         break;

      case BNET_CMD_ALLTOALL:
         ActivateAllToAll((double*)fCmdDataBuffer);
         cmd_res = true;
         break;

      case BNET_CMD_GETRUNRES:
         PrepareAllToAllResults();
         cmd_res = true;
         break;

      default:
         cmd_res = true;
         break;
   }

   // copy header back to the buffer
   ptr = buf;
   ptr.copyfrom(&msg, sizeof(CommandMessage));

   buf.SetTotalSize(sendpacketsize);

   if (!cmd_res) EOUT(("Command %d failed", cmdid));

   return cmdid;
}

void bnet::TransportModule::ActivateAllToAll(double* arguments)
{
   dabc::SetFileLevel(0);

   fTestBufferSize = arguments[0];
//   int NumUsedNodes = arguments[1];
   fSendQueueLimit = arguments[2];
   fRecvQueueLimit = arguments[3];
   fTestStartTime = arguments[4];
   fTestStopTime = arguments[5];
   int patternid = arguments[6];
   double datarate = arguments[7];

   fTestControlKind = (int) arguments[10];

   fOperPreTime = arguments[11];
   int startturns = (int) arguments[12];


   fDoDataReceiving = (Node() > 0) || !haveController();
   fDoEventBuilding = (Node() > 0) || !haveController();


   // double ratemeterinterval = arguments[8];
   // bool canskipoperation = arguments[9] > 0;

   DOUT2(("ActivateAllToAll - start"));

   fSendRate.Reset();
   fRecvRate.Reset();
   fWorkRate.Reset();

   fTimeScheduled = datarate > 0;
   if (!fTimeScheduled) datarate = 1.; // just avoid math errors, time in schedule will be ingored anyway

   // schstep is in seconds, but datarate in MBytes/sec, therefore 1e-6
   double schstep = 1e-6 * fTestBufferSize / datarate;

   switch(patternid) {
      case 1: // send data to next node
         fSendSch.Allocate(1, NumNodes());
         for (int n=0;n<NumNodes();n++)
            fSendSch.Item(0,n).Set((n+1) % NumNodes(), n % NumLids());
         fSendSch.SetEndTime(schstep); // this is end of the schedule
         break;

      case 2: // send always with shift num/2
         fSendSch.Allocate(1, NumNodes());
         for (int n=0;n<NumNodes();n++)
            fSendSch.Item(0,n).Set((n+NumNodes()/2) % NumNodes(), n % NumLids());
         fSendSch.SetEndTime(schstep); // this is end of the schedule
         break;

      case 3: // one to all
         fSendSch.Allocate(NumNodes()-1, NumNodes());
         for (int nslot=0; nslot<NumNodes()-1;nslot++) {
            fSendSch.Item(nslot,NumNodes()-1).Set(nslot, nslot % NumLids());
            fSendSch.SetTimeSlot(nslot, schstep*nslot);
         }

         fSendSch.SetEndTime(schstep*(NumNodes()-1)); // this is end of the schedule
         break;

      case 4:    // all-to-one
         fSendSch.Allocate(NumNodes()-1, NumNodes());
         for (int nslot=0; nslot<NumNodes()-1;nslot++) {
            fSendSch.Item(nslot,nslot).Set(NumNodes()-1, nslot % NumLids());
            fSendSch.SetTimeSlot(nslot, schstep*nslot);
         }

         fSendSch.SetEndTime(schstep*(NumNodes()-1)); // this is end of the schedule
         break;

      case 5: // even nodes sending data to odd nodes
         fSendSch.Allocate(1, NumNodes());
         for (int n=0;n<NumNodes();n++)
            if ((n % 2 == 0) && (n<NumNodes()-1))
               fSendSch.Item(0,n).Set(n+1, n % NumLids());

         fSendSch.SetEndTime(schstep); // this is end of the schedule
         break;

      case 7:  // all-to-all from file
         if ((fPreparedSch.numSenders() == NumNodes()) && (fPreparedSch.numLids() <= NumLids())) {
            fPreparedSch.CopyTo(fSendSch);
            fSendSch.FillRegularTime(schstep / fPreparedSch.numSlots() * (NumNodes()-1));
         } else {
            fSendSch.Allocate(NumNodes()-1, NumNodes());
            fSendSch.FillRoundRoubin(0, schstep);
         }
         break;


      default:
         // default, all-to-all round-robin schedule
         fSendSch.Allocate(NumNodes(), NumNodes());
         fSendSch.FillRoundRoubin(0, schstep, true);
         break;
   }

   DOUT4(("ActiavteAllToAll: Define schedule"));

   fSendSch.FillReceiveSchedule(fRecvSch);

   for (unsigned node=0; node<fActiveNodes.size(); node++)
      if (!fActiveNodes[node]) {
         fSendSch.ExcludeInactiveNode(node);
         fRecvSch.ExcludeInactiveNode(node);
      }

   if (patternid==6) // disable receive operation 1->0 to check that rest data will be transformed
      for (int nslot=0; nslot<fRecvSch.numSlots(); nslot++)
         if (fRecvSch.Item(nslot, 0).node == 1) {
            DOUT0(("Disable recv operation 1->0"));
            fRecvSch.Item(nslot, 0).Reset();
          }

   fSendTurnRec = 0; fSendTurnCnt = 0; fSendSlotIndx = -2; fSendTurnEndTime = fTestStartTime;

   fSendStartTime.Reset();
   fSendComplTime.Reset();
   fRecvComplTime.Reset();
   fOperBackTime.Reset();

   DOUT2(("ActiavteAllToAll: remains: %5.3fs", fTestStartTime - fStamping()));

   // counter for must have send operations over each channel
   fSendOperCounter.SetSize(NumLids(), NumNodes());

   // actual received id from sender
   fRecvOperCounter.SetSize(NumLids(), NumNodes());

   // number of recv skip operations one should do
   fRecvSkipCounter.SetSize(NumLids(), NumNodes());

   fSendOperCounter.Fill(0);
   fRecvOperCounter.Fill(-1);
   fRecvSkipCounter.Fill(0);

   fNumLostPackets = 0;
   fTotalRecvPackets = 0;
   fNumSendPackets = 0;
   fNumComplSendPackets = 0;

   fSkipSendCounter = 0;


   // ===================== allocation of events queue ==============

   // sender part
   fEvPartsQueue.Allocate(fEventsQueueSize);

   fIsFirstEventId = false;
   fFirstEventId = 0;

   // FIXME: Is 10000 slots is enough???
   fSchTurnsQueue.Allocate(10000 / NumNodes());
   fSchTurnsQueue.AllocateRecs(NumNodes()); // each turn include event assignment for each nodes, including controller
   fLastTurnId = 0;

   if (!haveController()) {
      AutomaticProduceScheduleTurn(true);
      startturns = 0;
   } else {
      if (startturns<1) EOUT(("No any start turns specified in control mode!!"));
      if (startturns > (int) fSchTurnsQueue.Capacity()) {
         EOUT(("Starting turns %d exceed capacity %u of correspondent queue", startturns, fSchTurnsQueue.Capacity()));
         startturns  = (int) fSchTurnsQueue.Capacity();
      }
   }


   // receiver part
   int numfullevents = fEventsQueueSize / NumNodes();
   if (numfullevents<5) numfullevents = 5;
   fEvBundelsQueue.Allocate(numfullevents);
   fEvBundelsQueue.AllocateRecs(NumWorkers());
   fLastBuildEventId.SetNull();
   fLastBuildEventTime = 0.;


   // controller part
   if (IsController()) {
      fEvMasterQueue.SetStrictOrder(true); // TODO: make configurable, extend on other queues
      fEvMasterQueue.Allocate(fEventsQueueSize*2);
      fEvMasterQueue.AllocateRecs(NumNodes());

      // we suppose that start runs generated by each worker individually
      fNodesInfo.resize(NumNodes());
      for (unsigned n=0;n<fNodesInfo.size(); n++) {
         fNodesInfo[n].last_send_turnid = (uint64_t) startturns;
      }

      fMasterSkipEventId.SetNull();
      fMasterSkipTurnId = 0;
   }


   fRunnable->ConfigQueueLimits(fSendQueueLimit, fRecvQueueLimit);

   fRunnable->ResetStatistic();

   fAllToAllActive = true;

   ShootTimer("AllToAll", 0.001);

   DOUT0(("Activate ALL-to-ALL"));

   fLastTurnTime = fTestStartTime;
   fLastTurnId = 0;

   for (int cnt=0; cnt<startturns; cnt++) {
      if (fSchTurnsQueue.Full()) {
         EOUT(("Problem - no place in the turns queue - why!!!"));
         exit(8);
      }
      ScheduleTurnRec* rec = fSchTurnsQueue.PushEmpty();

      rec->fTurn = ++fLastTurnId;
      rec->starttime = fLastTurnTime;
      fLastTurnTime += fSendSch.endTime();

      rec->FillEmptyTurn();
   }

   DOUT1(("Staring with %u turns in the queue", (unsigned) fSchTurnsQueue.Size()));

   // submitting some buffers into receive pool of runnable

   fRefillCounter = 0;
   int nrecvpool = 100;

   DOUT0(("Fill %d bufferes in runnable pool", nrecvpool));

   for (int n=0;n<nrecvpool;n++) {
      dabc::Buffer buf = FindPool("BnetDataPool")->TakeBuffer(fTestBufferSize);

      if (buf.null()) {
         EOUT(("Not enough buffers"));
         exit(18);
      }

      int recid = fRunnable->PrepareOperation(kind_Recv, sizeof(TransportHeader), buf);
      OperRec* rec = fRunnable->GetRec(recid);

      if (rec==0) {
         EOUT(("Cannot prepare recv pool operation recid = %d", recid));
         exit(19);
      }

      rec->skind = skind_Pool;
      rec->SetTarget(0, 0);
      rec->SetImmediateTime();

      if (!SubmitToRunnable(recid,rec)) {
         EOUT(("Cannot submit recv operation to pool"));
         exit(20);
      }
   }

   // now fill recv queues according to recv schedule
   for (int indx=0;indx<fRecvSch.numSlots();indx++)
      for (int nqueue=0;nqueue<fRecvQueueLimit;nqueue++) {

         // no need to fill recv queue for itself
         if (fRecvSch.Item(indx, Node()).node == Node()) continue;

         dabc::Buffer buf = FindPool("BnetDataPool")->TakeBuffer(fTestBufferSize);

         if (buf.null()) {
            EOUT(("Not enough buffers"));
            exit(18);
         }

         int recid = fRunnable->PrepareOperation(kind_Recv, sizeof(TransportHeader), buf);
         OperRec* rec = fRunnable->GetRec(recid);

         if (rec==0) {
            EOUT(("Cannot prepare recv pool operation recid = %d", recid));
            exit(19);
         }

         rec->skind = skind_Refill; // indicate that ready operation should be replaced as soon as possible
         rec->SetTarget(fRecvSch.Item(indx, Node()).node, fRecvSch.Item(indx, Node()).lid);
         rec->SetImmediateTime();

         if (!SubmitToRunnable(recid,rec)) {
            EOUT(("Cannot submit recv operation to pool"));
            exit(20);
         }
      }

   fWorkerCtrlEvId.SetNull();
}


void bnet::TransportModule::AutomaticProduceScheduleTurn(bool for_first_time)
{
   while ((fSendTurnCnt>0) && !fSchTurnsQueue.Empty()) {
      if (fSchTurnsQueue.Front().id() < fSendTurnCnt)
         fSchTurnsQueue.PopOnly();
      else
         break;
   }

   if (fSchTurnsQueue.Full()) return;

   unsigned nextid = 1; // start turn id producing from 1
   double nexttm = fTestStartTime;

   if (!fSchTurnsQueue.Empty()) {
      nextid = fSchTurnsQueue.Back().id() + 1;
      nexttm = fSchTurnsQueue.Back().starttime + fSendSch.endTime();
   } else {
      if (!for_first_time) {
         // DOUT0(("Do not fill turn queue - it was emptied intentionally"));
         return;
      }
   }

   while (!fSchTurnsQueue.Full() && (nexttm<fTestStopTime)) {
      ScheduleTurnRec* rec = fSchTurnsQueue.AddNewTurn(nextid, nexttm);

      for (int n=0;n<NumNodes();n++)
         rec->fVector[n] = 1 + (nextid-1)*NumNodes() + n;

      nextid++;
      nexttm += fSendSch.endTime();
   }
}

void bnet::TransportModule::PrepareAllToAllResults()
{
   AllocResults(14);

   fAllToAllActive = false;

   fCpuStat.Measure();

   double r_mean_loop(0.), r_max_loop(0.);
   int r_long_cnt(0);
   fRunnable->GetStatistic(r_mean_loop, r_max_loop, r_long_cnt);

   DOUT0(("Mean loop %11.9f max %8.6f Longer loops %d", r_mean_loop, r_max_loop, r_long_cnt));

   DOUT3(("Total recv queue %ld send queue %ld", TotalSendQueue(), TotalRecvQueue()));

   if (fSendSlotIndx>=0)
   DOUT3(("Sending curr_tm %8.7f next_tm %8.7f slot %d node %d lid %d",
         curr_tm, fSendSch.timeSlot(fSendSlotIndx), fSendSlotIndx,
         fSendSch.Item(fSendSlotIndx, Node()).node, fSendSch.Item(fSendSlotIndx, Node()).lid));

   DOUT3(("Send operations diff %ld: submited %ld, completed %ld", fNumSendPackets-fNumComplSendPackets, fNumSendPackets, fNumComplSendPackets));

   DOUT5(("CPU utilization = %5.1f % ", fCpuStat.CPUutil()*100.));

   DOUT0(("Send start time min:%8.6f aver:%8.6f max:%8.6f cnt:%ld", fSendStartTime.Min(), fSendStartTime.Mean(), fSendStartTime.Max(), fSendStartTime.Number()));

   if (fNumSendPackets!=fNumComplSendPackets) {
      EOUT(("Mismatch %d between submitted %ld and completed send operations %ld",fNumSendPackets-fNumComplSendPackets, fNumSendPackets, fNumComplSendPackets));

      for (int lid=0;lid<NumLids(); lid++)
        for (int node=0;node<NumNodes();node++) {
          if ((node==Node()) || !fActiveNodes[node]) continue;
          DOUT5(("   Lid:%d Node:%d SendQueue = %d recvskp %d", lid, node, SendQueue(lid,node), fRecvSkipCounter(lid, node)));
       }
   }

   fResults[0] = fSendRate.GetRate();
   fResults[1] = fRecvRate.GetRate();
   fResults[2] = fSendStartTime.Mean();
   fResults[3] = fSendComplTime.Mean();
   fResults[4] = fNumLostPackets;
   fResults[5] = fTotalRecvPackets;
   fResults[6] = 0;
   fResults[7] = 0;
   fResults[8] = 0.;
   fResults[9] = fCpuStat.CPUutil();
   fResults[10] = r_mean_loop; // loop_time.Mean();
   fResults[11] = r_max_loop; // loop_time.Max();
   fResults[12] = (fNumSendPackets+fSkipSendCounter) > 0 ? 1.*fSkipSendCounter / (fNumSendPackets + fSkipSendCounter) : 0.;
   fResults[13] = fRecvComplTime.Mean();
}

void bnet::TransportModule::ShowAllToAllResults()
{
   int setsize = 14;

   if (fCollectBufferSize < (int) sizeof(double)*NumNodes()*setsize) {
      EOUT(("Collected results size too small !!!"));
      return;
   }

   double* allres = (double*) fCollectBuffer;

   DOUT0(("  # |      Node |   Send |   Recv |   Start |S_Compl|R_Compl| Lost | Skip | Loop | Max ms | CPU"));
   double sum1send(0.), sum2recv(0.), sum3multi(0.), sum4cpu(0.);
   int sumcnt = 0;
   // bool isok(true), isskipok(true);
   double totallost(0), totalrecv(0), totalskipped(0);

   char sbuf1[100], cpuinfobuf[100];

   for (int n=0;n<NumNodes();n++) {
       if (allres[n*setsize+12]>=0.099)
          sprintf(sbuf1,"%4.0f%s",allres[n*setsize+12]*100.,"%");
       else
          sprintf(sbuf1,"%4.1f%s",allres[n*setsize+12]*100.,"%");

       if (allres[n*setsize+9]>=0.099)
          sprintf(cpuinfobuf,"%4.0f%s",allres[n*setsize+9]*100.,"%");
       else
          sprintf(cpuinfobuf,"%4.1f%s",allres[n*setsize+9]*100.,"%");

       DOUT0(("%3d |%10s |%7.1f |%7.1f |%8.1f |%6.0f |%6.0f |%5.0f |%s |%5.2f |%7.2f |%s",
             n, dabc::mgr()->GetNodeName(n).c_str(),
           allres[n*setsize+0], allres[n*setsize+1], fTimeScheduled ? allres[n*setsize+2]*1e6 : 0., allres[n*setsize+3]*1e6, allres[n*setsize+13]*1e6, allres[n*setsize+4], sbuf1, allres[n*setsize+10]*1e6, allres[n*setsize+11]*1e3, cpuinfobuf));
       totallost += allres[n*setsize+4];
       totalrecv += allres[n*setsize+5];

       // check if send starts in time
       // if (allres[n*setsize+2] > 20.) isok = false;

       sumcnt++;
       sum1send += allres[n*setsize+0];
       sum2recv += allres[n*setsize+1];
       sum3multi += allres[n*setsize+8]; // Multi rate
       sum4cpu += allres[n*setsize+9]; // CPU util
       totalskipped += allres[n*setsize+12];
       // if (allres[n*setsize+12]>0.03) isskipok = false;
   }
   DOUT0(("          Total |%7.1f |%7.1f | Skip: %4.0f/%4.0f = %3.1f percent",
          sum1send, sum2recv, totallost, totalrecv, 100*(totalrecv>0. ? totallost/(totalrecv+totallost) : 0.)));
}

bool bnet::TransportModule::SubmitTransfersWithoutSchedule()
{
   // this is method of submitting transfers in simplest way without any synchronisation and schedule
   // just send data as it comes

   // first fill receive queues for all connections specified

   dabc::TimeStamp starttm = dabc::Now();

   while (fRefillCounter>0) {
      // break execution if it take too long
      if (starttm.Expired(0.05)) return true;

      dabc::Buffer buf = FindPool("BnetDataPool")->TakeBuffer(fTestBufferSize);

      if (buf.null()) {
         EOUT(("Not enough buffers"));
         return false;
      }

      // DOUT0(("Submit recv operation for buffer %u", buf.GetTotalSize()));

      int recid = fRunnable->PrepareOperation(kind_Recv, sizeof(TransportHeader), buf);
      OperRec* rec = fRunnable->GetRec(recid);

      if (rec==0) {
          EOUT(("Cannot prepare recv operation recid = %d", recid));
          return false;
      }

      rec->skind = skind_Pool;
      rec->SetTarget(0, 0);
      rec->SetImmediateTime();

      if (!SubmitToRunnable(recid,rec)) {
         EOUT(("Cannot submit recv pool operation"));
         return false;
      }

      fRefillCounter--;
   }

   while (!starttm.Expired(0.05)) {
      bnet::EventPartRec* evrec = GetFirstReadyPartRec();

      if (evrec==0) break;

      int tgtnode = evrec->evid % NumNodes();

      if (tgtnode == Node()) {
         if (ProvideReceivedBuffer(evrec->evid, Node(), evrec->buf))
            evrec->state = eps_Ready;
         else
            break; // if we cannot provide buffer wait - do not continue
      } else {
         int nslot = fSendSch.FindSlotWithConnection(Node(), tgtnode);

         if (nslot<0) {
            EOUT(("Did not find connection %d -> %d",Node(),tgtnode));
            return false;
         }

         int tgtlid = fSendSch.Item(nslot, Node()).lid;

         // do not submit too many operations over single link

         // TODO: should we limit by any means queue in such unscheduled mode
         // should it be done in the runnable ??

         if (SendQueue(tgtlid,tgtnode) >= 2*fSendQueueLimit) break;

         int recid = fRunnable->PrepareOperation(kind_Send, sizeof(TransportHeader), evrec->buf.Duplicate());
         OperRec* rec = fRunnable->GetRec(recid);

         if (rec==0) {
            EOUT(("Cannot prepare send operation recid = %d", recid));
            return false;
         }

         TransportHeader* hdr = (TransportHeader*) rec->header;
         hdr->srcnode   = Node();         // source node id
         hdr->tgtnode   = tgtnode;        // target node id
         hdr->evid      = evrec->evid;    // event id
         hdr->send_tm   = fStamping();    // time
         hdr->seqid     = fSendOperCounter(tgtlid,tgtnode)++; // sequence number
         hdr->sendkind  = 1;

         rec->SetTarget(tgtnode, tgtlid);
         rec->SetImmediateTime();

         if (!SubmitToRunnable(recid, rec)) {
            EOUT(("Cannot submit send operation to lid %d node %d", tgtlid, tgtnode));
            return false;
         }

         DOUT2(("SubmitSendOper recid %d node %d lid %d stamp %7.6f", recid, tgtnode, tgtlid, fStamping()));

         evrec->state = eps_Scheduled;

         fNumSendPackets++;
      }
   }

   return true;
}

void bnet::TransportModule::AnalyzeControllerData()
{
   // here one should analyze events statistic from the nodes,
   // produce schedule turns, skip very old events

   double curr_tm = fStamping();

   int numready = 0;

   std::vector<EventMasterRec*> ready; //(NumWorkers(), NULL);
   ready.resize(NumWorkers(), 0);

   uint64_t lastturnproduced(0);

   for (EventsMasterQueue::Iterator iter = fEvMasterQueue.begin();
         iter != fEvMasterQueue.end(); iter++) {

      if (iter()->complete() && (iter()->tgtnode<0))
         ready[numready++] = iter();

      if (numready < NumWorkers()) continue;

      if (fSchTurnsQueue.Full()) break;

      // do not continue work if running over test time
      if ((fTestStopTime>0.) && (fLastTurnTime > fTestStopTime - 0.1)) break;

      ScheduleTurnRec* turn = fSchTurnsQueue.AddNewTurn(++fLastTurnId, fLastTurnTime);

      fLastTurnTime += fSendSch.endTime();

      lastturnproduced = fLastTurnId;

      DOUT2(("Produce normal turn %u at time %7.6f", (unsigned) turn->fTurn, turn->starttime ));

      // set controller id to 0, will be automatically replaced
      turn->fVector[0].SetNull();

      for (int num=0;num<NumWorkers();num++) {
         // assign events to the workers
         turn->fVector[num+1] = ready[num]->evid;
         // mark event as assigned
         ready[num]->tgtnode = num+1;

         DOUT2(("Schedule event %u to node %d at turn %u", (unsigned) ready[num]->evid, num+1, (unsigned) turn->fTurn ));

         ready[num] = 0;
      }

      numready = 0;
   }

   if ((fTestStopTime>0.) && (fLastTurnTime > fTestStopTime - 0.1)) {
      // at the end of the test produce only dummy schedules
      while ((fLastTurnTime < fTestStopTime) && !fSchTurnsQueue.Full()) {
         fSchTurnsQueue.AddNewTurn(++fLastTurnId, fLastTurnTime, true);
         fLastTurnTime += fSendSch.endTime();
         lastturnproduced = fLastTurnId;
      }
   } else {

      // if there is very short time until end of last turn,
      // we need to produce several dummy turns to be able continue communication
      // TODO: how to define minimal time - one should include operation preparation time which is about 5 ms

      while (curr_tm + 2*fSendSch.endTime() > fLastTurnTime) {
         // produce empty turn as soon as possible

         if (fSchTurnsQueue.Full()) {
             EOUT(("Problem - no place in the turns queue - why!!!"));
             exit(8);
         }

         DOUT1(("Generate additional dummy turn %u", (unsigned) fLastTurnId));

         fSchTurnsQueue.AddNewTurn(++fLastTurnId, fLastTurnTime, true);

         fLastTurnTime += fSendSch.endTime();

         lastturnproduced = fLastTurnId;
      }
   }

   if (lastturnproduced) {
      DOUT1(("PRODUCING NEW TURNS bis %u took %7.6f s", (unsigned) lastturnproduced, fStamping() - curr_tm));

      // fSchTurnsQueue.PrintAll();
   }




   // analyze which events can be thrown away

   bnet::EventId minbuild;
   minbuild.SetNull();

   for (int nodeid=1; nodeid<NumNodes(); nodeid++) {
      if (fNodesInfo[nodeid].last_build_id.null()) {
         minbuild.SetNull(); // this indicates that information is not complete
         break;
      }

      if (minbuild.null() || (fNodesInfo[nodeid].last_build_id < minbuild))
         minbuild = fNodesInfo[nodeid].last_build_id;
   }

   bool newboundary = !minbuild.null() && (fMasterSkipEventId!=minbuild);

   fMasterSkipEventId = minbuild;

   if (newboundary) {
      fEvMasterQueue.SkipEventsUntil(fMasterSkipEventId);
      DOUT1(("EVENTS SKIP UNTIL %u queue %u", (unsigned) fMasterSkipEventId, fEvMasterQueue.Size()));
   }

   // TODO: more sophisticated way to find turn id which could be skipped
   if (fSendTurnCnt>5)
      fMasterSkipTurnId = fSendTurnCnt - 2;

   if (fMasterSkipTurnId>0)
      fSchTurnsQueue.RemoveTurnsUntil(fMasterSkipTurnId);
}


bool bnet::TransportModule::ProcessAllToAllAction()
{
   if (!fAllToAllActive) return false;

   // TODO: is all this calculations require additional module/thread,
   // were runs parallel to normal traffic preparation work

   if (doEventBuilding())
      BuildReadyEvents(FindPort("DataOutput"));

   if (doDataReceiving())
      ReadoutNextEvents(FindPort("DataInput"));

   if (!fTimeScheduled)
      return SubmitTransfersWithoutSchedule();

   if (!haveController())
      AutomaticProduceScheduleTurn(false);

   // TODO: call it once per turn or per regular time intervall
   if (IsController())
      AnalyzeControllerData();

   double start_tm = 0;

   // FIXME: maxcnt should be removed - can be dangerous for small buffers
   int maxcnt(200);
   int send_node(0), send_lid(0);

   bnet::EventId send_evid;

   static dabc::Average loop_time;
   double last_tm(0.);

   bool did_operation(false);

   // TODO: loop condition must be different
   do {

      did_operation = false;

      double next_send_time(0.);

      // if (maxcnt<199) DOUT1(("<<<<<<<<<<<<<<<< EXEC ProcessAllToAllLoop %d", maxcnt));

      double curr_tm = fStamping();
      if (start_tm==0.) start_tm = curr_tm;
      if (last_tm>0) loop_time.Fill(curr_tm - last_tm);
      last_tm = curr_tm;

      if (curr_tm > fTestStopTime) {
         fAllToAllActive = false;
         fRunnable->StopRefilling();
         DOUT0(("All-to-all processing done"));
         dabc::SetFileLevel(1);
         fSendStartTime.Show("SendStartTime", true);
         fOperBackTime.Show("OperBackTime", true);
         return false;
      }

      if (curr_tm > start_tm + 0.03) {
         EOUT(("Too long in the loop"));
         return true;
      }

      if (fSendSlotIndx == -2) {
         if ((fTestStopTime > 0.) && (fSendTurnEndTime > fTestStopTime)) {
            fSendTurnRec = 0; // end of the work for sender
            fSendSlotIndx = -3; //
         } else {
            fSendTurnRec = fSchTurnsQueue.FindItemWithId(fSendTurnCnt+1);

            if (fSendTurnRec!=0) {
               fSendTurnCnt++;
               DOUT1(("Switch to the send turn %u", (unsigned) fSendTurnCnt));
               fSendSlotIndx = -1;
               fSendSch.ShiftToNextOperation(Node(), fSendSlotIndx);
               fSendTurnEndTime = fSendTurnRec->starttime + fSendSch.endTime();
            } else {
               EOUT(("Problem - no schedule when it is required. Not yet handled - abort"));
               exit(11);
            }
         }
      }

      if ((fSendSlotIndx>=0) && fSendTurnRec && fRunnable->IsFreeRec() && (curr_tm<fTestStopTime-0.1)) {

         send_node = fSendSch.Item(fSendSlotIndx, Node()).node;
         send_lid = fSendSch.Item(fSendSlotIndx, Node()).lid;
         send_evid = fSendTurnRec->fVector[send_node];

         if ( (IsWorker() && (send_node==ControllerNodeId())) ||
              (IsController() && (send_node!=Node())) ) send_evid.SetCtrl();

         if (send_evid.null())
            fSendSch.ShiftToNextOperation(Node(), fSendSlotIndx);
         else {
            // TODO: this is just cross-check, can be removed
            if ((send_node == ControllerNodeId()) || IsController())
               if (!send_evid.ctrl()) EOUT(("Expect controll packet id here!!!"));
            // TODO: if operation is too late, one should skip it already here, no need to submit into runnable
            next_send_time = fSendTurnRec->starttime + fSendSch.timeSlot(fSendSlotIndx);
            if (curr_tm < next_send_time - fOperPreTime) next_send_time = 0;
         }
      }


      // this is submit of send operation
      if (next_send_time > 0.) {

         // TODO: if record not found, we should submit dummy record to complete operation on the other side
         EventPartRec* evrec(0);
         dabc::Buffer buf;
         bool do_operation(false);

         if (send_evid.ctrl()) {
            buf = ProduceCtrlBuffer(send_node);
            do_operation = true;
         } else {
            evrec = FindEventPartRec(send_evid);
            if ((evrec==0) && (send_evid>fLastEventId)) {
               // event is not appears when we want to send it
               // we should send dummy buffer that operation is completed on the other side
               // when we are working without time schedule, just wait until data appears
               EOUT(("Cannot find event %u at the moment when it should be sumbitted", (unsigned)send_evid));
            } else
            if (evrec==0) {
               // this is situation when buffer either lost or not arrived at all
               // we only could inform receiver that no chance to get data
            } else
            if (send_node == Node()) {
               // this is sending of operation to itself, do it directly
               if (ProvideReceivedBuffer(send_evid, Node(), evrec->buf)) {
                  did_operation = true;
                  evrec->state = eps_Ready;
                  DOUT2(("Provide event %u to myself", (unsigned) send_evid));
               } else
                  break;
            } else {
               do_operation = true;
               // we keep copy of original buffer in the record
               // TODO: do we need duplication here - buffer is not modified elsewhere and used always from this thread
               // one could use buffer as is
               buf = evrec->buf.Duplicate();
            }
         }

         if (do_operation) {

            int recid = fRunnable->PrepareOperation(kind_Send, sizeof(TransportHeader), buf);
            OperRec* rec = fRunnable->GetRec(recid);

            if (rec==0) {
               EOUT(("Cannot prepare send operation recid = %d till_oper %5.3f", recid, next_send_time - curr_tm));
               return true;
            }

            TransportHeader* hdr = (TransportHeader*) rec->header;
            hdr->srcnode   = Node();      // source node id
            hdr->tgtnode   = send_node;   // target node id
            hdr->evid      = send_evid;   // event id
            hdr->send_tm   = curr_tm;     // time
            hdr->seqid     = fSendOperCounter(send_lid,send_node)++; // sequence number
            hdr->sendkind  = 1;
            hdr->sendlen   = buf.GetTotalSize();

            rec->SetTarget(send_node, send_lid);
            rec->SetTime(next_send_time);

            if (!SubmitToRunnable(recid, rec)) {
               EOUT(("Cannot submit send operation to lid %d node %d", send_lid, send_node));
               return false;
            }

            DOUT1(("SubmitSendOper recid %d node %d lid %d buf %u till oper %8.6f stamp %7.6f", recid, send_node, send_lid, (unsigned) buf.GetTotalSize(), next_send_time - curr_tm, fStamping()));

            if (evrec) evrec->state = eps_Scheduled;

            fNumSendPackets++;

            did_operation = true;
         }

         // shift to next operation, in some cases even when previous operation was not performed
         if (did_operation)
            fSendSch.ShiftToNextOperation(Node(), fSendSlotIndx);
      }

      // this is submit of recv operation
      if ((fRefillCounter > 0) && !did_operation) {
         dabc::Buffer buf = FindPool("BnetDataPool")->TakeBuffer(fTestBufferSize);

         if (buf.null()) {
            EOUT(("Not enough buffers"));
            exit(18);
         }

         int recid = fRunnable->PrepareOperation(kind_Recv, sizeof(TransportHeader), buf);
         OperRec* rec = fRunnable->GetRec(recid);

         if (rec==0) {
            EOUT(("Cannot prepare recv pool operation recid = %d", recid));
            exit(19);
         }

         rec->skind = skind_Pool;
         rec->SetTarget(0, 0);
         rec->SetImmediateTime();

         if (!SubmitToRunnable(recid,rec)) {
            EOUT(("Cannot submit recv operation for the pool queue"));
            exit(20);
         }
         fRefillCounter--;
         did_operation = true;
      }

   } while (did_operation && (maxcnt-->0));

   if (loop_time.Number() > 50) {
      DOUT1((" <<<<<<<<<<<<< LOOP TIME %5.3f ms >>>>>>>>>>>", loop_time.Mean()*1e3 ));
      loop_time.Reset();
   }
   // if (maxcnt<198) DOUT1(("<<<<<<<<<<<<<<<<<<<< EXIT ProcessAllToAllLoop maxcnt %d",maxcnt));

   return true;
}

int bnet::TransportModule::ProcessAskQueue(void* tgt)
{
   DOUT2(("Ask queue sizes"));

   int len = NumNodes()*NumLids()*2*sizeof(int32_t);

   if (!fRunnable->GetQueuesFillState(tgt, len)) {
      EOUT(("Runnable fails to return queue sizes - why???"));
      exit(7);
   } else {
      DOUT2(("Get queue sizes"));
   }

   return len;
}

bool bnet::TransportModule::ProcessCleanup(int32_t* pars)
{
   bool isany(true);

   DOUT2(("Invoke cleanup command"));

   while (isany) {
      isany = false;
      for (int node=0;node<NumNodes();node++)
         for (int lid=0;lid<NumLids();lid++) {
            int* entry = pars + node*NumLids()*2 + lid*2;

            if (entry[0]>0) {
               // perform send operation, buffer can be empty
               dabc::Buffer buf;

               int recid = fRunnable->PrepareOperation(kind_Send, sizeof(TransportHeader), buf);
               OperRec* rec = fRunnable->GetRec(recid);

               if (rec==0) {
                  EOUT(("Cannot prepare send operation"));
                  return false;
               }

               TransportHeader* hdr = (TransportHeader*) rec->header;
               hdr->srcnode = Node();    // source node id
               hdr->tgtnode = node;      // target node id
               hdr->evid = 123456;     // event id
               hdr->send_tm = fStamping();
               hdr->seqid = 0;

               rec->SetTarget(node, lid);
               rec->SetImmediateTime();
               rec->skind = skind_NoQueueChk;

               if (!SubmitToRunnable(recid,rec)) {
                  EOUT(("Cannot submit send operation to lid %d node %d", lid, node));
                  return false;
               }

               entry[0]--;
               isany = true;
            }

            if (entry[1]>0) {
               // perform recv operation

               dabc::Buffer buf = FindPool("BnetDataPool")->TakeBuffer(fTestBufferSize);

               if (buf.null()) {
                  EOUT(("Not enough buffers"));
                  return false;
               }

               int recid = fRunnable->PrepareOperation(kind_Recv, sizeof(TransportHeader), buf);
               OperRec* rec = fRunnable->GetRec(recid);

               if (rec==0) {
                  EOUT(("Cannot prepare recv operation"));
                  return false;
               }

               rec->SetTarget(node, lid);
               rec->SetImmediateTime();
               rec->skind = skind_NoQueueChk;

               if (!SubmitToRunnable(recid,rec)) {
                  EOUT(("Cannot submit receive operation to lid %d node %d", lid, node));
                  return false;
               }

               entry[1]--;
               isany = true;
            }
         }
      // let process events and cleanup queues
      if (isany) WorkerSleep(0.001);
   }

   return true;
}


void bnet::TransportModule::ProcessInputEvent(dabc::Port* port)
{
   unsigned portid = IOPortNumber(port);

   if (port->IsName("DataInput")) {
      if (doDataReceiving()) ReadoutNextEvents(port);
   } else
   if (IsSlave() && (portid==0)) {
      ProcessNextSlaveInputEvent();
   } else
   if (IsMaster() && ((int)portid<NumNodes()-1)) {
      if (!port->CanRecv()) return;
      dabc::Buffer buf = port->Recv();
      ProcessReplyBuffer(portid+1, buf);
   }
}

void bnet::TransportModule::ProcessOutputEvent(dabc::Port* port)
{
   if (port->IsName("DataOutput")) {
      if (doEventBuilding()) BuildReadyEvents(port);
   }
}

bnet::EventId bnet::TransportModule::ExtractEventId(const dabc::Buffer& buf)
{
   bnet::EventId evid(0);

   if (fEventHandling()->ExtractEventId(buf,evid)) return evid;

   return 0;
}

void bnet::TransportModule::ReadoutNextEvents(dabc::Port* port)
{
   if (port==0) return;

   if (!fAllToAllActive) return;

   ReleaseReadyEventParts();

   while (port->CanRecv() && !fEvPartsQueue.Full()) {
      dabc::Buffer buf = port->Recv();

      if (buf.null()) break;

      EventPartRec* rec = fEvPartsQueue.PushEmpty();

      rec->state = eps_Init;
      rec->buf << buf;
      rec->evid = ExtractEventId(rec->buf);
      rec->acq_tm = fStamping();

      fLastEventId = rec->evid;

      if (!fIsFirstEventId) {
         fIsFirstEventId = true;
         fFirstEventId = fLastEventId;
         // initialize overflow counter to match schedule with first and folowing events
         fSendTurnCnt = fFirstEventId / NumNodes();
      } else
      if (fFirstEventId > fLastEventId) {
         EOUT(("Order mismatch of comming events - first event id bigger than current"));
         exit(579);
      }
   }
}

unsigned bnet::TransportModule::CloseSubpacketHeader(unsigned kind, dabc::Pointer& bgn, const dabc::Pointer& ptr)
{
   ControlSubheader hdr;
   hdr.kind = kind;
   hdr.len = bgn.distance_to(ptr);
   bgn.copyfrom(&hdr, sizeof(hdr));
   return hdr.len;
}


dabc::Buffer bnet::TransportModule::ProduceCtrlBuffer(int tgtnode)
{
//   DOUT1(("ProduceCtrlBuffer for node %d", tgtnode));

   dabc::Buffer buf;

   if ((tgtnode<0) || (tgtnode>=NumNodes())) {
      EOUT(("Wrong index specified %d", tgtnode));
      return buf;
   }

   buf = FindPool("BnetDataPool")->TakeBuffer(fTestBufferSize);

//   DOUT1(("ProduceCtrlBuffer buffer for node %d size %u", tgtnode, (unsigned) buf.GetTotalSize()));

   if (buf.null()) {
      EOUT(("Not enough buffers"));
      return buf;
   }

   unsigned totallen(0);

   dabc::Pointer ptr = buf, bgn;

//   DOUT1(("ProduceCtrlBuffer for node %d isworker %s", tgtnode, DBOOL(IsWorker())));

   if (IsWorker()) {
      bgn = ptr;
      ptr.shift(sizeof(ControlSubheader)); // keep place for header
      fEvPartsQueue.FillEventsInfo(ptr, fWorkerCtrlEvId);
      totallen += CloseSubpacketHeader(cpk_SubevSizes, bgn, ptr);

//      DOUT1(("Event data with header %u", (unsigned) bgn.distance_to(ptr)));

      bgn = ptr;
      ptr.shift(sizeof(ControlSubheader)); // keep place for header
      uint64_t evid = fLastBuildEventId;
      ptr.copyfrom_shift(&evid, sizeof(uint64_t));
      ptr.copyfrom_shift(&fLastBuildEventTime, sizeof(double));
      totallen += CloseSubpacketHeader(cpk_BuilderInfo, bgn, ptr);

//      DOUT1(("Last build event data with header %u", (unsigned) bgn.distance_to(ptr)));

   } else
   if (IsController()) {

      bgn = ptr;
      ptr.shift(sizeof(ControlSubheader));
      fSchTurnsQueue.FillTurnsInfo(ptr, fNodesInfo[tgtnode].last_send_turnid);
      totallen += CloseSubpacketHeader(cpk_Turns, bgn, ptr);

//      DOUT1(("Turns data with header %u queue %u node_send %u", (unsigned) bgn.distance_to(ptr), fSchTurnsQueue.Size(), (unsigned) fNodesInfo[tgtnode].last_send_turnid));

      bgn = ptr;
      ptr.shift(sizeof(ControlSubheader));
      uint64_t id = fMasterSkipEventId;
      ptr.copyfrom_shift(&id, sizeof(uint64_t));
      id = fMasterSkipTurnId;
      ptr.copyfrom_shift(&id, sizeof(uint64_t));
      totallen += CloseSubpacketHeader(cpk_SkipMarkers, bgn, ptr);

//      DOUT1(("Skip data with header %u", (unsigned) bgn.distance_to(ptr)));

   } else {
      EOUT(("Thats happend??"));
      exit(7);
   }

   DOUT2(("ProduceCtrlBuffer for node %d done totallen %u", tgtnode, totallen));

   buf.SetTotalSize(totallen);

   return buf;
}


bnet::EventPartRec* bnet::TransportModule::FindEventPartRec(const bnet::EventId& evid)
{
   if (evid.null()) return 0;

   if (evid.ctrl()) {
      EOUT(("Should not request control event here"));
      return 0;
   }

   // TODO: make searching even more optimal - one could use map or binary search
   return fEvPartsQueue.Find(evid);
}

bnet::EventPartRec* bnet::TransportModule::GetFirstReadyPartRec()
{
   // method to find first event part, which could be send to the receiver

   for (unsigned n=0;n<fEvPartsQueue.Size();n++) {
      bnet::EventPartRec* rec = &fEvPartsQueue.Item(n);
      if (rec->state == eps_Init) return rec;
   }

   return 0;
}


bnet::EventBundleRec* bnet::TransportModule::FindEventBundleRec(const bnet::EventId& evid)
{
   for (unsigned n=0;n<fEvBundelsQueue.Size();n++) {
      bnet::EventBundleRec* rec = &fEvBundelsQueue.Item(n);
      if (rec->evid == evid) return rec;
   }

   return 0;
}

bool bnet::TransportModule::ProcessControlPacket(int nodeid, uint32_t kind, dabc::Pointer& rawdata)
{
   bool res(true);

   switch (kind) {
      case cpk_Null:
         EOUT(("Null control kind"));
         return false;
      case cpk_SubevSizes:   // data with subevents info from node
         // for the moment only subevents ids
         DOUT1(("Add raw event data from node %d size %u", nodeid, rawdata.fullsize()));
         res = fEvMasterQueue.AddRawEventInfo(rawdata, nodeid, fStamping());
         break;
      case cpk_Turns: {    // data with event association and in which time slot these events should be send

         unsigned oldsize = fSchTurnsQueue.Size();
         fSchTurnsQueue.AddRawTurns(rawdata);
         if (oldsize != fSchTurnsQueue.Size()) DOUT1(("Add new turns old %u new %u", oldsize, fSchTurnsQueue.Size()));
         break;
      }
      case cpk_BuilderInfo: {

         if ((int) fNodesInfo.size() <= nodeid) {
            EOUT(("NodesInfo %u too small - nodeid %d", fNodesInfo.size(), nodeid));
            exit(77);
         }

         uint64_t id(0);
         double tm(0);
         rawdata.copyto_shift(&id, sizeof(uint64_t));
         rawdata.copyto_shift(&tm, sizeof(double));
         fNodesInfo[nodeid].last_build_id = id;
         fNodesInfo[nodeid].last_build_tm = tm;
         break;
      }

      case cpk_SkipMarkers: {
         uint64_t id(0);
         rawdata.copyto_shift(&id, sizeof(uint64_t));
         bnet::EventId evid = id;
         if (!evid.null()) {
            unsigned sz1 = fEvPartsQueue.Size(), sz2 = fEvBundelsQueue.Size();
            fEvPartsQueue.SkipEventParts(evid);
            fEvBundelsQueue.SkipEventBundels(evid);
            DOUT1(("SKIP ev %u parts old:%u diff:%u front:%u bundels old:%u diff:%u", (unsigned) evid, sz1, sz1 - fEvPartsQueue.Size(), (unsigned) fEvPartsQueue.Front().evid, sz2, sz2 - fEvBundelsQueue.Size()));
         }
         rawdata.copyto_shift(&id, sizeof(uint64_t));
         if (id>0)
            fSchTurnsQueue.RemoveTurnsUntil(id);

         break;
      }

      case cpk_SchedSlot:      // definition of base time for next schedules
         break;

      default:
         EOUT(("Uncknown control packet kind %u", (unsigned) kind));
         return false;
   }

   return true;
}



bool bnet::TransportModule::ProvideReceivedBuffer(const bnet::EventId& evid, int nodeid, dabc::Buffer& buf)
{
   // here one should collect subevents and build ready event when all parts are there

   if (evid.null()) {
      EOUT(("Dimmy event here - error"));
      buf.Release();
      return false;
   }

   // if this is control packet, decode it - it has same structure for all cases
   if (evid.ctrl()) {
      bool res(true);

      DOUT2(("Get control packet size %u from node %d", (unsigned) buf.GetTotalSize(), nodeid));

      dabc::Pointer ptr(buf);

      while (ptr.fullsize() >= sizeof(ControlSubheader)) {
         ControlSubheader hdr;
         ptr.copyto(&hdr, sizeof(hdr));

         if (hdr.len > sizeof(hdr)) {

            dabc::Pointer rawdata(ptr, sizeof(hdr), hdr.len - sizeof(hdr));

            DOUT2(("Control data kind %u size %u rawsize %u", (unsigned) hdr.kind, (unsigned) hdr.len, (unsigned) rawdata.fullsize()));

            res = res & ProcessControlPacket(nodeid, hdr.kind, rawdata);
         }

         ptr.shift(hdr.len);
      }

      buf.Release();

      return res;
   }

   bnet::EventBundleRec* rec = FindEventBundleRec(evid);

   if (rec==0) {
      if (fEvBundelsQueue.Full()) {
         // EOUT(("No place for new events!!!"));
         // buf.Release();
         return false;
      }

      rec = fEvBundelsQueue.PushEmpty();

      rec->acq_tm = fStamping();
      rec->evid = evid;
   }

   if (haveController()) nodeid--;
   rec->buf[nodeid] << buf;

   return true;
}


void bnet::TransportModule::BuildReadyEvents(dabc::Port* port)
{
   double curr_tm = fStamping();

   // TODO: we should build not only the first event, it could be ready events in the middle

   while (!fEvBundelsQueue.Empty()) {
      bnet::EventBundleRec* rec = &fEvBundelsQueue.Front();

      if (curr_tm > rec->acq_tm + fEventLifeTime) {
         EOUT(("Release event bundle due to timeout"));
         fEvBundelsQueue.PopOnly();
         continue;
      }

      if (!rec->ready() || !port->CanSend()) return;


      // building of event
      dabc::Buffer res = fEventHandling()->BuildFullEvent(rec->evid, rec->buf, rec->numbuf);

      if (res.null()) {
         EOUT(("Event not build !!!!"));
         return;
      }

      DOUT1(("BUILD event %u len %u", (unsigned) rec->evid, (unsigned) res.GetTotalSize()));

      fLastBuildEventId = rec->evid;
      fLastBuildEventTime = curr_tm;

      // deliver to the output
      port->Send(res);

      // release item
      fEvBundelsQueue.PopOnly();
   }
}


void bnet::TransportModule::ReleaseReadyEventParts()
{
   double curr_tm = fStamping();

   while (!fEvPartsQueue.Empty()) {
      EventPartRec* rec = &fEvPartsQueue.Front();

//      if (rec->state == eps_Ready) {
//         fEvPartsQueue.PopOnly();
//         continue;
//      }

      if (curr_tm > rec->acq_tm + fEventLifeTime) {
         EOUT(("Event %u should be dropped", (unsigned) rec->evid));
         fEvPartsQueue.PopOnly();
         continue;
      }

      return;
   }
}

void bnet::TransportModule::ProcessNextSlaveInputEvent()
{
   // we cannot accept next command until previous is not yet executed
   if (fRunningCmdId>0) return;

   dabc::Port* port = IOPort(0);

   if (!port->CanRecv()) return;

   dabc::Buffer buf = port->Recv();
   if (buf.null()) return;

   fRunningCmdId = PreprocessTransportCommand(buf);

   port->Send(buf);

   switch (fRunningCmdId) {
      case BNET_CMD_TIMESYNC: {
         // DOUT0(("Prepare slave sync loop"));
         PrepareSyncLoop(0);
         fCmdEndTime.GetNow(5.);
         break;
      }
      default:
         fRunningCmdId = 0;
         break;
   }
}


bool bnet::TransportModule::RequestMasterCommand(int cmdid, void* cmddata, int cmddatasize, void* allresults, int resultpernode)
{
    // Request from master side to move in specific command

   if (!IsMaster()) {
      EOUT(("Cannot run on the slave"));
      return false;
   }

   // if module not working, do not try to do anything else
   if (!IsRunning()) {
      DOUT0(("Command %d cannot be executed - module is not active", cmdid));
      return false;
   }

   bool incremental_data = false;
   if (cmddatasize<0) {
      cmddatasize = -cmddatasize;
      incremental_data = true;
   }

   int fullpacketsize = sizeof(CommandMessage) + cmddatasize;

   if (resultpernode > cmddatasize) {
      // special case, to use same buffer on slave for reply,
      // original buffer size should be not less than requested
      // therefore we just append fake data
      fullpacketsize = sizeof(CommandMessage) + resultpernode;
   }

   if (fullpacketsize>fCmdBufferSize) {
      EOUT(("Too big command data size %d", cmddatasize));
      return false;
   }


   // prepare all vatiables before submit first buffer - one could get reply faster than expected
   fCmdReplies.resize(NumNodes());
   for (int n=0;n<NumNodes();n++) fCmdReplies[n] = false;
   DOUT1(("Reset all replies"));

   fCmdAllResults = allresults;   // buffer where replies from all nodes collected
   fCmdResultsPerNode = resultpernode;  // how many data in replied is expected

   fRunningCmdId = cmdid;
   fCmdStartTime.GetNow();
   fCmdEndTime = fCmdStartTime + fCurrentCmd.GetTimeout(); // time will be expired in 5 seconds


   dabc::Buffer buf0;

   for(int node=0;node<NumNodes();node++) if (fActiveNodes[node]) {

      dabc::Buffer buf = FindPool("BnetCtrlPool")->TakeBuffer(fullpacketsize);
      if (buf.null()) { EOUT(("No empty buffer")); return false; }

      dabc::Pointer ptr(buf);

      CommandMessage msg;
      msg.magic = BNET_CMD_MAGIC;
      msg.cmdid = cmdid;
      msg.cmddatasize = cmddatasize;
      msg.delay = 0;
      msg.getresults = resultpernode;
      ptr.copyfrom_shift(&msg, sizeof(msg));

      if ((cmddatasize>0) && (cmddata!=0)) {
         if (incremental_data)
            ptr.copyfrom((uint8_t*) cmddata + node * cmddatasize, cmddatasize);
         else
            ptr.copyfrom(cmddata, cmddatasize);
      }

      buf.SetTotalSize(fullpacketsize);

      if (node==0) {
         buf0 << buf;
      } else {
//         DOUT0(("Send buffer to slave %d", node));
         IOPort(node-1)->Send(buf);
      }
   }

   PreprocessTransportCommand(buf0);

   ProcessReplyBuffer(0, buf0);

   return true;
}


bool bnet::TransportModule::ProcessReplyBuffer(int nodeid, dabc::Buffer buf)
{
   if (buf.null()) { EOUT(("Empty buffer")); return false; }

   dabc::Pointer ptr(buf);

   CommandMessage rcv;
   ptr.copyto(&rcv, sizeof(CommandMessage));

   bool iserr = false;

   if (rcv.magic!=BNET_CMD_MAGIC) { EOUT(("Wrong magic")); iserr = true; }
   if (rcv.cmdid != fRunningCmdId) { EOUT(("Wrong ID recv %u  cmd %u", (unsigned) rcv.cmdid, (unsigned) fRunningCmdId)); iserr = true; }
   if (rcv.node != nodeid) { EOUT(("Wrong node")); iserr = true; }

   if (fCmdReplies[nodeid]) { EOUT(("Node %d already replied command rcvid %u", nodeid, (unsigned) rcv.cmdid)); iserr = true; }

   if (iserr) return false;

   DOUT1(("Get reply buffer from node %d cmd %u", nodeid, (unsigned) rcv.cmdid));

   fCmdReplies[nodeid] = true;

   if ((fCmdAllResults!=0) && (fCmdResultsPerNode>0) && (rcv.cmddatasize == fCmdResultsPerNode)) {
      ptr.shift(sizeof(CommandMessage));

      // DOUT0(("Copy from node %d buffer %d", nodeid, resultpernode));
      ptr.copyto((uint8_t*)fCmdAllResults + nodeid*fCmdResultsPerNode, fCmdResultsPerNode);
   }

   bool finished(true);

   for (unsigned n=0;n<fCmdReplies.size();n++)
      if (fActiveNodes[n] && !fCmdReplies[n]) finished = false;

   if (finished) {
      DOUT2(("Command %d execution finished in %7.6f", fRunningCmdId, fCmdStartTime.SpentTillNow()));

      if (fRunningCmdId == BNET_CMD_TEST)
         fCmdTestLoop.Fill(fCmdStartTime.SpentTillNow());

      CompleteRunningCommand();

      ShootTimer("Timer", 0.); // invoke next command as soon as possible
   }


   return true;
}


int bnet::TransportModule::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName(TransportCmd::CmdName())) {
      fCmdsQueue.Push(cmd);
      ShootTimer("Timer", 0.); // invoke next command as soon as possible
      return dabc::cmd_postponed;
   }

   return ModuleAsync::ExecuteCommand(cmd);
}

void bnet::TransportModule::PrepareSyncLoop(int tgtnode)
{
   int numcycles = fSyncArgs[0];
   int maxqueuelen = fSyncArgs[1];
   int sync_lid = fSyncArgs[2];
   int nrepeat = fSyncArgs[3];
   bool dosynchronisation = fSyncArgs[4]>0;
   bool doscaling = fSyncArgs[5]>0;

   fRunnable->ConfigSync(IsMaster(), numcycles, dosynchronisation, doscaling);

   fRunnable->ConfigQueueLimits(2, maxqueuelen);


   // first fill receiving queue
   for (int n=-1;n<maxqueuelen+1;n++) {

      OperKind kind = (n==-1) || (n==maxqueuelen) ? kind_Send : kind_Recv;

      if ((kind==kind_Send)) {
         if (IsMaster() && (n==-1)) continue; // master should submit send at the end
         if (!IsMaster() && (n==maxqueuelen)) continue; // slave should submit send in the begin
      }

      int recid = fRunnable->PrepareOperation(kind, sizeof(TimeSyncMessage));
      OperRec* rec = fRunnable->GetRec(recid);
      if ((recid<0) || (rec==0)) { EOUT(("Internal")); return; }
      if (kind==kind_Recv) {
         rec->skind = IsMaster() ? skind_SyncMasterRecv : skind_SyncSlaveRecv;
         rec->SetRepeatCnt(nrepeat);
      } else {
         rec->skind = IsMaster() ? skind_SyncMasterSend : skind_SyncSlaveSend;
         rec->SetRepeatCnt(nrepeat * maxqueuelen);
      }

      rec->SetTarget(tgtnode, sync_lid);

      SubmitToRunnable(recid, rec);
   }
}

void bnet::TransportModule::CompleteRunningCommand(int res)
{
   if (fRunningCmdId<=0) return;

   fRunningCmdId = 0;
   fCmdStartTime.Reset();
   fCmdEndTime.Reset();
   fCurrentCmd.Reply(res);

   ShootTimer("Timer", 0.);
}

void bnet::TransportModule::ProcessUserEvent(dabc::ModuleItem* item, uint16_t evid)
{
   // DOUT1(("ENTER bnet::TransportModule::ProcessUserEvent"));

   if (item == fReplyItem) {

      int recid;

      while ((recid = fRunnable->GetCompleted())>=0) {
         OperRec* rec = fRunnable->GetRec(recid);

         if (rec==0) {
            EOUT(("Internal error"));
            exit(7);
         }

         if ((rec->tgtnode>=NumNodes()) || (rec->tgtindx>=NumLids())) {
            EOUT(("Wrong result id lid:%d node:%d", rec->tgtindx, rec->tgtnode));
            exit(6);
         }

         // DOUT1(("Process user event kind:%d", rec->kind));

         // keep track only over normal send/recv operations
         switch (rec->kind) {
            case kind_Send:
               fSendQueue[rec->tgtindx][rec->tgtnode]--;
               fTotalSendQueue--;
               ProcessSendCompleted(recid, rec);
               break;
            case kind_Recv:
               fRecvQueue[rec->tgtindx][rec->tgtnode]--;
               fTotalRecvQueue--;
               ProcessRecvCompleted(recid, rec);
               break;
            default:
               break;
         }

         fRunnable->ReleaseRec(recid);
      }

      // activate extra timer event when no send/recv queue
      if ((TotalRecvQueue()==0) && (TotalSendQueue()==0)) {
         // DOUT0(("AAAAAAA - we do not get next event"));
         ShootTimer("Timer", 0.);
      }

   } else {
      EOUT(("User event from unknown item"));
   }
}

void bnet::TransportModule::ProcessSendCompleted(int recid, OperRec* rec)
{
   if (!fAllToAllActive) return;

   TransportHeader* hdr = (TransportHeader*) rec->header;

   bnet::EventId evid = hdr->evid;
   if (!evid.ctrl()) {
      EventPartRec* evrec = FindEventPartRec(evid);
      if (evrec==0) {
         EOUT(("Cannot find event structure for event %u which was send - is it skipped meanwhile???", (unsigned) hdr->evid));
      } else {
         // we mark event record as ready - it will be removed soon
         evrec->state = eps_Ready;

         DOUT2(("Process send completed evid %u", (unsigned) hdr->evid));
      }
   }

   DOUT1(("ProcessSendCompleted recid %d node %d err %s stamp %7.6f", recid, rec->tgtnode, DBOOL(rec->err), fStamping()));

   fNumComplSendPackets++;

   double curr_tm = fStamping();

   fSendStartTime.Fill(rec->is_time - rec->oper_time);

   fSendComplTime.Fill(rec->compl_time - rec->is_time);

   fOperBackTime.Fill(curr_tm - rec->compl_time);

   fSendRate.Packet(fTestBufferSize, curr_tm);
}

void bnet::TransportModule::ProcessRecvCompleted(int recid, OperRec* rec)
{
   if (!fAllToAllActive) return;

   // TODO: event building should be implemented, for the moment we just skip all data

   if (rec->skind == skind_Pool) {
      DOUT0(("Runnable returns pool buffer - can it happen when AllToAll active?"));
      return;
   }

   TransportHeader* hdr = (TransportHeader*) rec->header;

   DOUT1(("ProcessRecvCompleted recid %d node %d err %s stamp %7.6f evid %u", recid, rec->tgtnode, DBOOL(rec->err), fStamping(), (unsigned)hdr->evid ));


   bool isok(true);

   int lost = (int) hdr->seqid - fRecvOperCounter(rec->tgtindx, rec->tgtnode) - 1;
   if (lost>0) {
      fNumLostPackets += lost;
      fRecvSkipCounter(rec->tgtindx, rec->tgtnode) += lost;
   }

   if ((int) hdr->srcnode != rec->tgtnode) {
      EOUT(("Missmatch of source node id in received buffer expected:%u received:%u", (unsigned) rec->tgtnode, (unsigned) hdr->srcnode));
      isok = false;
   }

   if ((int) hdr->tgtnode != Node()) {
      EOUT(("Missmatch of target node id in received buffer expected:%d received:%u", Node(), (unsigned) hdr->tgtnode));
      isok = false;
   }

   if (hdr->sendlen > rec->buf.GetTotalSize()) {
      EOUT(("Send length bigger than size of prepared buffer"));
      isok = false;
   } else {
      rec->buf.SetTotalSize(hdr->sendlen);
   }

   fRecvOperCounter(rec->tgtindx, rec->tgtnode) = hdr->seqid;

   double curr_tm = fStamping();

   fRecvComplTime.Fill(rec->compl_time - hdr->send_tm);

   fOperBackTime.Fill(curr_tm - rec->compl_time);

   fRecvRate.Packet(fTestBufferSize, curr_tm);

   fTotalRecvPackets++;

   // account buffers which should be refill
   if (rec->skind == skind_Refill) fRefillCounter++;

   if (isok) ProvideReceivedBuffer(hdr->evid, rec->tgtnode, rec->buf);
}


void bnet::TransportModule::ProcessTimerEvent(dabc::Timer* timer)
{
//   DOUT1(("ENTER bnet::TransportModule::ProcessTimerEvent"));

   if (timer->IsName("AllToAll")) {
      if (ProcessAllToAllAction()) timer->SingleShoot(0.00001);
      return;
   }


   if ((fRunningCmdId>0) && !fCmdEndTime.null() && fCmdEndTime.Expired()) {
      if (fRunningCmdId!=BNET_CMD_WAIT)
         EOUT(("Command %d execution timed out", fRunningCmdId));
      CompleteRunningCommand(dabc::cmd_timedout);
   }

   if (fRunningCmdId>0) {
      // we are in the middle of running operation, in some cases we could activate new actions

      switch (fRunningCmdId) {
         case BNET_CMD_TIMESYNC:
            if ((TotalSendQueue() > 0) || (TotalRecvQueue() > 0) || IsMaster()) break;
            CompleteRunningCommand();
            break;
         case BNET_CMD_EXECSYNC:
            // DOUT0(("Here is master send:%d recv:%d", TotalSendQueue(), TotalRecvQueue()));
            if ((TotalSendQueue() > 0) || (TotalRecvQueue() > 0)) break;
            fSlaveSyncNode++;

            if (fSlaveSyncNode<NumNodes()) {
               DOUT0(("Prepeare master for node %d", fSlaveSyncNode));
               PrepareSyncLoop(fSlaveSyncNode);
            } else {
               CompleteRunningCommand();
               DOUT0(("Tyme sync done in %5.4f sec", fSyncStartTime.SpentTillNow()));
            }
            break;
         default:
            break;
      }

      return;
   }

   if (IsSlave()) {
      ProcessNextSlaveInputEvent();
      return;
   }

   // this is sequential execution of master commands, placed in the command queue

   if (fCmdsQueue.Size()==0) return;

   fCurrentCmd = fCmdsQueue.Pop();
   switch (fCurrentCmd.GetId()) {
      case BNET_CMD_WAIT:
         fCmdEndTime.GetNow(fCurrentCmd.GetTimeout());
         if (fCurrentCmd.GetTimeout()>2) DOUT0(("Master sleeps for %3.1f s", fCurrentCmd.GetTimeout()));
         fRunningCmdId = BNET_CMD_WAIT;
         break;

      case BNET_CMD_ACTIVENODES: {
         uint8_t buff[NumNodes()];

         int cnt(1);
         buff[0] = 1;

         for(int node=1;node<NumNodes();node++) {
            bool active = IOPort(node-1)->IsConnected();
            if (active) cnt++;
            buff[node] = active ? 1 : 0;
         }

         DOUT0(("There are %d active nodes, %d missing", cnt, NumNodes() - cnt));
         RequestMasterCommand(BNET_CMD_ACTIVENODES, buff, NumNodes());
         break;
      }

      case BNET_CMD_TEST:
         RequestMasterCommand(BNET_CMD_TEST);
         break;

      case BNET_CMD_EXIT:
         RequestMasterCommand(BNET_CMD_EXIT);
         break;

      case BNET_CMD_MEASURE:
         if (fCmdTestLoop.Number() > 1) {
            fCmdDelay = fCmdTestLoop.Mean();
            DOUT0(("Command loop = %5.3f ms", fCmdDelay*1e3));
         }
         fCmdTestLoop.Reset();
         break;


      case BNET_CMD_CREATEQP: {
         fConnStartTime.GetNow();

         int blocksize = fRunnable->ConnectionBufferSize();

         if (blocksize==0) return;

         AllocCollResults(NumNodes() * blocksize);

         DOUT1(("First collect all QPs info"));

         // own records will be add automatically
         RequestMasterCommand(BNET_CMD_CREATEQP, 0, 0, fCollectBuffer, blocksize);
         break;
      }

      case BNET_CMD_CONNECTQP: {

         DOUT1(("Invoke command for establishing of connections"));

         fRunnable->ResortConnections(fCollectBuffer);

         RequestMasterCommand(BNET_CMD_CONNECTQP, fCollectBuffer, -fRunnable->ConnectionBufferSize());
         break;
      }

      case BNET_CMD_CONNECTDONE: {
         DOUT0(("Establish connections in %5.4f s ", fConnStartTime.SpentTillNow()));
         CompleteRunningCommand();
         break;
      }

      case BNET_CMD_CLOSEQP:
         RequestMasterCommand(BNET_CMD_CLOSEQP);
         break;

      case BNET_CMD_TIMESYNC: {
         int maxqueuelen = 20;
         if (maxqueuelen > fCurrentCmd.GetNRepeat()) maxqueuelen = fCurrentCmd.GetNRepeat();
         int nrepeat = fCurrentCmd.GetNRepeat()/maxqueuelen;
         if (nrepeat<=0) nrepeat = 1;
         int numcycles = nrepeat * maxqueuelen;

         int sync_lid = 0;

         fSyncArgs[0] = numcycles;
         fSyncArgs[1] = maxqueuelen;
         fSyncArgs[2] = sync_lid;
         fSyncArgs[3] = nrepeat;
         fSyncArgs[4] = fCurrentCmd.GetDoSync() ? 1 : 0;
         fSyncArgs[5] = fCurrentCmd.GetDoScale() ? 1 : 0;

         // this is just distribution of sync over slaves, master will be trigerred additionally
         RequestMasterCommand(BNET_CMD_TIMESYNC, fSyncArgs, sizeof(fSyncArgs));
         break;
      }

      case BNET_CMD_EXECSYNC:
         fRunningCmdId = BNET_CMD_EXECSYNC;
         fSyncStartTime.GetNow();
         fSlaveSyncNode = 1;
         PrepareSyncLoop(fSlaveSyncNode);
         break;

      case BNET_CMD_GETSYNC:
         RequestMasterCommand(BNET_CMD_GETSYNC);
         break;

      case BNET_CMD_ALLTOALL: {

         bool allowed_to_skip = false;
         int pattern = Cfg("TestPattern").AsInt(0);

         if ((pattern>=10) && (pattern<20)) {
            allowed_to_skip = true;
            pattern = pattern % 10;
         }

         const char* pattern_name = "---";
         switch (pattern) {
            case 1:
               pattern_name = "ALL-TO-ALL (shift 1)";
               break;
            case 2:
               pattern_name = "ALL-TO-ALL (shift n/2)";
               break;
            case 3:
               pattern_name = "ONE-TO-ALL";
               break;
            case 4:
               pattern_name = "ALL-TO-ONE";
               break;
            case 5:
               pattern_name = "EVEN-TO-ODD";
               break;
            case 7:
               pattern_name = "FILE_BASED";
               break;
            default:
               pattern_name = "ALL-TO-ALL";
               break;
         }


         double arguments[13];

         arguments[0] = Cfg("BufferSize").AsInt(128*1024);
         arguments[1] = NumNodes();
         arguments[2] = Cfg("TestOutputQueue").AsInt(5);
         arguments[3] = Cfg("TestInputQueue").AsInt(10);
         arguments[4] = fStamping() + fCmdDelay + 1.; // start 1 sec after cmd is delivered
         arguments[5] = arguments[4] + Cfg("TestTime").AsInt(10);    // stopping time
         arguments[6] = pattern;
         arguments[7] = Cfg("TestRate").AsInt(1000);
         arguments[8] = 0; // interval for ratemeter, 0 - off
         arguments[9] = allowed_to_skip ? 1 : 0;
         arguments[10] = Cfg("TestControlKind").AsInt(0);  // kind of control 0 - no any controller, 1 - first node is master
         arguments[11] = Cfg(names::SubmitPreTime()).AsDouble(0.005);

         int startruns = Cfg("TestStartTurns").AsInt(5);
         // we need at least 2 transfers before normal turns can be generated
         // on small number of nodes with big operation submit latency many turns can
         // be necessary before such information exchange will be completed
         double emptyturns = 5;
         if (fSendSch.endTime() > 0) emptyturns += 3 * arguments[11] / fSendSch.endTime();
         if (startruns < emptyturns) startruns = (int) emptyturns;
         DOUT0(("Start with %d empty runs", startruns));
         arguments[12] = startruns;

         DOUT0(("====================================="));
         DOUT0(("%s pattern:%d size:%d rate:%3.0f send:%d recv:%d nodes:%d canskip:%s",
               pattern_name, pattern, (int) arguments[0], arguments[7], (int) arguments[2], (int) arguments[3], NumNodes(), DBOOL(allowed_to_skip)));

         RequestMasterCommand(BNET_CMD_ALLTOALL, arguments, sizeof(arguments));

         break;
      }

      case BNET_CMD_GETRUNRES: {
         RequestMasterCommand(BNET_CMD_GETRUNRES);
         break;
      }

      case BNET_CMD_COLLECT: {

         int setsize = fCurrentCmd.GetInt("SetSize", 0);
         if (setsize<=0) {
            EOUT(("no set size was specified"));
            CompleteRunningCommand(dabc::cmd_false);
            break;
         }

         AllocCollResults(setsize*NumNodes());

         RequestMasterCommand(BNET_CMD_COLLECT, 0, 0, fCollectBuffer, setsize);

         break;
      }

      case BNET_CMD_SHOWRUNRES: {
         ShowAllToAllResults();
         CompleteRunningCommand();
         break;
      }

      case BNET_CMD_ASKQUEUE: {
         int blocksize = NumNodes()*NumLids()*2*sizeof(uint32_t);
         AllocCollResults(NumNodes() * blocksize);
         RequestMasterCommand(BNET_CMD_ASKQUEUE, 0, 0, fCollectBuffer, blocksize);
         break;
      }

      case BNET_CMD_CLEANUP: {

         int blocksize = NumNodes()*NumLids()*2*sizeof(uint32_t);

         int32_t* src = (int32_t*) fCollectBuffer;
         int32_t* tgt = (int32_t*) malloc(NumNodes() * blocksize);

         int allsend(0), allrecv(0);

         for(int node1=0;node1<NumNodes();node1++)
            for (int node2=0;node2<NumNodes();node2++)
               for(int nlid=0;nlid<NumLids();nlid++) {
                  int tgtindx = node1*NumNodes()*NumLids()*2 + node2*NumLids()*2 + nlid*2;
                  int srcindx = node2*NumNodes()*NumLids()*2 + node1*NumLids()*2 + nlid*2;
                  // send operation, which should be executed from node1 -> node2
                  tgt[tgtindx] = src[srcindx + 1];

                  // recv operation, which should be executed on node1 for node2
                  tgt[tgtindx + 1] = src[srcindx];

                  allsend += tgt[tgtindx];
                  allrecv += tgt[tgtindx + 1];
               }

         DOUT0(("All send %d and recv %d operation", allsend, allrecv));

         memcpy(fCollectBuffer, tgt, NumNodes()*blocksize);
         free(tgt);

         if (allsend+allrecv == 0)
            CompleteRunningCommand();
         else
            RequestMasterCommand(BNET_CMD_CLEANUP, fCollectBuffer, -blocksize);
         break;
      }

      default:
         EOUT(("Command not supported"));
         exit(1);
   }
}
