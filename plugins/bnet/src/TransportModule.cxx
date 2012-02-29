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

#ifdef WITH_VERBS
#include "bnet/VerbsRunnable.h"
#endif


bnet::TransportModule::TransportModule(const char* name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fStamping(),
   fCmdsQueue()
{
   fNodeNumber = cmd.Field("NodeNumber").AsInt(0);
   fNumNodes = cmd.Field("NumNodes").AsInt(1);

   fNumLids = Cfg("TestNumLids", cmd).AsInt(1);
   if (fNumLids>BNET_MAXLID) fNumLids = BNET_MAXLID;

   int nports = cmd.Field("NumPorts").AsInt(1);

   fTestNumBuffers = Cfg(dabc::xmlNumBuffers, cmd).AsInt(1000);

   DOUT0(("Test num buffers = %d", fTestNumBuffers));

   CreatePoolHandle("BnetCtrlPool");

   CreatePoolHandle("BnetDataPool");

   for (int n=0;n<nports;n++) {
//      DOUT0(("Create IOport %d", n));
      CreateIOPort(FORMAT(("Port%d", n)), FindPool("BnetCtrlPool"), 1, 1);
  }

   fActiveNodes.clear();
   fActiveNodes.resize(fNumNodes, true);

   fTestScheduleFile = Cfg("TestSchedule",cmd).AsStdStr();
   if (!fTestScheduleFile.empty())
      fPreparedSch.ReadFromFile(fTestScheduleFile);

   fResults = 0;

   fCmdDelay = 0.;

   for (int n=0;n<BNET_MAXLID;n++) {
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


   fTrendingInterval = 0;

   fRunnable = 0;
   fRunThread = 0;

   #ifdef WITH_VERBS
   fRunnable = new VerbsRunnable();
   #endif

   fReplyItem = CreateUserItem("Reply");

   if (fRunnable==0) fRunnable = new TransportRunnable();

   fRunnable->SetNodeId(fNodeNumber, fNumNodes, fNumLids, fReplyItem);

   // call Configure before runnable has own thread - no any synchronization problems
   fRunnable->Configure(this, FindPool("BnetDataPool")->Pool(), cmd);

   fCmdBufferSize = 256*1024;
   fCmdDataBuffer = new char[fCmdBufferSize];

   fCollectBuffer = 0;
   fCollectBufferSize = 0;

   // new members

   fRunningCmdId = 0;
   fCmdState = mcmd_None;
   fCmdEndTime.Reset();  // time when command should finish its execution
   fCmdReplies.resize(0); // indicates if cuurent cmd was replied
   fCmdAllResults = 0;   // buffer where replies from all nodes collected
   fCmdResultsPerNode = 0;  // how many data in replied is expected

   fAllToAllActive = false;

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

      fCmdsQueue.PushD(TransportCmd(BNET_CMD_WAIT, 10.)); // master wait 10 seconds

      TransportCmd cmd_sync2(BNET_CMD_TIMESYNC, 5.);
      cmd_sync2.SetSyncArg(200, true, true);
      fCmdsQueue.PushD(cmd_sync2);
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_EXECSYNC));
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_GETSYNC));

      fCmdsQueue.PushD(TransportCmd(BNET_CMD_ALLTOALL)); // configure and start all to all execution

      fCmdsQueue.PushD(TransportCmd(BNET_CMD_WAIT, Cfg("TestTime").AsInt(10) + 3)); // master wait 10 seconds

      fCmdsQueue.PushD(TransportCmd(BNET_CMD_GETRUNRES));
      TransportCmd cmd_coll1(BNET_CMD_COLLECT);
      cmd_coll1.SetInt("SetSize", 14*sizeof(double));
      fCmdsQueue.PushD(cmd_coll1);

      fCmdsQueue.PushD(TransportCmd(BNET_CMD_SHOWRUNRES));

      fCmdsQueue.PushD(TransportCmd(BNET_CMD_ASKQUEUE));
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_CLEANUP));
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_ASKQUEUE));
      fCmdsQueue.PushD(TransportCmd(BNET_CMD_CLEANUP));


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
   delete fRunnable; fRunnable = 0;

   if (fCmdDataBuffer) {
      delete [] fCmdDataBuffer;
      fCmdDataBuffer = 0;
   }

   for (int n=0;n<BNET_MAXLID;n++) {
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
      EOUT(("Cannot submit receive operation to lid %d node %d", rec->tgtindx, rec->tgtnode));
      return false;
   }

   return true;
}

