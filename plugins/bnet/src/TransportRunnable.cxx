#include "bnet/TransportRunnable.h"

void bnet::TimeStamping::ChangeShift(double shift)
{
   fTimeShift += shift;
}

void bnet::TimeStamping::ChangeScale(double koef)
{
   fTimeShift += (get(dabc::Now()) - fTimeShift)*(1. - koef);
   fTimeScale *= koef;
}

// ==============================================================================

bnet::TransportRunnable::TransportRunnable() :
   dabc::Runnable(),
   fMutex(),
   fCondition(&fMutex),
   fReplyCond(&fMutex),
   fNodeId(0),
   fNumNodes(1),
   fNumLids(1),
   fActiveNodes(0),
   fNumRecs(0),
   fRecs(0),
   fFreeRecs(),
   fSubmRecs(),
   fAcceptedRecs(),
   fRunningRecs(),
   fCompletedRecs(),
   fReplyedRecs(),
   fSegmPerOper(8),
   fHeaderPool("TransportHeaders", false),
   fModuleThrd(),
   fTransportThrd(),
   fStamping(),
   fLoopTime(),
   fSendQueueLimit(3),
   fRecvQueueLimit(6)
{
}

bnet::TransportRunnable::~TransportRunnable()
{
   fHeaderPool.Release(); // not necessary, but call it explicitly

   if (fRecs!=0) {
      delete [] fRecs; fRecs = 0;
   }
   fNumRecs = 0;

   if (fActiveNodes!=0) {
      delete [] fActiveNodes;
      fActiveNodes = 0;
   }
}

bool bnet::TransportRunnable::Configure(dabc::Module* m, dabc::MemoryPool* pool, dabc::Command cmd)
{
   fActiveNodes = new bool[NumNodes()];
   for (int n=0;n<NumNodes();n++)
      fActiveNodes[n] = true;

   fNumRecs = 100 + NumNodes()*10;   // FIXME - should depend from number of nodes
   fRecs = new OperRec[fNumRecs];       // operation records
   fFreeRecs.Allocate(fNumRecs);
   fSubmRecs.Allocate(fNumRecs);
   fAcceptedRecs.Allocate(fNumRecs);
   fNumRunningRecs = 0;
   fRunningRecs.resize(fNumRecs, false);
   fCompletedRecs.Allocate(fNumRecs);
   fReplyedRecs.Allocate(fNumRecs);
   for (int n=0;n<fNumRecs;n++)
      fFreeRecs.Push(n);
   fSegmPerOper = 8;

   fHeaderBufSize = 128;
   fHeaderPool.Allocate(fHeaderBufSize, fNumRecs, 2);

   fDoTimeSync = false;
   fDoScaleSync = false;
   fNumSyncCycles = 100;

   fSendQueue.resize(NumLids() * NumNodes(), 0);
   fRecvQueue.resize(NumLids() * NumNodes(), 0);

   return true;
}

int bnet::TransportRunnable::SubmitCmd(int cmdid, void* args, int argssize)
{
   CheckModuleThrd();

   // DOUT0(("Submitting cmd %d", cmdid));

   if (fFreeRecs.Empty()) return -1;

   int recid = fFreeRecs.Pop();
   OperRec* rec = GetRec(recid);

   rec->kind = kind_None;
   rec->SetImmediateTime();
   rec->skind = skind_Command;
   rec->tgtindx = cmdid;

   rec->header = args;
   rec->hdrsize = argssize;
   rec->err = true;

   if (SubmitRec(recid)) return recid;

   fFreeRecs.Push(recid);
   return -1;
}

