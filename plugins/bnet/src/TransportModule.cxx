#include "bnet/TransportModule.h"

#include "bnet/defines.h"

#include <vector>
#include <math.h>

#include "dabc/timing.h"
#include "dabc/logging.h"
#include "dabc/Queue.h"
#include "dabc/Manager.h"
#include "dabc/PoolHandle.h"
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
   if (fNumLids>IBTEST_MAXLID) fNumLids = IBTEST_MAXLID;

   int nports = cmd.Field("NumPorts").AsInt(1);

   fTestKind = Cfg("TestKind", cmd).AsStdStr();

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

   for (int n=0;n<IBTEST_MAXLID;n++) {
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

   // new members

   fRunningCmdId = 0;
   fCmdState = mcmd_None;
   fCmdEndTime.Reset();  // time when command should finish its execution
   fCmdReplies.resize(0); // indicates if cuurent cmd was replied
   fCmdAllResults = 0;   // buffer where replies from all nodes collected
   fCmdResultsPerNode = 0;  // how many data in replied is expected
   fConnRawData = 0;
   fConnSortData = 0;

   CreateTimer("Timer", 0.1); // every 100 milisecond

   if (IsMaster()) {
      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_WAIT, 0.3)); // master wait 0.3 seconds
      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_ACTIVENODES)); // first collect active nodes
      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_TEST)); // test connection
      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_MEASURE)); // reset first measurement

      for (int n=0;n<10;n++) fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_TEST)); // test connection
      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_MEASURE)); // do real measurement of test loop

      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_CREATEQP));
      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_CONNECTQP));
      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_CONNECTDONE));
      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_TEST)); // test that slaves are there
      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_MEASURE)); // reset single measurement

      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_WAIT, 1.)); // master wait 1 second

      TransportCmd cmd_sync1(IBTEST_CMD_TIMESYNC, 5.);
      cmd_sync1.SetSyncArg(200, true, false);
      fCmdsQueue.PushD(cmd_sync1);
      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_EXECSYNC));

      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_WAIT, 10.)); // master wait 10 seconds

      TransportCmd cmd_sync2(IBTEST_CMD_TIMESYNC, 5.);
      cmd_sync2.SetSyncArg(200, true, true);
      fCmdsQueue.PushD(cmd_sync2);
      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_EXECSYNC));
      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_GETSYNC));

      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_WAIT, 10.)); // master wait 10 seconds

      TransportCmd cmd_sync3(IBTEST_CMD_TIMESYNC, 5.);
      cmd_sync3.SetSyncArg(200, false, false);
      fCmdsQueue.PushD(cmd_sync3);
      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_EXECSYNC));

      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_CLOSEQP)); // close connections
      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_TEST)); // test that slaves are there

      fCmdsQueue.PushD(TransportCmd(IBTEST_CMD_EXIT)); // master wait 1 seconds
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

   for (int n=0;n<IBTEST_MAXLID;n++) {
      if (fSendQueue[n]) delete [] fSendQueue[n];
      if (fRecvQueue[n]) delete [] fRecvQueue[n];
   }


   AllocResults(0);
}

