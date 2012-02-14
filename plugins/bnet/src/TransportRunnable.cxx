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
   fNodeId(0),
   fNumNodes(1),
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
   fStamping()
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

void bnet::TransportRunnable::IsModuleThrd(const char* method)
{
   if (dabc::PosixThread::Self() == fModuleThrd) return;

   EOUT(("%s called from wrong thread - expected module thread", method ? method : "unknown"));
   exit(1);
}

void bnet::TransportRunnable::IsTransportThrd(const char* method)
{
   if (dabc::PosixThread::Self() == fTransportThrd) return;

   EOUT(("%s called from wrong thread - expected transport thread", method ? method : "unknown"));
   exit(1);
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

   return true;
}

int bnet::TransportRunnable::SubmitCmd(int cmdid, void* args, int argssize)
{
   IsModuleThrd(__func__);

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

   IsModuleThrd(__func__);

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
   IsTransportThrd(__func__);

   fDoTimeSync = args[0] > 0;
   fDoScaleSync = args[1] > 0;
   fNumSyncCycles = args[2];
   fSyncCycle = 0;
   fSyncSlaveRec = -1;

   // allocate enough space for all time measurements
   m_to_s.reserve(fNumSyncCycles);
   s_to_m.reserve(fNumSyncCycles);

   m_to_s.clear();
   s_to_m.clear();
   time_shift = 0.;
   set_time_shift = 0.;
   set_time_scale = 1.;
   needreset = false;
   max_cut = 0.7;

   if ((int) fSyncTimes.size() != NumNodes())
      fSyncTimes.resize(NumNodes(), 0.);
   return true;
}

bool bnet::TransportRunnable::ExecuteTransportCommand(int cmdid, void* args, int argssize)
{
   IsTransportThrd(__func__);

   switch (cmdid) {
      case cmd_None:  return true;

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

      case cmd_CloseQP:
         return ExecuteCloseQPs();

      default:
         EOUT(("Why here??"));
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
         TimeSyncMessage* msg = (TimeSyncMessage*) rec->header;
         msg->master_time = 0.;
         msg->slave_shift = 0.;
         msg->slave_time = 0;
         msg->slave_scale = 1.;
         msg->msgid = fSyncCycle;

         needreset = false;

         // apply fine shift when 2/3 of work is done
         if ((fSyncCycle == fNumSyncCycles*2/3) && fDoTimeSync) {
            s_to_m.Sort(); m_to_s.Sort();
            time_shift = (s_to_m.Mean(max_cut) - m_to_s.Mean(max_cut)) / 2.;

            set_time_shift = time_shift;
            msg->slave_shift = time_shift;
            double sync_t = fStamping();
            if (fDoScaleSync) {
               msg->slave_scale = 1./(1.-time_shift/(sync_t - fSyncTimes[rec->tgtnode]));
               set_time_scale = msg->slave_scale;
            }
            fSyncTimes[rec->tgtnode] = sync_t;

            needreset = true;
         }

         // change slave time with first sync message
         if ((rec->repeatcnt == fNumSyncCycles) && fDoTimeSync && !fDoScaleSync) {
            msg->master_time = fStamping() + 0.000010;
            needreset = true;
         }

         send_time = recv_time = fStamping();

         return;
      }
      case skind_SyncMasterRecv:
         return;
      case skind_SyncSlaveSend:
         fSyncSlaveRec = recid;
         recid = -1;
         return;
      case skind_SyncSlaveRecv:
         return;

      default:
         return;
   }
}