bool bnet::TransportRunnable::ExecuteCmd(int cmdid, void* args, int argssize)
{
   int recid = SubmitCmd(cmdid, args, argssize);

   if (recid<0) return false;

   CheckModuleThrd();

   dabc::TimeStamp tm = dabc::Now();

   while (!tm.Expired(5.)) {
      int complid = WaitCompleted();

      if (complid<0) { EOUT(("Cmd:%d transport problem", cmdid)); return false; }

      bool res = ! GetRec(complid)->err;

      ReleaseRec(complid);

      if (complid==recid) return res;

      EOUT(("Cmd:%d Some other operation completed - wrong!!!", cmdid));
   }

   EOUT(("Cmd:%d command timedout!!!", cmdid));

   return false;
}


bool bnet::TransportRunnable::ExecuteConfigSync(int* args)
{
   CheckTransportThrd();

   bool ismaster = args[0] > 0;
   fSyncCycle = 0;
   fNumSyncCycles = args[1];
   fSyncMaxCut = 0.7;

   if (ismaster) {
      fDoTimeSync = args[2] > 0;
      fDoScaleSync = args[3] > 0;
      fSyncMasterRec = -1;
      fSyncRecvDone = true; // for the first time we suppose that recv operation is ready and we can send new buffer

      // allocate enough space for all time measurements
      fSync_m_to_s.reserve(fNumSyncCycles);
      fSync_s_to_m.reserve(fNumSyncCycles);

      fSync_m_to_s.clear();
      fSync_s_to_m.clear();
      fSyncSetTimeShift = 0.;
      fSyncSetTimeScale = 1.;
      fSyncResetTimes = false;

      if ((int) fSyncTimes.size() != NumNodes())
         fSyncTimes.resize(NumNodes(), 0.);
   } else {
      fSyncSlaveRec = -1;
      fSyncRecvDone = false;
   }
   return true;
}

bool bnet::TransportRunnable::ExecuteTransportCommand(int cmdid, void* args, int argssize)
{
   CheckTransportThrd();

   switch (cmdid) {
      case cmd_None:  return true;

      case cmd_Exit:  return true;

      case cmd_ActiveNodes: {
         if (argssize != NumNodes()) {
            EOUT(("Wrong size of active nodes array"));
            return false;
         }

         uint8_t* buff = (uint8_t*) args;

         for (int n=0;n<NumNodes();n++)
            fActiveNodes[n] = buff[n] > 0;

         return true;
      }
      case cmd_CreateQP:
         return ExecuteCreateQPs(args, argssize);

      case cmd_ConnectQP:
         return ExecuteConnectQPs(args, argssize);

      case cmd_ConfigSync:
         return ExecuteConfigSync((int*)args);

      case cmd_GetSync:
         fStamping.GetCoeff(args);
         return true;

      case cmd_ResetStat:
         fLoopTime.Reset();
         fLoopMaxCnt = 0;
         return true;

      case cmd_GetStat:
         ((double*) args)[0] = fLoopTime.Max();
         ((double*) args)[1] = fLoopTime.Mean();
         ((double*) args)[2] = fLoopMaxCnt;
         return true;

      case cmd_CloseQP:
         return ExecuteCloseQPs();

      default:
         EOUT(("Uncknown command %d", cmdid));
         break;
   }

   return false;
}