int bnet::TransportModule::PreprocessTransportCommand(dabc::Buffer& buf)
{
   if (buf.null()) return BNET_CMD_EXIT;

   CommandMessage* msg = (CommandMessage*) buf.SegmentPtr(0);

   if (msg->magic!=BNET_CMD_MAGIC) {
       EOUT(("Magic id is not correct"));
       return BNET_CMD_EXIT;
   }

   int cmdid = msg->cmdid;

   fCmdDataSize = msg->cmddatasize;
   if (fCmdDataSize > fCmdBufferSize) {
      EOUT(("command data too big for buffer!!!"));
      fCmdDataSize = fCmdBufferSize;
   }

   memcpy(fCmdDataBuffer, msg->cmddata(), fCmdDataSize);
//   double delay = msg->delay;
//   dabc::TimeStamp recvtm = dabc::Now();

   msg->node = dabc::mgr()->NodeId();
   msg->cmddatasize = 0;
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
         if ((msg->getresults > 0) && (fResults!=0)) {
            msg->cmddatasize = msg->getresults;
            sendpacketsize += msg->cmddatasize;
            memcpy(msg->cmddata(), fResults, msg->getresults);
            cmd_res = true;
         }
         break;

      case BNET_CMD_CREATEQP: {
         int ressize = msg->getresults;
         cmd_res = fRunnable->CreateQPs(msg->cmddata(), ressize);
         msg->cmddatasize = ressize;
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
         msg->cmddatasize = ProcessAskQueue(msg->cmddata());
         sendpacketsize += msg->cmddatasize;
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

   buf.SetTotalSize(sendpacketsize);

   if (!cmd_res) EOUT(("Command %d failed", cmdid));

   return cmdid;
}

void bnet::TransportModule::ActivateAllToAll(double* arguments)
{
   fTestBufferSize = arguments[0];
//   int NumUsedNodes = arguments[1];
   int max_sending_queue = arguments[2];
   int max_receiving_queue = arguments[3];
   fTestStartTime = arguments[4];
   fTestStopTime = arguments[5];
   int patternid = arguments[6];
   double datarate = arguments[7];
   // double ratemeterinterval = arguments[8];
   // bool canskipoperation = arguments[9] > 0;

   DOUT2(("ActivateAllToAll - start"));

   fSendRate.Reset();
   fRecvRate.Reset();
   fWorkRate.Reset();

   for (int lid=0;lid<NumLids();lid++)
      for (int n=0;n<NumNodes();n++) {
         if (SendQueue(lid,n)>0) EOUT(("!!!! Problem in SendQueue[%d,%d]=%d",lid,n,SendQueue(lid,n)));
         if (RecvQueue(lid,n)>0) EOUT(("!!!! Problem in RecvQueue[%d,%d]=%d",lid,n,RecvQueue(lid,n)));
      }

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
         fSendSch.Allocate(NumNodes()-1, NumNodes());
         fSendSch.FillRoundRoubin(0, schstep);
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

   fSendBaseTime = fTestStartTime;
   fRecvBaseTime = fTestStartTime;
   fSendSlotIndx = -1;
   fRecvSlotIndx = -1;

   fDoSending = fSendSch.ShiftToNextOperation(Node(), fSendBaseTime, fSendSlotIndx);
   fDoReceiving = fRecvSch.ShiftToNextOperation(Node(), fRecvBaseTime, fRecvSlotIndx);

   fSendStartTime.Reset();
   fSendComplTime.Reset();
   fRecvComplTime.Reset();
   fOperBackTime.Reset();

   DOUT2(("ActiavteAllToAll: Starting dosend %s dorecv %s remains: %5.3fs", DBOOL(fDoSending), DBOOL(fDoReceiving), fTestStartTime - fStamping()));

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

   fRunnable->ConfigQueueLimits(max_sending_queue, max_receiving_queue);

   fRunnable->ResetStatistic();

   fAllToAllActive = true;

   ShootTimer("AllToAll", 0.001);

   DOUT0(("Activate ALL-to-ALL"));
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
   DOUT3(("Do recv %s curr_tm %8.7f next_tm %8.7f slot %d node %d lid %d queue %d",
         DBOOL(fDoReceiving), curr_tm, fRecvBaseTime + fRecvSch.timeSlot(fRecvSlotIndx), fRecvSlotIndx,
         fRecvSch.Item(fRecvSlotIndx, Node()).node, fRecvSch.Item(fRecvSlotIndx, Node()).lid,
         RecvQueue(fRecvSch.Item(fRecvSlotIndx, Node()).lid, fRecvSch.Item(fRecvSlotIndx, Node()).node)));
   DOUT3(("Do send %s curr_tm %8.7f next_tm %8.7f slot %d node %d lid %d queue %d",
         DBOOL(fDoSending), curr_tm, fSendBaseTime + fSendSch.timeSlot(fSendSlotIndx), fSendSlotIndx,
         fSendSch.Item(fSendSlotIndx, Node()).node, fSendSch.Item(fSendSlotIndx, Node()).lid,
         SendQueue(fSendSch.Item(fSendSlotIndx, Node()).lid, fSendSch.Item(fSendSlotIndx, Node()).node)));

   DOUT3(("Send operations diff %ld: submited %ld, completed %ld", fNumSendPackets-fNumComplSendPackets, fNumSendPackets, fNumComplSendPackets));

   DOUT5(("CPU utilization = %5.1f % ", fCpuStat.CPUutil()*100.));

   DOUT0(("Send start time min:%8.6f aver:%8.6f max:%8.6f cnt:%ld", fSendStartTime.Min(), fSendStartTime.Mean(), fSendStartTime.Max(), fSendStartTime.Number()));

   if (fNumSendPackets!=fNumComplSendPackets) {
      EOUT(("Mismatch %d between submitted %ld and completed send operations %ld",fNumSendPackets-fNumComplSendPackets, fNumSendPackets, fNumComplSendPackets));

      for (int lid=0;lid<NumLids(); lid++)
        for (int node=0;node<NumNodes();node++) {
          if ((node==Node()) || !fActiveNodes[node]) continue;
          DOUT5(("   Lid:%d Node:%d RecvQueue = %d SendQueue = %d recvskp %d", lid, node, RecvQueue(lid,node), SendQueue(lid,node), fRecvSkipCounter(lid, node)));
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

   DOUT0(("  # |      Node |   Send |   Recv |   Start |S_Compl|R_Compl| Lost | Skip | Loop | Max ms | CPU  | Multicast "));
   double sum1send(0.), sum2recv(0.), sum3multi(0.), sum4cpu(0.);
   int sumcnt = 0;
   bool isok = true;
   bool isskipok = true;
   double totallost = 0, totalrecv = 0;
   double totalmultilost = 0, totalmulti = 0, totalskipped = 0;

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

       DOUT0(("%3d |%10s |%7.1f |%7.1f |%8.1f |%6.0f |%6.0f |%5.0f |%s |%5.2f |%7.2f |%s |%5.1f (%5.0f)",
             n, dabc::mgr()->GetNodeName(n).c_str(),
           allres[n*setsize+0], allres[n*setsize+1], allres[n*setsize+2]*1e6, allres[n*setsize+3]*1e6, allres[n*setsize+13]*1e6, allres[n*setsize+4], sbuf1, allres[n*setsize+10]*1e6, allres[n*setsize+11]*1e3, cpuinfobuf, allres[n*setsize+8], allres[n*setsize+7]));
       totallost += allres[n*setsize+4];
       totalrecv += allres[n*setsize+5];
       totalmultilost += allres[n*setsize+6];
       totalmulti += allres[n*setsize+7];

       // check if send starts in time
       if (allres[n*setsize+2] > 20.) isok = false;

       sumcnt++;
       sum1send += allres[n*setsize+0];
       sum2recv += allres[n*setsize+1];
       sum3multi += allres[n*setsize+8]; // Multi rate
       sum4cpu += allres[n*setsize+9]; // CPU util
       totalskipped += allres[n*setsize+12];
       if (allres[n*setsize+12]>0.03) isskipok = false;
   }
   DOUT0(("          Total |%7.1f |%7.1f | Skip: %4.0f/%4.0f = %3.1f percent",
          sum1send, sum2recv, totallost, totalrecv, 100*(totalrecv>0. ? totallost/(totalrecv+totallost) : 0.)));
   if (totalmulti>0)
      DOUT0(("Multicast %4.0f/%4.0f = %7.6f%s  Rate = %4.1f",
         totalmultilost, totalmulti, totalmultilost/totalmulti*100., "%", sum3multi/sumcnt));
}


bool bnet::TransportModule::ProcessAllToAllAction()
{
   if (!fAllToAllActive) return false;

   double start_tm = 0;

   // when receive operation should be prepared before send is started
   // we believe that 100 microsec is enough (it is latency of runnable)
   double recv_pre_time = 0.0001;

   // this is time required to deliver operation to the runnable
   // 5 ms should be enough to be able to use wait in the main loop
   // this is latency of module
   double submit_pre_time = 0.005;

   double next_recv_time(0.), next_send_time(0.), next_oper_time(0.);

   int maxcnt(5);

   while ((next_oper_time<=0.) && (maxcnt-->0)) {

      next_oper_time = 0.;

      double curr_tm = fStamping();
      if (start_tm==0.) start_tm = curr_tm;

      if (curr_tm>fTestStopTime) {
         fAllToAllActive = false;
         DOUT0(("All-to-all processing done"));
         fSendStartTime.Show("SendStartTime", true);
         fOperBackTime.Show("OperBackTime", true);
         return false;
      }

      if (curr_tm > start_tm + 0.03) {
         EOUT(("Too long in the loop"));
         return true;
      }

      if (fDoReceiving && fRunnable->IsFreeRec() && (curr_tm<fTestStopTime-0.1))
         next_recv_time = fRecvBaseTime + fRecvSch.timeSlot(fRecvSlotIndx) - recv_pre_time;

      if (fDoSending && fRunnable->IsFreeRec() && (curr_tm<fTestStopTime-0.1))
         next_send_time = fSendBaseTime + fSendSch.timeSlot(fSendSlotIndx);

      // if both operation want to be executed, selected first in the time
      if (next_recv_time>0. && next_send_time>0.) {
         if (next_recv_time<next_send_time)
            next_send_time = 0.;
         else
            next_recv_time = 0.;
      }

      // this is submit of recv operation

      if (next_recv_time>0.) {
         if (next_recv_time - submit_pre_time > curr_tm) {
            next_oper_time = next_recv_time;
         } else {
            int node = fRecvSch.Item(fRecvSlotIndx, Node()).node;
            int lid = fRecvSch.Item(fRecvSlotIndx, Node()).lid;
            bool did_submit = false;

            if ((fRecvSkipCounter(lid, node) > 0) && (RecvQueue(lid, node)>0)) {
               // if packets over this channel were lost, we should not try
               // to receive them - they were skipped by sender most probably
               fRecvSkipCounter(lid, node)--;
               did_submit = true;
            } else {

               dabc::Buffer buf = FindPool("BnetDataPool")->TakeBuffer(fTestBufferSize);

               if (buf.null()) {
                  EOUT(("Not enough buffers"));
                  return false;
               }

               // DOUT0(("Submit recv operation for buffer %u", buf.GetTotalSize()));

               int recid = fRunnable->PrepareOperation(kind_Recv, sizeof(TransportHeader), buf);
               OperRec* rec = fRunnable->GetRec(recid);

               if (rec==0) {
                  EOUT(("Cannot prepare recv operation"));
                  return false;
               }

               rec->SetTarget(node, lid);
               rec->SetTime(next_recv_time);

               if (!SubmitToRunnable(recid,rec)) {
                  EOUT(("Cannot submit receive operation to lid %d node %d", lid, node));
                  return false;
               }

               // DOUT1(("SubmitRecvOper recid %d node %d lid %d", recid, node, lid));

               did_submit = true;
            }

            // shift to next operation, even if previous was not submitted.
            // dangerous but it is like it is - we should continue receiving otherwise input will be blocked
            // we should do here more smart solutions

            if (did_submit)
               fDoReceiving = fRecvSch.ShiftToNextOperation(Node(), fRecvBaseTime, fRecvSlotIndx);
         }
      }

      // this is submit of send operation
      if (next_send_time > 0.) {
         if (next_send_time - submit_pre_time > curr_tm) {
            next_oper_time = next_send_time;
         } else {
            int node = fSendSch.Item(fSendSlotIndx, Node()).node;
            int lid = fSendSch.Item(fSendSlotIndx, Node()).lid;
            bool did_submit = false;

            if (/*canskipoperation &&*/ false /*  ((SendQueue(lid, node) >= max_sending_queue) || (TotalSendQueue() >= 2*max_sending_queue))*/ ) {
               // TODO: one could skip operation if time is far away from scheduled
               // TODO: probably, one should implement skip in runnable
               did_submit = true;
               fSkipSendCounter++;
            } else {

               dabc::Buffer buf = FindPool("BnetDataPool")->TakeBuffer(fTestBufferSize);

               if (buf.null()) {
                  EOUT(("Not enough buffers"));
                  return false;
               }

               // DOUT0(("Submit send operation for buffer %u", buf.GetTotalSize()));

               double mem[2] = { curr_tm, fSendOperCounter(lid,node) };
               buf.CopyFrom(mem, sizeof(mem));

               int recid = fRunnable->PrepareOperation(kind_Send, sizeof(TransportHeader), buf);
               OperRec* rec = fRunnable->GetRec(recid);

               if (rec==0) {
                  EOUT(("Cannot prepare send operation recid = %d", recid));
                  return true;
               }

               TransportHeader* hdr = (TransportHeader*) rec->header;
               hdr->srcid = Node();    // source node id
               hdr->tgtid = node;      // target node id
               hdr->eventid = 123456;  // event id

               rec->SetTarget(node, lid);
               rec->SetTime(next_send_time);

               if (!SubmitToRunnable(recid, rec)) {
                  EOUT(("Cannot submit receive operation to lid %d node %d", lid, node));
                  return false;
               }

               // DOUT1(("SubmitSendOper recid %d node %d lid %d", recid, node, lid));

               fNumSendPackets++;
               did_submit = true;
            }

            // shift to next operation, in some cases even when previous operation was not performed
            if (did_submit) {
               fSendOperCounter(lid,node)++; // this indicates that we perform operation and moved to next counter
               fDoSending = fSendSch.ShiftToNextOperation(Node(), fSendBaseTime, fSendSlotIndx);
            }
         }
      }
   }

   return true;
}

int bnet::TransportModule::ProcessAskQueue(void* tgt)
{
   // method is used to calculate how many queues entries are existing

   dabc::TimeStamp tm = dabc::Now();

   while (!tm.Expired(0.1)) {
      int recid = -1; // CheckCompletionQueue(0.001);
      if (recid<0) break;
      fRunnable->ReleaseRec(recid);
   }
   int32_t* mem = (int32_t*) tgt;
   for(int node=0;node<NumNodes();node++)
      for(int nlid=0;nlid<NumLids();nlid++) {
         *mem++ = SendQueue(nlid, node);
         *mem++ = RecvQueue(nlid, node);
      }

   return NumNodes()*NumLids()*2*sizeof(int32_t);
}

bool bnet::TransportModule::MasterCleanup()
{
   double tm = fStamping();

   int blocksize = NumNodes()*NumLids()*2*sizeof(uint32_t);

   void* recs = malloc(NumNodes() * blocksize);
   memset(recs, 0, NumNodes() * blocksize);

   // own records will be add automatically
//   if (!MasterCommandRequest(BNET_CMD_ASKQUEUE, 0, 0, recs, blocksize)) {
//      EOUT(("Cannot collect queue sizes"));
//      free(recs);
//      return false;
//   }

   // now we should resort connection records, loop over targets

   DOUT0(("First collect all queue sizes"));

   void* conn = malloc(NumNodes() * blocksize);
   memset(conn, 0, NumNodes()*blocksize);

   int32_t* src = (int32_t*) recs;
   int32_t* tgt = (int32_t*) conn;

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

   bool res(true);
//   if ((allsend!=0) || (allrecv!=0))
//       res = MasterCommandRequest(BNET_CMD_CLEANUP, conn, -blocksize);

   free(recs);

   free(conn);

   // for syncronisation
//   if (!MasterCommandRequest(BNET_CMD_TEST)) res = false;

   DOUT0(("Queues are cleaned in %5.4f s res = %s", fStamping() - tm, DBOOL(res)));

   return res;
}

bool bnet::TransportModule::ProcessCleanup(int32_t* pars)
{
   bool isany(true);

   int maxqueue(5);

   while (isany) {
      isany = false;
      for (int node=0;node<NumNodes();node++)
         for (int lid=0;lid<NumLids();lid++) {
            int* entry = pars + node*NumLids()*2 + lid*2;

            if ((entry[0]>0) || (entry[1]>0)) isany = true;

            if ((entry[0]>0) && (SendQueue(lid,node)<maxqueue)) {
               // perform send operation, buffer can be empty
               dabc::Buffer buf;

               int recid = fRunnable->PrepareOperation(kind_Send, sizeof(TransportHeader), buf);
               OperRec* rec = fRunnable->GetRec(recid);

               if (rec==0) {
                  EOUT(("Cannot prepare send operation"));
                  return false;
               }

               TransportHeader* hdr = (TransportHeader*) rec->header;
               hdr->srcid = Node();    // source node id
               hdr->tgtid = node;      // target node id
               hdr->eventid = 123456;  // event id

               rec->SetTarget(node, lid);
               rec->SetImmediateTime();

               if (!SubmitToRunnable(recid,rec)) {
                  EOUT(("Cannot submit receive operation to lid %d node %d", lid, node));
                  return false;
               }

               entry[0]--;
            }

            if ((entry[1]>0) && (RecvQueue(lid,node)<maxqueue)) {
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

               if (!SubmitToRunnable(recid,rec)) {
                  EOUT(("Cannot submit receive operation to lid %d node %d", lid, node));
                  return false;
               }

               entry[1]--;
            }
         }
   }

   return true;
}


void bnet::TransportModule::ProcessInputEvent(dabc::Port* port)
{
   unsigned portid = IOPortNumber(port);

   if (IsSlave() && (portid==0)) {
      ProcessNextSlaveInputEvent();
   } else
   if (IsMaster() && ((int)portid<NumNodes()-1)) {
      if (!port->CanRecv()) return;
      dabc::Buffer buf;
      buf << port->Recv();
      ProcessReplyBuffer(portid+1, buf);
   }
}

void bnet::TransportModule::ProcessOutputEvent(dabc::Port* port)
{

}

void bnet::TransportModule::ProcessNextSlaveInputEvent()
{
   // we cannot accept next command until previous is not yet executed
   if (fRunningCmdId>0) return;

   dabc::Port* port = IOPort(0);

   if (!port->CanRecv()) return;

   dabc::Buffer buf;
   buf << port->Recv();
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

   dabc::Buffer buf0;

   for(int node=0;node<NumNodes();node++) if (fActiveNodes[node]) {

      dabc::Buffer buf = FindPool("BnetCtrlPool")->TakeBuffer(fullpacketsize);
      if (buf.null()) { EOUT(("No empty buffer")); return false; }

      CommandMessage* msg = (CommandMessage*) buf.GetPointer()();

      msg->magic = BNET_CMD_MAGIC;
      msg->cmdid = cmdid;
      msg->cmddatasize = cmddatasize;
      msg->delay = 0;
      msg->getresults = resultpernode;
      if ((cmddatasize>0) && (cmddata!=0)) {
         if (incremental_data)
            memcpy(msg->cmddata(), (uint8_t*) cmddata + node * cmddatasize, cmddatasize);
         else
            memcpy(msg->cmddata(), cmddata, cmddatasize);
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

   fCmdReplies.resize(0);
   fCmdReplies.resize(NumNodes(), false);

   fCmdAllResults = allresults;   // buffer where replies from all nodes collected
   fCmdResultsPerNode = resultpernode;  // how many data in replied is expected

   fRunningCmdId = cmdid;
   fCmdStartTime.GetNow();
   fCmdEndTime = fCmdStartTime + fCurrentCmd.GetTimeout(); // time will be expired in 5 seconds

   ProcessReplyBuffer(0, buf0);

   return true;
}


bool bnet::TransportModule::ProcessReplyBuffer(int nodeid, dabc::Buffer buf)
{
   if (buf.null()) { EOUT(("Empty buffer")); return false; }
   if (fCmdReplies[nodeid]) { EOUT(("Node %d already replied command", nodeid)); return false; }

   fCmdReplies[nodeid] = true;

   CommandMessage* rcv = (CommandMessage*) buf.GetPointer()();

   if (rcv->magic!=BNET_CMD_MAGIC) { EOUT(("Wrong magic")); return false; }
   if (rcv->cmdid != fRunningCmdId) { EOUT(("Wrong ID recv %d  cmd %d", rcv->cmdid, fRunningCmdId)); return false; }
   if (rcv->node != nodeid) { EOUT(("Wrong node")); return false; }

   if ((fCmdAllResults!=0) && (fCmdResultsPerNode>0) && (rcv->cmddatasize == fCmdResultsPerNode)) {
      // DOUT0(("Copy from node %d buffer %d", nodeid, resultpernode));
      memcpy((uint8_t*)fCmdAllResults + nodeid*fCmdResultsPerNode, rcv->cmddata(), fCmdResultsPerNode);
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

         // DOUT0(("Process user event kind:%d", rec->kind));

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
      if ((TotalRecvQueue()==0) && (TotalSendQueue()==0)) ShootTimer("Timer", 0.);

   } else {
      EOUT(("User event from unknown item"));
   }
}

void bnet::TransportModule::ProcessSendCompleted(int recid, OperRec* rec)
{
   if (!fAllToAllActive) return;

   // DOUT1(("ProcessSendCompleted recid %d err %s", recid, DBOOL(rec->err)));

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

   // DOUT1(("ProcessRecvCompleted recid %d err %s", recid, DBOOL(rec->err)));

   double mem[2] = {0., 0.};
   rec->buf.CopyTo(mem, sizeof(mem));
   double sendtime = mem[0];
   int sendcnt = (int) mem[1];

   int lost = sendcnt - fRecvOperCounter(rec->tgtindx, rec->tgtnode) - 1;
   if (lost>0) {
      fNumLostPackets += lost;
      fRecvSkipCounter(rec->tgtindx, rec->tgtnode) += lost;
   }

   fRecvOperCounter(rec->tgtindx, rec->tgtnode) = sendcnt;

   double curr_tm = fStamping();

   fRecvComplTime.Fill(rec->compl_time - sendtime);

   fOperBackTime.Fill(curr_tm - rec->compl_time);

   fRecvRate.Packet(fTestBufferSize, curr_tm);

   fTotalRecvPackets++;
}


void bnet::TransportModule::ProcessTimerEvent(dabc::Timer* timer)
{
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

         DOUT0(("First collect all QPs info"));

         // own records will be add automatically
         RequestMasterCommand(BNET_CMD_CREATEQP, 0, 0, fCollectBuffer, blocksize);
         break;
      }

      case BNET_CMD_CONNECTQP: {

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


         double arguments[10];

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