void  bnet::TransportRunnable::ProcessSpecialKind(int recid)
{
   OperRec* rec = GetRec(recid);

   switch (rec->skind) {
      case skind_None:
         return;
      case skind_SyncMasterSend:
         return;
      case skind_SyncMasterRecv: {
         TimeSyncMessage* rcv = (TimeSyncMessage*) rec->header;
         if (needreset) {
            m_to_s.clear();
            s_to_m.clear();
            time_shift = 0.;
         } else {
            // exclude very first packet - it is
            m_to_s.push_back(rcv->slave_time - send_time);
            s_to_m.push_back(recv_time - rcv->slave_time);
         }

         if (rcv->msgid != fSyncCycle) {
            EOUT(("Mismatch in ID %d %d", rcv->msgid, fSyncCycle));
         }

         fSyncCycle++;

         if (fSyncCycle==fNumSyncCycles) {
            m_to_s.Sort(); s_to_m.Sort();
            time_shift = (s_to_m.Mean(max_cut) - m_to_s.Mean(max_cut)) / 2.;

            DOUT0(("Round trip to %2d: %5.2f microsec", rec->tgtnode, m_to_s.Mean(max_cut)*1e6 + s_to_m.Mean(max_cut)*1e6));
            DOUT0(("   Master -> Slave  : %5.2f  +- %4.2f (max = %5.2f min = %5.2f)", m_to_s.Mean(max_cut)*1e6, m_to_s.Dev(max_cut)*1e6, m_to_s.Max()*1e6, m_to_s.Min()*1e6));
            DOUT0(("   Slave  -> Master : %5.2f  +- %4.2f (max = %5.2f min = %5.2f)", s_to_m.Mean(max_cut)*1e6, s_to_m.Dev(max_cut)*1e6, s_to_m.Max()*1e6, s_to_m.Min()*1e6));

            if (fDoTimeSync)
               DOUT0(("   SET: Shift = %5.2f  Coef = %12.10f", set_time_shift*1e6, set_time_scale));
            else {
               DOUT0(("   GET: Shift = %5.2f", time_shift*1e6));
               //get_shift.Fill(time_shift*1e6);
            }
         }

         return;
      }
      case skind_SyncSlaveSend:
         return;
      case skind_SyncSlaveRecv: {
         if (fSyncSlaveRec<0) {
            EOUT(("No special record to reply on recv message!!!"));
            return;
         }

         OperRec* recout = GetRec(fSyncSlaveRec);

         TimeSyncMessage* msg_in = (TimeSyncMessage*) rec->header;
         TimeSyncMessage* msg_out = (TimeSyncMessage*) recout->header;

         msg_out->master_time = 0;
         msg_out->slave_shift = 0;
         msg_out->slave_time = fStamping();
         msg_out->msgid = msg_in->msgid;

         if (msg_in->master_time>0) {
            fStamping.ChangeShift(msg_in->master_time - msg_out->slave_time);
         }

         if (msg_in->slave_shift!=0.) {
            fStamping.ChangeShift(msg_in->slave_shift);
            if (msg_in->slave_scale!=1.)
               fStamping.ChangeScale(msg_in->slave_scale);
         }

         // put in the queue buffer which should be replied
         fAcceptedRecs.Push(fSyncSlaveRec);
         fSyncSlaveRec = -1;

         return;
      }
      default:
         return;
   }
}


bool bnet::TransportRunnable::RunSyncLoop(bool ismaster, int tgtnode, int tgtlid, int queuelen, int nrepeat)
{
   IsModuleThrd(__func__);

   dabc::Queue<int> submoper(queuelen+1);

   // first fill receiving queue
   for (int n=0;n<queuelen;n++) {
      int recid = PrepareOperation(kind_Recv, sizeof(TimeSyncMessage));
      OperRec* rec = GetRec(recid);
      if ((recid<0) || (rec==0)) { EOUT(("Internal")); return false; }
      rec->skind = ismaster ? skind_SyncMasterRecv : skind_SyncSlaveRecv;
      rec->SetRepeatCnt(nrepeat);
      rec->SetTarget(tgtnode, tgtlid);
      SubmitRec(recid);
      submoper.Push(recid);
   }

   // than submit send packet, which is repeated many times
   int sendrecid = PrepareOperation(kind_Send, sizeof(TimeSyncMessage));
   OperRec* sendrec = GetRec(sendrecid);
   if ((sendrecid<0) || (sendrec==0)) { EOUT(("Internal")); return false; }
   sendrec->skind = ismaster ? skind_SyncMasterSend : skind_SyncSlaveSend;
   sendrec->SetRepeatCnt(nrepeat * queuelen);
   sendrec->SetTarget(tgtnode, tgtlid);
   SubmitRec(sendrecid);
   submoper.Push(sendrecid);

   while (!submoper.Empty()) {
      int complid = WaitCompleted();

      if (complid<0) { EOUT(("transport problem")); return false; }

      OperRec* rec = GetRec(complid);
      if (rec->err) EOUT(("Operation error"));
      ReleaseRec(complid);
      submoper.Remove(complid);
   }

   return true;
}