void bnet::TransportRunnable::PrepareSpecialKind(int& recid)
{
   OperRec* rec = GetRec(recid);

   switch (rec->skind) {
      case skind_None:
         return;
      case skind_Command:
         rec->err = !ExecuteTransportCommand(rec->tgtindx, rec->header, rec->hdrsize);
         fCompletedRecs.Push(recid);
         recid = -1;
         if (rec->tgtindx==cmd_Exit) recid = -111;
         return;
      case skind_SyncMasterSend: {
         // DOUT1(("Prepare PrepareSpecialKind skind_SyncMasterSend"));

         // send packet immediately only very first time,
         // in other cases remember id to use it when next reply comes from the slave

         if (fSyncMasterRec>=0) {
            EOUT(("How it could happend - fSyncMasterRec submitted twice!!!"));
         }

         // send operation is ready, but we need to wait that recv operation is done
         if (!fSyncRecvDone) {
            fSyncMasterRec = recid;
            recid = -1;
            return;
         }

         // send

         TimeSyncMessage* msg = (TimeSyncMessage*) rec->header;
         msg->master_time = 0.;
         msg->slave_shift = 0.;
         msg->slave_time = 0;
         msg->slave_scale = 1.;
         msg->msgid = fSyncCycle;

         fSyncResetTimes = false;

         // apply fine shift when 2/3 of work is done
         if ((fSyncCycle == fNumSyncCycles*2/3) && fDoTimeSync) {
            fSync_s_to_m.Sort(); fSync_m_to_s.Sort();
            double time_shift = (fSync_s_to_m.Mean(fSyncMaxCut) - fSync_m_to_s.Mean(fSyncMaxCut)) / 2.;

            fSyncSetTimeShift = time_shift;
            msg->slave_shift = time_shift;
            double sync_t = fStamping();
            int tgtnode = rec->tgtnode;
            if (fDoScaleSync) {
               msg->slave_scale = 1./(1.-time_shift/(sync_t - fSyncTimes[tgtnode]));
               fSyncSetTimeScale = msg->slave_scale;
            }
            fSyncTimes[tgtnode] = sync_t;

            fSyncResetTimes = true;
         }

         // change slave time with first sync message
         if ((fSyncCycle==0) && fDoTimeSync && !fDoScaleSync) {
            msg->master_time = fStamping() + 0.000010;
            fSyncResetTimes = true;
         }

         //if (fSyncCycle==0) DOUT0(("Sending first master packet"));
         // DOUT0(("Sending %d master packet", fSyncCycle));

         fSyncSendTime = fStamping();
         PerformOperation(recid, fSyncSendTime);
         recid = -1;
         fSyncRecvDone = false;

         return;
      }
      case skind_SyncMasterRecv:
         //DOUT0(("Prepare PrepareSpecialKind skind_SyncMasterRecv repeat = %d", rec->repeatcnt));
         return;
      case skind_SyncSlaveSend:
         //DOUT0(("Prepare PrepareSpecialKind skind_SyncSlaveSend"));

         fSyncSlaveRec = recid;
         recid = -1;
         return;
      case skind_SyncSlaveRecv:
         //DOUT0(("Prepare PrepareSpecialKind skind_SyncSlaveRecv"));
         return;

      default:
         return;
   }
}