void bnet::TransportModule::AllocResults(int size)
{
   if (fResults!=0) delete[] fResults;
   fResults = 0;
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


bool bnet::TransportModule::MasterCommandRequest(int cmdid, void* cmddata, int cmddatasize, void* allresults, int resultpernode)
{
    // Request from master side to move in specific command
    // Actually, appropriate functions should run at about the
    // same time while special delay is calculated before and informed
    // each slave how long it should be delayed

   // if module not working, do not try to do anything else
/*

   if (!ModuleWorking(0.)) {
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

      dabc::Buffer buf = TakeBuffer(FindPool("BnetCtrlPool"), fullpacketsize, 1.);
      if (buf.null()) { EOUT(("No empty buffer")); return false; }

      CommandMessage* msg = (CommandMessage*) buf.GetPointer()();

      msg->magic = IBTEST_CMD_MAGIC;
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
         Send(IOPort(node-1), buf, 1.);
      }
   }

   PreprocessTransportCommand(buf0);

   std::vector<bool> replies;
   replies.resize(NumNodes(), false);
   bool finished = false;

   while (ModuleWorking() && !finished) {
      dabc::Buffer buf;
      buf << buf0;
      int nodeid = 0;

      if (buf.null()) {
         dabc::Port* port(0);
         nodeid = -1;
         buf = RecvFromAny(&port, 4.);
         if (port) nodeid = IOPortNumber(port) + 1;
      }

      if (buf.null() || (nodeid<0)) {
         for (unsigned n=0;n<replies.size();n++)
            if (fActiveNodes[n] && !replies[n]) EOUT(("Cannot get any reply from node %u", n));
         return false;
      }

//      DOUT0(("Recv reply from slave %d", nodeid));

      replies[nodeid] = true;

      CommandMessage* rcv = (CommandMessage*) buf.GetPointer()();

      if (rcv->magic!=IBTEST_CMD_MAGIC) { EOUT(("Wrong magic")); return false; }
      if (rcv->cmdid!=cmdid) { EOUT(("Wrong ID")); return false; }
      if (rcv->node != nodeid) { EOUT(("Wrong node")); return false; }

      if ((allresults!=0) && (resultpernode>0) && (rcv->cmddatasize == resultpernode)) {
         // DOUT0(("Copy from node %d buffer %d", nodeid, resultpernode));
         memcpy((uint8_t*)allresults + nodeid*resultpernode, rcv->cmddata(), resultpernode);
      }

      finished = true;

      for (unsigned n=0;n<replies.size();n++)
         if (fActiveNodes[n] && !replies[n]) finished = false;
   }

   // Should we call slave method here ???
*/
   return true;
}

bool bnet::TransportModule::MasterCollectActiveNodes()
{
   uint8_t buff[NumNodes()];

   int cnt = 1;
   buff[0] = 1;

   for(int node=1;node<NumNodes();node++) {
      bool active = IOPort(node-1)->IsConnected();
      if (active) cnt++;
      buff[node] = active ? 1 : 0;
   }

   DOUT0(("There are %d active nodes, %d missing", cnt, NumNodes() - cnt));

   if (!MasterCommandRequest(IBTEST_CMD_ACTIVENODES, buff, NumNodes())) return false;

   return cnt == NumNodes();
}


bool bnet::TransportModule::CalibrateCommandsChannel(int nloop)
{
   dabc::TimeStamp tm;

   dabc::Average aver;

   for (int n=0;n<nloop;n++) {
      dabc::Sleep(0.001);

      tm = dabc::Now();
      if (!MasterCommandRequest(IBTEST_CMD_TEST)) return false;

      if (n>0)
         aver.Fill(dabc::Now()-tm);
   }

   fCmdDelay = aver.Mean();

   aver.Show("Command loop", true);

   DOUT0(("Command loop = %5.3f ms", fCmdDelay*1e3));

   return true;
}

bool bnet::TransportModule::MasterCallExit()
{
   MasterCommandRequest(IBTEST_CMD_EXIT);

   fRunnable->StopRunnable();
   fRunThread->Join();

   dabc::mgr.StopApplication();

   return true;
}

int bnet::TransportModule::PreprocessTransportCommand(dabc::Buffer& buf)
{
   if (buf.null()) return IBTEST_CMD_EXIT;

   CommandMessage* msg = (CommandMessage*) buf.SegmentPtr(0);

   if (msg->magic!=IBTEST_CMD_MAGIC) {
       EOUT(("Magic id is not correct"));
       return IBTEST_CMD_EXIT;
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
      case IBTEST_CMD_TEST:
         cmd_res = true;
         break;

      case IBTEST_CMD_EXIT:
         WorkerSleep(0.1);
         DOUT2(("Propose worker to stop"));
         // Stop(); // stop module

         fRunnable->StopRunnable();
         fRunThread->Join();

         dabc::mgr.StopApplication();
         cmd_res = true;
         break;

      case IBTEST_CMD_ACTIVENODES: {
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

      case IBTEST_CMD_COLLECT:
         if ((msg->getresults > 0) && (fResults!=0)) {
            msg->cmddatasize = msg->getresults;
            sendpacketsize += msg->cmddatasize;
            memcpy(msg->cmddata(), fResults, msg->getresults);
            cmd_res = true;
         }
         break;

      case IBTEST_CMD_CREATEQP: {
         int ressize = msg->getresults;
         cmd_res = fRunnable->CreateQPs(msg->cmddata(), ressize);
         msg->cmddatasize = ressize;
         sendpacketsize += ressize;
         break;
      }

      case IBTEST_CMD_CONNECTQP:
         cmd_res = fRunnable->ConnectQPs(fCmdDataBuffer, fCmdDataSize);
         break;

      case IBTEST_CMD_CLEANUP:
         cmd_res = ProcessCleanup((int32_t*)fCmdDataBuffer);
         break;

      case IBTEST_CMD_CLOSEQP:
         cmd_res = fRunnable->CloseQPs();
         break;

      case IBTEST_CMD_ASKQUEUE:
         msg->cmddatasize = ProcessAskQueue(msg->cmddata());
         sendpacketsize += msg->cmddatasize;
         cmd_res = true;
         break;

      case IBTEST_CMD_TIMESYNC:
         // here we just copy arguments for sync command
         if (sizeof(fSyncArgs) != fCmdDataSize) {
            EOUT(("time sync buffer not match!!!"));
            exit(5);
         }
         memcpy(fSyncArgs, fCmdDataBuffer, sizeof(fSyncArgs));
         cmd_res = true;
         break;

      case IBTEST_CMD_GETSYNC:
         fRunnable->GetSync(fStamping);
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

bool bnet::TransportModule::MasterCloseConnections()
{
   dabc::TimeStamp tm = dabc::Now();

   if (!MasterCommandRequest(IBTEST_CMD_CLOSEQP)) return false;

   // this is just ensure that all nodes are on ready state
   if (!MasterCommandRequest(IBTEST_CMD_TEST)) return false;

   DOUT0(("Comm ports are closed in %5.3fs", tm.SpentTillNow()));

   return true;
}


void bnet::TransportModule::ActivateAllToAll(double*)
{
//   int bufsize = arguments[0];
//   int NumUsedNodes = arguments[1];
//   int max_sending_queue = arguments[2];
//   int max_receiving_queue = arguments[3];
//   double starttime = arguments[4];
//   double stoptime = arguments[5];
//   int patternid = arguments[6];
//  double datarate = arguments[7];
//   double ratemeterinterval = arguments[8];
//   bool canskipoperation = arguments[9] > 0;

}


bool bnet::TransportModule::ExecuteAllToAll(double* arguments)
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
   bool canskipoperation = arguments[9] > 0;

   AllocResults(14);

   DOUT2(("ExecuteAllToAll - start"));

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

   DOUT4(("ExecuteAllToAll: Define schedule"));

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


   dabc::Average send_start, send_compl, recv_compl, loop_time, wait_time_aver, submit_send;

   DOUT2(("ExecuteAllToAll: Starting dosend %s dorecv %s remains: %5.3fs", DBOOL(fDoSending), DBOOL(fDoReceiving), fTestStartTime - fStamping()));

   // counter for must have send operations over each channel
   IbTestIntMatrix sendcounter(NumLids(), NumNodes());

   // actual received id from sender
   IbTestIntMatrix recvcounter(NumLids(), NumNodes());

   // number of recv skip operations one should do
   IbTestIntMatrix recvskipcounter(NumLids(), NumNodes());

   sendcounter.Fill(0);
   recvcounter.Fill(-1);
   recvskipcounter.Fill(0);

   long numlostpackets(0);

   long totalrecvpackets(0), numsendpackets(0), numcomplsend(0);

   long  loop_cnt(0), no_recv_cnt(0), no_send_cnt(0);

   int64_t sendmulticounter = 0;
   int64_t skipmulticounter = 0;
   int64_t numlostmulti = 0;
   int64_t totalrecvmulti = 0;
   int64_t skipsendcounter = 0;

   double last_tm(-1), curr_tm(0);

   dabc::CpuStatistic cpu_stat;

   int recid(-1);
   OperRec* rec(0);

   fRunnable->ConfigQueueLimits(max_sending_queue, max_receiving_queue);

   fRunnable->ResetStatistic();

   double next_oper_time(0.);

   // when receive operation should be prepared before send is started
   // we believe that 100 microsec is enough
   double recv_pre_time = 0.0001;

   // this is time required to deliver operation to the runnable
   // 5 ms should be enough to be able to use wait in the main loop
   double submit_pre_time = 0.005;

   while ((curr_tm=fStamping()) < fTestStopTime) {

      loop_cnt++;

      if ((last_tm>0) && (last_tm>fTestStartTime))
         loop_time.Fill(curr_tm - last_tm);
      last_tm = curr_tm;

      // just indicate that we were active at this time interval
      fWorkRate.Packet(1, curr_tm);

      // wait only when next operation far away
      double waittm = 0.;
      if ((next_oper_time>0) && (next_oper_time > curr_tm+0.003)) waittm = 0.001;

//      if (waittm>0) sched_yield();

      recid = -1; // CheckCompletionQueue(waittm);
      if (waittm>0) {
         double next_tm = fStamping();
         wait_time_aver.Fill(next_tm - curr_tm);
         curr_tm = next_tm;
      }


      next_oper_time = 0;

      rec = fRunnable->GetRec(recid);

      if (rec!=0) {
         switch (rec->kind) {
            case kind_Send: {
               double sendtime(0);
               rec->buf.CopyTo(&sendtime, sizeof(sendtime));
               numcomplsend++;

               double sendcompltime = fStamping();

               send_start.Fill(rec->is_time - rec->oper_time);

               send_compl.Fill(sendcompltime - sendtime);

               fSendRate.Packet(fTestBufferSize, sendcompltime);
               break;
            }
            case kind_Recv: {
               double mem[2] = {0., 0.};
               rec->buf.CopyTo(mem, sizeof(mem));
               double sendtime = mem[0];
               int sendcnt = (int) mem[1];

               int lost = sendcnt - recvcounter(rec->tgtindx, rec->tgtnode) - 1;
               if (lost>0) {
                  numlostpackets += lost;
                  recvskipcounter(rec->tgtindx, rec->tgtnode) += lost;
               }

               recvcounter(rec->tgtindx, rec->tgtnode) = sendcnt;

               double recv_time = fStamping();

               recv_compl.Fill(recv_time - sendtime);

               fRecvRate.Packet(fTestBufferSize, recv_time);

               totalrecvpackets++;
               break;
            }
            default:
               EOUT(("Wrong operation kind"));
               break;
         }

         // buffer will be released as well
         fRunnable->ReleaseRec(recid);
      }

      double next_recv_time(0.), next_send_time(0.);

      if (fDoReceiving && fRunnable->IsFreeRec() && (curr_tm<fTestStopTime-0.1))
         next_recv_time = fRecvBaseTime + fRecvSch.timeSlot(fRecvSlotIndx) - recv_pre_time;

      if (fDoSending && fRunnable->IsFreeRec() && (curr_tm<fTestStopTime-0.1))
         next_send_time = fSendBaseTime + fSendSch.timeSlot(fSendSlotIndx);

      if (fDoReceiving && !fRunnable->IsFreeRec()) no_recv_cnt++;
      if (fDoSending && !fRunnable->IsFreeRec()) no_send_cnt++;

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

            if ((recvskipcounter(lid, node) > 0) && (RecvQueue(lid, node)>0)) {
               // if packets over this channel were lost, we should not try
               // to receive them - they were skipped by sender most probably
               recvskipcounter(lid, node)--;
               did_submit = true;
            } else {

               dabc::Buffer buf = FindPool("BnetDataPool")->TakeBuffer(fTestBufferSize);

               if (buf.null()) {
                  EOUT(("Not enough buffers"));
                  return false;
               }

               // DOUT0(("Submit recv operation for buffer %u", buf.GetTotalSize()));

               recid = fRunnable->PrepareOperation(kind_Recv, sizeof(TransportHeader), buf);
               rec = fRunnable->GetRec(recid);

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

            if (canskipoperation && false /*  ((SendQueue(lid, node) >= max_sending_queue) || (TotalSendQueue() >= 2*max_sending_queue))*/ ) {
               // TODO: one could skip operation if time is far away from scheduled
               // TODO: probably, one should implement skip in runnable
               did_submit = true;
               skipsendcounter++;
            } else {

               dabc::Buffer buf = FindPool("BnetDataPool")->TakeBuffer(fTestBufferSize);

               if (buf.null()) {
                  EOUT(("Not enough buffers"));
                  return false;
               }

               // DOUT0(("Submit send operation for buffer %u", buf.GetTotalSize()));

               double mem[2] = { curr_tm, sendcounter(lid,node) };
               buf.CopyFrom(mem, sizeof(mem));

               recid = fRunnable->PrepareOperation(kind_Send, sizeof(TransportHeader), buf);
               rec = fRunnable->GetRec(recid);

               if (rec==0) {
                  EOUT(("Cannot prepare send operation"));
                  return false;
               }

               TransportHeader* hdr = (TransportHeader*) rec->header;
               hdr->srcid = Node();    // source node id
               hdr->tgtid = node;      // target node id
               hdr->eventid = 123456;  // event id

               rec->SetTarget(node, lid);
               rec->SetTime(next_send_time);

               submit_send.Fill(next_send_time - curr_tm);

               if (!SubmitToRunnable(recid,rec)) {
                  EOUT(("Cannot submit receive operation to lid %d node %d", lid, node));
                  return false;
               }

               numsendpackets++;
               did_submit = true;
            }

            // shift to next operation, in some cases even when previous operation was not performed
            if (did_submit) {
               sendcounter(lid,node)++; // this indicates that we perform operation and moved to next counter
               fDoSending = fSendSch.ShiftToNextOperation(Node(), fSendBaseTime, fSendSlotIndx);
            }
         }
      }
   }

   cpu_stat.Measure();

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

   DOUT3(("Send operations diff %ld: submited %ld, completed %ld", numsendpackets-numcomplsend, numsendpackets, numcomplsend));

   DOUT5(("CPU utilization = %5.1f % ", cpu_stat.CPUutil()*100.));

   DOUT0(("Send submit time min:%8.6f aver:%8.6f max:%8.6f cnt:%ld", submit_send.Min(), submit_send.Mean(), submit_send.Max(), submit_send.Number()));
   DOUT0(("Send start time min:%8.6f aver:%8.6f max:%8.6f cnt:%ld", send_start.Min(), send_start.Mean(), send_start.Max(), send_start.Number()));
   DOUT0(("Real wait time min:%5.4f aver:%5.4f max:%5.4f cnt:%ld", wait_time_aver.Min(), wait_time_aver.Mean(), wait_time_aver.Max(), wait_time_aver.Number()));
   DOUT0(("Module loop time min:%5.4f aver:%5.4f max:%5.4f cnt:%ld", loop_time.Min(), loop_time.Mean(), loop_time.Max(), loop_time.Number()));
   DOUT0(("Loop cnt %ld  no_send:%ld %8.6f no_recv:%ld %8.6f", loop_cnt, no_send_cnt, 1.*no_send_cnt/loop_cnt, no_recv_cnt, 1.*no_recv_cnt/loop_cnt));


   if (numsendpackets!=numcomplsend) {
      EOUT(("Mismatch %d between submitted %ld and completed send operations %ld",numsendpackets-numcomplsend, numsendpackets, numcomplsend));

      for (int lid=0;lid<NumLids(); lid++)
        for (int node=0;node<NumNodes();node++) {
          if ((node==Node()) || !fActiveNodes[node]) continue;
          DOUT5(("   Lid:%d Node:%d RecvQueue = %d SendQueue = %d recvskp %d", lid, node, RecvQueue(lid,node), SendQueue(lid,node), recvskipcounter(lid, node)));
       }
   }

   fResults[0] = fSendRate.GetRate();
   fResults[1] = fRecvRate.GetRate();
   fResults[2] = send_start.Mean();
   fResults[3] = send_compl.Mean();
   fResults[4] = numlostpackets;
   fResults[5] = totalrecvpackets;
   fResults[6] = numlostmulti;
   fResults[7] = totalrecvmulti;
   fResults[8] = 0.;
   fResults[9] = cpu_stat.CPUutil();
   fResults[10] = r_mean_loop; // loop_time.Mean();
   fResults[11] = r_max_loop; // loop_time.Max();
   fResults[12] = (numsendpackets+skipsendcounter) > 0 ? 1.*skipsendcounter / (numsendpackets + skipsendcounter) : 0.;
   fResults[13] = recv_compl.Mean();

   DOUT5(("Multi: total %ld lost %ld", totalrecvmulti, numlostmulti));

   if (sendmulticounter+skipmulticounter>0)
      DOUT0(("Multi send: %ld skipped %ld (%5.3f)",
             sendmulticounter, skipmulticounter,
             100.*skipmulticounter/(1.*sendmulticounter+skipmulticounter)));

   return true;
}

bool bnet::TransportModule::MasterAllToAll(int full_pattern,
                                        int bufsize,
                                        double duration,
                                        int datarate,
                                        int max_sending_queue,
                                        int max_receiving_queue,
                                        bool fromperfmtest)
{
   // pattern == 0 round-robin schedule
   // pattern == 1 fixed target (shift always 1)
   // pattern == 2 fixed target (shift always NumNodes / 2)
   // pattern == 3 one-to-all, one node sending, other receiving
   // pattern == 4 all-to-one, all sending, one receiving
   // pattern == 5 even nodes sending data to next odd node (0->1, 2->3, 4->5 and so on)
   // pattern == 6 same as 0, but node0 will not receive data from node1 (check if rest data will be transformed)
   // pattern == 7 schedule read from the file
   // pattern = 10..17, same as 0..7, but allowed to skip send/recv packets when queue full

   bool allowed_to_skip = false;
   int pattern = full_pattern;

   if ((pattern>=0)  && (pattern<20)) {
      allowed_to_skip = full_pattern / 10 > 0;
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

   arguments[0] = bufsize;
   arguments[1] = NumNodes();
   arguments[2] = max_sending_queue;
   arguments[3] = max_receiving_queue;
   arguments[4] = fStamping() + fCmdDelay + 1.; // start 1 sec after cmd is delivered
   arguments[5] = arguments[4] + duration;      // stopping time
   arguments[6] = pattern;
   arguments[7] = datarate;
   arguments[8] = fTrendingInterval; // interval for ratemeter, 0 - off
   arguments[9] = allowed_to_skip ? 1 : 0;

   DOUT0(("====================================="));
   DOUT0(("%s pattern:%d size:%d rate:%d send:%d recv:%d nodes:%d canskip:%s",
         pattern_name, pattern, bufsize, datarate, max_sending_queue, max_receiving_queue, NumNodes(), DBOOL(allowed_to_skip)));

   if (!MasterCommandRequest(IBTEST_CMD_ALLTOALL, arguments, sizeof(arguments))) return false;

   if (!ExecuteAllToAll(arguments)) return false;

   int setsize = 14;
   double allres[setsize*NumNodes()];
   for (int n=0;n<setsize*NumNodes();n++) allres[n] = 0.;

   if (!MasterCommandRequest(IBTEST_CMD_COLLECT, 0, 0, allres, setsize*sizeof(double))) return false;

//   DOUT((1,"Results of all-to-all test"));
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

   MasterCleanup();

   totalskipped = totalskipped / sumcnt;
   if (totalskipped > 0.01) isskipok = false;

   AllocResults(8);
   fResults[0] = sum1send / sumcnt;
   fResults[1] = sum2recv / sumcnt;

   if ((pattern==3) || (pattern==4)) {
      fResults[0] = fResults[0] * NumNodes();
      fResults[1] = fResults[1] * NumNodes();
   }

   if ((pattern>=0) && (pattern!=5)) {
      if (fResults[0] < datarate - 5) isok = false;
      if (!isskipok) isok = false;
   }

   fResults[2] = isok ? 1. : 0.;
   fResults[3] = (totalrecv>0 ? totallost/totalrecv : 0.);

   if (totalmulti>0) {
      fResults[4] = sum3multi/sumcnt;
      fResults[5] = totalmultilost/totalmulti;
   }
   fResults[6] = sum4cpu/sumcnt;
   fResults[7] = totalskipped;

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
   if (!MasterCommandRequest(IBTEST_CMD_ASKQUEUE, 0, 0, recs, blocksize)) {
      EOUT(("Cannot collect queue sizes"));
      free(recs);
      return false;
   }

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
   if ((allsend!=0) || (allrecv!=0))
       res = MasterCommandRequest(IBTEST_CMD_CLEANUP, conn, -blocksize);

   free(recs);

   free(conn);

   // for syncronisation
   if (!MasterCommandRequest(IBTEST_CMD_TEST)) res = false;

   DOUT0(("Queues are cleaned in %5.4f s res = %s", fStamping() - tm, DBOOL(res)));

   return res;
}

bool bnet::TransportModule::ProcessCleanup(int32_t* pars)
{
   dabc::TimeStamp tm = dabc::Now();

   int recid(-1);
   OperRec* rec(0);
   int maxqueue(5);

   // execute at maximum 7 seconds
   while (!tm.Expired(7.)) {
      recid = -1; // CheckCompletionQueue(0.001);

      rec = fRunnable->GetRec(recid);

      if (rec!=0) {
         // buffer will be released as well
         fRunnable->ReleaseRec(recid);
      }

      bool isany = false;

      for (int node=0;node<NumNodes();node++)
         for (int lid=0;lid<NumLids();lid++) {
            int* entry = pars + node*NumLids()*2 + lid*2;

            if ((entry[0]>0) || (entry[1]>0)) isany = true;

            if ((entry[0]>0) && (SendQueue(lid,node)<maxqueue)) {
               // perform send operation
               dabc::Buffer buf = FindPool("BnetDataPool")->TakeBuffer(fTestBufferSize);

               if (buf.null()) {
                  EOUT(("Not enough buffers"));
                  return false;
               }

               recid = fRunnable->PrepareOperation(kind_Send, sizeof(TransportHeader), buf);
               rec = fRunnable->GetRec(recid);

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

               recid = fRunnable->PrepareOperation(kind_Recv, sizeof(TransportHeader), buf);
               rec = fRunnable->GetRec(recid);

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

      // no any operation to execute
      if (!isany && (TotalSendQueue()==0) && (TotalRecvQueue()==0)) break;
   }

   if ((TotalSendQueue()!=0) || (TotalRecvQueue()!=0))
      EOUT(("Cleanup failed still %d send and %d recv operations", TotalSendQueue(), TotalRecvQueue()));

   return true;
}

void bnet::TransportModule::PerformNormalTest()
{
//   MasterTiming();

//   DOUT0(("SLEEP 1 sec"));
//   WorkerSleep(1.);

   int outq = Cfg("TestOutputQueue").AsInt(5);
   int inpq = Cfg("TestInputQueue").AsInt(10);
   int buffersize = Cfg("BufferSize").AsInt(128*1024);
   int rate = Cfg("TestRate").AsInt(1000);
   int testtime = Cfg("TestTime").AsInt(10);
   int testpattern = Cfg("TestPattern").AsInt(0);

   MasterAllToAll(testpattern, buffersize, testtime, rate, outq, inpq);

   MasterAllToAll(testpattern, buffersize, testtime, rate, outq, inpq);

//   MasterAllToAll(0, 2*1024,   5., 100., 5, 10);

//   MasterAllToAll(-1, 128*1024, 5., 1000., 5, 10);

//   MasterAllToAll(0,  128*1024, 5., 500., 5, 10);

//   MasterAllToAll(-1, 1024*1024, 5., 900., 2, 4);

//   MasterAllToAll(0, 1024*1024, 5., 2300., 2, 4);
}

void bnet::TransportModule::MainLoop()
{
   if (IsSlave()) {
      return;
   }

   DOUT0(("Entering mainloop master: %s test = %s", DBOOL(IsMaster()), fTestKind.c_str()));

   // here we are doing our test sequence (like in former verbs tests)

   MasterCollectActiveNodes();

   CalibrateCommandsChannel();

   if (fTestKind == "OnlyConnect") {

      DOUT0(("----------------- TRY ONLY COMMAND CHANNEL ------------------- "));

      CalibrateCommandsChannel(30);

      MasterCallExit();

      return;
   }


   DOUT0(("----------------- TRY CONN ------------------- "));

   // MasterConnectQPs();

   DOUT0(("----------------- DID CONN !!! ------------------- "));

   //MasterTimeSync(true, 200, false);

   if (fTestKind != "Simple") {

      DOUT0(("SLEEP 10 sec"));
      WorkerSleep(10.);

      //MasterTimeSync(true, 200, true);

      if (fTestKind == "SimpleSync") {
         DOUT0(("Sleep 10 sec more before end"));
         WorkerSleep(10.);
      } else
      if (fTestKind == "TimeSync") {
         // PerformTimingTest();
      } else {
         PerformNormalTest();
      }

      //MasterTimeSync(false, 500, false);
   }

   MasterCloseConnections();

   MasterCallExit();

}

// ===================== new async code ================

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
      case IBTEST_CMD_TIMESYNC: {
         // DOUT0(("Prepare slave sync loop"));
         PrepareSyncLoop(0);
         fCmdEndTime.GetNow(5.);
         break;
      }

      case IBTEST_CMD_ALLTOALL:
         ActivateAllToAll((double*)fCmdDataBuffer);
         break;

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

      msg->magic = IBTEST_CMD_MAGIC;
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

   if (rcv->magic!=IBTEST_CMD_MAGIC) { EOUT(("Wrong magic")); return false; }
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

      if (fRunningCmdId == IBTEST_CMD_TEST)
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
}

void bnet::TransportModule::ProcessRecvCompleted(int recid, OperRec* rec)
{
}


void bnet::TransportModule::ProcessTimerEvent(dabc::Timer* timer)
{
   if ((fRunningCmdId>0) && !fCmdEndTime.null() && fCmdEndTime.Expired()) {
      if (fRunningCmdId!=IBTEST_CMD_WAIT)
         EOUT(("Command %d execution timed out", fRunningCmdId));
      CompleteRunningCommand(dabc::cmd_timedout);
   }

   if (fRunningCmdId>0) {
      // we are in the middle of running operation, in some cases we could activate new actions

      switch (fRunningCmdId) {
         case IBTEST_CMD_TIMESYNC:
            if ((TotalSendQueue() > 0) || (TotalRecvQueue() > 0) || IsMaster()) break;
            CompleteRunningCommand();
            break;
         case IBTEST_CMD_EXECSYNC:
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
      case IBTEST_CMD_WAIT:
         fCmdEndTime.GetNow(fCurrentCmd.GetTimeout());
         fRunningCmdId = IBTEST_CMD_WAIT;
         break;

      case IBTEST_CMD_ACTIVENODES: {
         uint8_t buff[NumNodes()];

         int cnt(1);
         buff[0] = 1;

         for(int node=1;node<NumNodes();node++) {
            bool active = IOPort(node-1)->IsConnected();
            if (active) cnt++;
            buff[node] = active ? 1 : 0;
         }

         DOUT0(("There are %d active nodes, %d missing", cnt, NumNodes() - cnt));
         RequestMasterCommand(IBTEST_CMD_ACTIVENODES, buff, NumNodes());
         break;
      }

      case IBTEST_CMD_TEST:
         RequestMasterCommand(IBTEST_CMD_TEST);
         break;

      case IBTEST_CMD_EXIT:
         RequestMasterCommand(IBTEST_CMD_EXIT);
         break;

      case IBTEST_CMD_MEASURE:
         if (fCmdTestLoop.Number() > 1) {
            fCmdDelay = fCmdTestLoop.Mean();
            DOUT0(("Command loop = %5.3f ms", fCmdDelay*1e3));
         }
         fCmdTestLoop.Reset();
         break;


      case IBTEST_CMD_CREATEQP: {
         fConnStartTime.GetNow();

         int blocksize = fRunnable->ConnectionBufferSize();

         if (blocksize==0) return;

         fConnRawData = malloc(NumNodes() * blocksize);
         fConnSortData = malloc(NumNodes() * blocksize);
         memset(fConnRawData, 0, NumNodes() * blocksize);
         memset(fConnSortData, 0, NumNodes() * blocksize);

         DOUT0(("First collect all QPs info"));

         // own records will be add automatically
         RequestMasterCommand(IBTEST_CMD_CREATEQP, 0, 0, fConnRawData, blocksize);
         break;
      }

      case IBTEST_CMD_CONNECTQP: {
         fRunnable->ResortConnections(fConnRawData, fConnSortData);

         RequestMasterCommand(IBTEST_CMD_CONNECTQP, fConnSortData, -fRunnable->ConnectionBufferSize());
         break;
      }

      case IBTEST_CMD_CONNECTDONE: {
         free(fConnRawData); fConnRawData = 0;
         free(fConnSortData); fConnSortData = 0;
         DOUT0(("Establish connections in %5.4f s ", fConnStartTime.SpentTillNow()));
         break;
      }

      case IBTEST_CMD_CLOSEQP:
         RequestMasterCommand(IBTEST_CMD_CLOSEQP);
         break;

      case IBTEST_CMD_TIMESYNC: {
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
         RequestMasterCommand(IBTEST_CMD_TIMESYNC, fSyncArgs, sizeof(fSyncArgs));
         break;
      }

      case IBTEST_CMD_EXECSYNC:
         fRunningCmdId = IBTEST_CMD_EXECSYNC;
         fSyncStartTime.GetNow();
         fSlaveSyncNode = 1;
         PrepareSyncLoop(fSlaveSyncNode);
         break;

      case IBTEST_CMD_GETSYNC:
         RequestMasterCommand(IBTEST_CMD_GETSYNC);
         break;

      default:
         EOUT(("Command not supported"));
         exit(1);
   }
}