void* bnet::TransportRunnable::MainLoop()
{
   DOUT0(("Enter TransportRunnable::MainLoop()"));

   while (true) {

      if (!fAcceptedRecs.Empty()) {
         OperRec* rec = GetRec(fAcceptedRecs.Front());

         if ((rec->time==0) || (rec->time<=fStamping())) {
            int recid = fAcceptedRecs.Pop();

            if (rec->skind!=skind_None)
               PrepareSpecialKind(recid);

            if (recid>=0) {
               if (DoPerformOperation(recid)) {
                  fRunningRecs[recid] = true;
                  fNumRunningRecs++;
               } else {
                  rec->err = true;
                  fCompletedRecs.Push(recid);
               }
            } else {
               // special return value, means exit from the loop
               if (recid==-111) break;
            }
         }
      }

      if (fNumRunningRecs>0) {
         int recid = DoWaitOperation(0.001,0.00001);

         if (recid>=0) {
            fRunningRecs[recid] = false;
            fNumRunningRecs--;

            OperRec* rec = GetRec(recid);

            if (rec->skind!=skind_None) ProcessSpecialKind(recid);

            if ((rec->repeatcnt-- <= 0) || rec->err) {
               fCompletedRecs.Push(recid);
            } else
               fAcceptedRecs.Push(recid);
         }
      }

      // probably, we should lock mutex not very often
      // TODO: check if next operation very close in time - than go to begin of the loop
      dabc::LockGuard lock(fMutex);

      // copy all submitted records to thread
      while (!fSubmRecs.Empty())
         fAcceptedRecs.Push(fSubmRecs.Pop());

      // copy to shared queue executed records
      while (!fCompletedRecs.Empty())
         fReplyedRecs.Push(fCompletedRecs.Pop());
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

bool bnet::TransportRunnable::ConfigMasterSync(bool dosync, bool doscale, int nrepeat)
{
   int args[3];
   args[0] = dosync ? 1 : 0;
   args[1] = doscale ? 1 : 0;
   args[2] = nrepeat;

   return ExecuteCmd(cmd_ConfigSync, args, sizeof(args));
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
   IsModuleThrd(__func__);

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
   IsModuleThrd(__func__);

   if (GetRec(recid) == 0) {
      EOUT(("Wrong RECID"));
      return false;
   }

   if (fFreeRecs.Full()) {
      EOUT(("internal problem - free records are full"));
      return false;
   }

   fRecs[recid].kind = kind_None;

   fRecs[recid].buf.Release();

   fFreeRecs.Push(recid);

   return true;
}

bool bnet::TransportRunnable::SubmitRec(int recid)
{
   IsModuleThrd(__func__);

   if (GetRec(recid) == 0) {
      EOUT(("Wrong RECID"));
      return false;
   }

   dabc::LockGuard lock(fMutex);

   if (fSubmRecs.Full()) {
      EOUT(("Sumbit queue full!!!!"));
      return false;
   }

   fSubmRecs.Push(recid);

   return true;
}


int bnet::TransportRunnable::WaitCompleted(double tm)
{
   // TODO: implement with condition

   IsModuleThrd(__func__);

   dabc::TimeStamp start = dabc::Now();

   while (!start.Expired(tm)) {
      dabc::LockGuard lock(fMutex);
      if (!fReplyedRecs.Empty())
         return fReplyedRecs.Pop();
   }

   return -1;
}