void  bnet::TransportRunnable::ProcessSpecialKind(int recid)
{
   OperRec* rec = GetRec(recid);

   //DOUT0(("Complete skind:%d err:%s", rec->skind, DBOOL(rec->err)));

   switch (rec->skind) {
      case skind_None:
         return;
      case skind_SyncMasterSend:
         // DOUT1(("skind_SyncMasterSend completed cycle %d", fSyncCycle));
         return;
      case skind_SyncMasterRecv: {
         //DOUT0(("Complete skind_SyncMasterRecv"));
         double recv_time = fStamping();
         TimeSyncMessage* rcv = (TimeSyncMessage*) rec->header;
         if (fSyncResetTimes) {
            fSync_m_to_s.clear();
            fSync_s_to_m.clear();
         } else {
            // exclude very first packet - it is
            fSync_m_to_s.push_back(rcv->slave_time - fSyncSendTime);
            fSync_s_to_m.push_back(recv_time - rcv->slave_time);
         }

         if (rcv->msgid != fSyncCycle) {
            EOUT(("Mismatch in ID %d %d", rcv->msgid, fSyncCycle));
         }

         fSyncCycle++;
         fSyncRecvDone = true;

         // reactivate send operation
         if (fSyncMasterRec>=0) {
            fAcceptedRecs.Push(fSyncMasterRec);
            fSyncMasterRec = -1;
         }

         if (fSyncCycle==fNumSyncCycles) {
            fSync_m_to_s.Sort(); fSync_s_to_m.Sort();
            double time_shift = (fSync_s_to_m.Mean(fSyncMaxCut) - fSync_m_to_s.Mean(fSyncMaxCut)) / 2.;

            DOUT0(("Round trip to %2d: %5.2f microsec", rec->tgtnode, fSync_m_to_s.Mean(fSyncMaxCut)*1e6 + fSync_s_to_m.Mean(fSyncMaxCut)*1e6));
            DOUT0(("   Master -> Slave  : %5.2f  +- %4.2f (max = %5.2f min = %5.2f)", fSync_m_to_s.Mean(fSyncMaxCut)*1e6, fSync_m_to_s.Dev(fSyncMaxCut)*1e6, fSync_m_to_s.Max()*1e6, fSync_m_to_s.Min()*1e6));
            DOUT0(("   Slave  -> Master : %5.2f  +- %4.2f (max = %5.2f min = %5.2f)", fSync_s_to_m.Mean(fSyncMaxCut)*1e6, fSync_s_to_m.Dev(fSyncMaxCut)*1e6, fSync_s_to_m.Max()*1e6, fSync_s_to_m.Min()*1e6));

            if (fDoTimeSync)
               DOUT0(("   SET: Shift = %5.2f  Coef = %12.10f", fSyncSetTimeShift*1e6, fSyncSetTimeScale));
            else {
               DOUT0(("   GET: Shift = %5.2f", time_shift*1e6));
               //get_shift.Fill(time_shift*1e6);
            }
         }

         return;
      }
      case skind_SyncSlaveSend:
         // DOUT1(("skind_SyncSlaveSend completed %d sendrec %d", fSyncCycle, fSyncSlaveRec));

         // send slave reply only when record is ready, means we not fulfill time constrains
         // but packet must be send anyway
         if (fSyncRecvDone /* && (fSyncSlaveRec==recid) */) {
            EOUT(("Reply on the master request with long delay"));
            OperRec* recout = GetRec(recid);
            TimeSyncMessage* msg_out = (TimeSyncMessage*) recout->header;
            msg_out->master_time = 0;
            msg_out->slave_shift = 0;
            msg_out->slave_time = fStamping(); // time irrelevant here
            msg_out->msgid = fSyncCycle++;
            // put in the queue buffer which should be replied
            PerformOperation(recid, msg_out->slave_time);
         }

         fSyncSlaveRec = -1;
         fSyncRecvDone = false;
         return;
      case skind_SyncSlaveRecv: {
         // DOUT1(("skind_SyncSlaveRecv completed %d sendrec %d", fSyncCycle, fSyncSlaveRec));

         double recv_time = fStamping();

         // if (fSyncCycle==0) DOUT0(("Slave receive first packet sendrec:%d", fSyncSlaveRec));
         //DOUT0(("Receive master packet on the slave err = %s", DBOOL(rec->err)));

         TimeSyncMessage* msg_in = (TimeSyncMessage*) rec->header;

         if (fSyncCycle!=msg_in->msgid)
            EOUT(("Missmatch of sync cycle %u %u on the slave", fSyncCycle, msg_in->msgid));

         if (msg_in->master_time>0) {
            fStamping.ChangeShift(msg_in->master_time - recv_time);
         }

         if (msg_in->slave_shift!=0.) {
            fStamping.ChangeShift(msg_in->slave_shift);
            if (msg_in->slave_scale!=1.)
               fStamping.ChangeScale(msg_in->slave_scale);
         }

         if (fSyncSlaveRec<0) {
            // this is situation that next recv operation faster than previous send is completed
            // we just indicate that recv was done and send complition should send new buffer
            fSyncRecvDone = true;
            return;
         }

         OperRec* recout = GetRec(fSyncSlaveRec);
         TimeSyncMessage* msg_out = (TimeSyncMessage*) recout->header;
         msg_out->master_time = 0;
         msg_out->slave_shift = 0;
         msg_out->slave_time = recv_time;
         msg_out->msgid = fSyncCycle++;
         // put in the queue buffer which should be replied
         PerformOperation(fSyncSlaveRec, recv_time);
         fSyncSlaveRec = -1;

         return;
      }
      default:
         return;
   }
}


bool bnet::TransportRunnable::RunSyncLoop(bool ismaster, int tgtnode, int tgtlid, int queuelen, int nrepeat)
{
   CheckModuleThrd();

   // DOUT0(("Enter sync loop"));

   dabc::Queue<int> submoper(queuelen+1);

   // first fill receiving queue
   for (int n=-1;n<queuelen+1;n++) {

      OperKind kind = (n==-1) || (n==queuelen) ? kind_Send : kind_Recv;

      if ((kind==kind_Send)) {
         if (ismaster && (n==-1)) continue; // master should submit send at the end
         if (!ismaster && (n==queuelen)) continue; // slave should submit send in the begin
      }

      int recid = PrepareOperation(kind, sizeof(TimeSyncMessage));
      OperRec* rec = GetRec(recid);
      if ((recid<0) || (rec==0)) { EOUT(("Internal")); return false; }
      if (kind==kind_Recv) {
         rec->skind = ismaster ? skind_SyncMasterRecv : skind_SyncSlaveRecv;
         rec->SetRepeatCnt(nrepeat);
      } else {
         rec->skind = ismaster ? skind_SyncMasterSend : skind_SyncSlaveSend;
         rec->SetRepeatCnt(nrepeat * queuelen);
      }

      rec->SetTarget(tgtnode, tgtlid);
      SubmitRec(recid);
      submoper.Push(recid);
   }

   dabc::TimeStamp start = dabc::Now();

   while (!submoper.Empty() && !start.Expired(5.)) {
      int complid = WaitCompleted(0.1);

      if (complid<0) continue;

      OperRec* rec = GetRec(complid);
      if (rec->err) EOUT(("Operation error"));
      ReleaseRec(complid);
      submoper.Remove(complid);
   }

   // DOUT0(("%s sync loop finished after %5.3f s on cycle:%d", ismaster ? "Master" : "Slave", start.SpentTillNow(), fSyncCycle));

   return true;
}



void* bnet::TransportRunnable::MainLoop()
{
   DOUT0(("Enter TransportRunnable::MainLoop()"));

   dabc::PosixThread::PrintAffinity("bnet-transport");

   // last time when in/out queue was checked
   double last_check_time(-1), last_tm(0);

   fLoopMaxCnt = 0;

   while (true) {

      double till_next_oper = 1.; // calculate time until next operation

      double currtm = fStamping();
      if (last_tm>0) {
         fLoopTime.Fill(currtm - last_tm); // for statistic
         if (currtm - last_tm > 0.001) fLoopMaxCnt++;
      }
      last_tm = currtm;

      if (!fAcceptedRecs.Empty()) {
         OperRec* rec = GetRec(fAcceptedRecs.Front());

         if ((rec->oper_time <= currtm) && rec->IsQueueOk()) {
            int recid = fAcceptedRecs.Pop();

            if (rec->skind!=skind_None)
               PrepareSpecialKind(recid);

            if (recid>=0) {
               PerformOperation(recid, currtm);
            } else {
               // special return value, means exit from the loop
               if (recid==-111) break;
            }
         } else
            till_next_oper = rec->oper_time - currtm;
      }

      if (fNumRunningRecs>0) {
//         double wait_time(0.001), fast_time(0.00002);

         // do not wait at all when new operation need to be submitted
//         if (!fAcceptedRecs.Empty())
//            if (wait_time > till_next_oper/2)
//               wait_time  = till_next_oper/2;

         double wait_time(0.0001), fast_time(0.0001);

         // do not wait at all when new operation need to be submitted
         if (!fAcceptedRecs.Empty() && (till_next_oper<fast_time)) {
            wait_time = 0.;
            fast_time = 0.;
         }

         int recid = DoWaitOperation(wait_time, fast_time);

         // if (wait_time>0) last_tm = 0;

         if (recid>=0) {
            if (!fRunningRecs[recid])
               EOUT(("Wrongly completed operation which was not active recid:%d", recid));

            fRunningRecs[recid] = false;
            fNumRunningRecs--;

            OperRec* rec = GetRec(recid);

            if (rec->skind!=skind_None) {
               ProcessSpecialKind(recid);
               last_tm = 0; // do not account time of special operations
            }

            // decrement appropriate queue counter
            if (rec->queuelen) (*rec->queuelen)--;


            if ((--rec->repeatcnt <= 0) || rec->err) {
               fCompletedRecs.Push(recid);
            } else
               fAcceptedRecs.Push(recid);
         }
      }

      // if next operation comes in 0.01 ms do not start any communications with other thread
      if (till_next_oper<0.00001) continue;

      // if we have running records, do not check queues very often - 1 ms should be enough
      if (fNumRunningRecs>0)
         if (currtm < last_check_time + 0.001) continue;

      // if there are no very urgent operations we could enter locking area
      // and even set condition

      last_check_time = currtm;

      sched_yield();

      // probably, we should lock mutex not very often
      // TODO: check if next operation very close in time - than go to begin of the loop
      dabc::LockGuard lock(fMutex);

      // copy all submitted records to thread
      while (!fSubmRecs.Empty()) {
         fAcceptedRecs.Push(fSubmRecs.Pop());
         till_next_oper = 0.; // indicate that next operation can be very soon
      }

      bool isanyreply(false);
      // copy to shared queue executed records
      while (!fCompletedRecs.Empty()) {
         isanyreply = true;
         fReplyedRecs.Push(fCompletedRecs.Pop());
      }
      if (isanyreply)  fReplyCond._DoFire();

      // if we have nothing to do for very long time, enter wait method for relaxing thread consumption
      if (!isanyreply && (fNumRunningRecs==0) && (fAcceptedRecs.Empty() || (till_next_oper>0.2))) {
         fCondition._DoReset();
         fCondition._DoWait(0.001);
         last_tm = 0; // do not account time when we doing wait
         while (!fSubmRecs.Empty())
            fAcceptedRecs.Push(fSubmRecs.Pop());
      }
   }

   DOUT0(("Exit TransportRunnable::MainLoop()"));

   return 0;
}

bool bnet::TransportRunnable::SetActiveNodes(void* data, int datasize)
{
   return ExecuteCmd(cmd_ActiveNodes, data, datasize);
}

bool bnet::TransportRunnable::CreateQPs(void* recs, int recssize)
{
   return ExecuteCmd(cmd_CreateQP, recs, recssize);
}

bool bnet::TransportRunnable::ConnectQPs(void* recs, int recssize)
{
   return ExecuteCmd(cmd_ConnectQP, recs, recssize);
}

bool bnet::TransportRunnable::ConfigSync(bool master, int nrepeat, bool dosync, bool doscale)
{
   int args[4];
   args[0] = master ? 1 : 0;
   args[1] = nrepeat;
   args[2] = dosync ? 1 : 0;
   args[3] = doscale ? 1 : 0;

   return ExecuteCmd(cmd_ConfigSync, args, sizeof(args));
}

bool bnet::TransportRunnable::ConfigQueueLimits(int send_limit, int recv_limit)
{
   // limits used from module thread, assigned to record when it is submitted
   fSendQueueLimit = send_limit;
   fRecvQueueLimit = recv_limit;
   return true;
}


bool bnet::TransportRunnable::GetSync(TimeStamping& stamp)
{
   double args[2];

   if (!ExecuteCmd(cmd_GetSync, args, sizeof(args))) return false;

   stamp.SetCoeff(args);

   return true;
}

bool bnet::TransportRunnable::ResetStatistic()
{
   return ExecuteCmd(cmd_ResetStat);
}

bool bnet::TransportRunnable::GetStatistic(double& mean_loop, double& max_loop, int& long_cnt)
{
   double args[3];

   if (!ExecuteCmd(cmd_GetStat, args, sizeof(args))) return false;

   max_loop = args[0];
   mean_loop = args[1];
   long_cnt = (int) args[2];

   return true;
}

bool bnet::TransportRunnable::CloseQPs()
{
   return ExecuteCmd(cmd_CloseQP);
}

bool bnet::TransportRunnable::StopRunnable()
{
   // one exit command is submitted, runnable will not be
   return SubmitCmd(cmd_Exit) >= 0;
}

int bnet::TransportRunnable::PrepareOperation(OperKind kind, int hdrsize, dabc::Buffer buf)
{
   CheckModuleThrd();

   if (fFreeRecs.Empty()) return -1;

   // this is only example, has no use

   int recid = fFreeRecs.Pop();

   OperRec* rec = GetRec(recid);

   rec->kind = kind;
   rec->skind = skind_None;

   rec->SetImmediateTime(); // operation will be submitted as soon as possible
   rec->tgtnode = 0;  // id of node to submit
   rec->tgtindx = 0;  // second id for the target, LID number in case of IB
   rec->repeatcnt = 1;  // repeat operation once
   rec->err = false;

   rec->header = fHeaderPool.GetBufferLocation(recid);
   if (hdrsize==0) hdrsize = fHeaderPool.GetBufferSize(recid);

   if ((int) fHeaderPool.GetBufferSize(recid) < hdrsize) {
      EOUT(("header size %d specified wrongly, maximum is %u", hdrsize, fHeaderPool.GetBufferSize(recid)));
   }

   rec->hdrsize = hdrsize;
   rec->buf << buf;

   if (DoPrepareRec(recid)) return recid;

   ReleaseRec(recid);

   return -1;
}

bool bnet::TransportRunnable::ReleaseRec(int recid)
{
   CheckModuleThrd();

   if (GetRec(recid) == 0) {
      EOUT(("Wrong RECID"));
      return false;
   }

   if (fFreeRecs.Full()) {
      EOUT(("internal problem - free records are full"));
      return false;
   }

   fRecs[recid].kind = kind_None;
   fRecs[recid].skind = skind_None;

   fRecs[recid].buf.Release();

   fFreeRecs.Push(recid);

   return true;
}

bool bnet::TransportRunnable::SubmitRec(int recid)
{
   CheckModuleThrd();


   OperRec* rec = GetRec(recid);
   if (rec == 0) {
      EOUT(("Wrong RECID"));
      return false;
   }

   // at the moment when record submitted, set queue size

   switch (rec->kind) {
      case kind_Send:
         rec->queuelen = &(SendQueue(rec->tgtindx, rec->tgtnode));
         rec->queuelimit = fSendQueueLimit;
         break;
      case kind_Recv:
         rec->queuelen = &(RecvQueue(rec->tgtindx, rec->tgtnode));
         rec->queuelimit = fRecvQueueLimit;
         break;
      default:
         rec->queuelen = 0;
         rec->queuelimit = 0;
         break;
   }

   dabc::LockGuard lock(fMutex);

   if (fSubmRecs.Full()) {
      EOUT(("Sumbit queue full!!!!"));
      return false;
   }

   fSubmRecs.Push(recid);

   fCondition._DoFire();

   return true;
}


int bnet::TransportRunnable::WaitCompleted(double tm)
{
   // TODO first : first implement with condition
   // TODO second: fire event to the module - when it works asynchronousely

   CheckModuleThrd();

   /*

   dabc::TimeStamp start = dabc::Now();
   do {
      dabc::LockGuard lock(fMutex);
      if (!fReplyedRecs.Empty())
         return fReplyedRecs.Pop();
   } while (!start.Expired(tm));
   */


   dabc::LockGuard lock(fMutex);
   if (!fReplyedRecs.Empty())
      return fReplyedRecs.Pop();

   fReplyCond._DoReset();
   fReplyCond._DoWait(tm);

   if (!fReplyedRecs.Empty())
      return fReplyedRecs.Pop();

   return -1;
}
