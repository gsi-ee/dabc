// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "dabc/BnetRunnable.h"

#include <algorithm>
#include <math.h>

#include "dabc/ModuleItem.h"
#include "dabc/Manager.h"



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

void bnet::DoublesVector::Sort()
{
   std::sort(begin(), end());
}

double bnet::DoublesVector::Mean(double max_cut)
{
   unsigned right = lrint(max_cut*(size()-1));

   double sum(0);
   for (unsigned n=0; n<=right; n++) sum+=at(n);
   return right>0 ? sum / (right+1) : 0.;
}

double bnet::DoublesVector::Dev(double max_cut)
{
   unsigned right = lrint(max_cut*(size()-1));

   if (right==0) return 0.;

   double sum1(0), sum2(0);
   for (unsigned n=0; n<=right; n++)
      sum1+=at(n);
   sum1 = sum1 / (right+1);

   for (unsigned n=0; n<=right; n++)
      sum2+=(at(n)-sum1)* (at(n)-sum1);

   return ::sqrt(sum2/(right+1));
}

double bnet::DoublesVector::Min()
{
   return size()>0 ? at(0) : 0.;
}

double bnet::DoublesVector::Max()
{
   return size()>0 ? at(size()-1) : 0.;
}

// ==============================================================================


bnet::BnetRunnable::BnetRunnable(const char* name) :
   dabc::Object(name),
   dabc::Runnable(),
   fMutex(),
   fCondition(&fMutex),
   fReplyItem(0),
   fNodeId(0),
   fNumNodes(1),
   fNumLids(1),
   fActiveNodes(0),
   fNumRecs(0),
   fRecs(0),
   fFreeRecs(),
   fSubmRecs(),
   fAcceptedRecs(),
   fPoolRecs(),
   fScheduledRecs(),
   fImmediateRecs(),
   fRunningRecs(),
   fCompletedRecs(),
   fReplyedRecs(),
   fReplySignalled(false),
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

bnet::BnetRunnable::~BnetRunnable()
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

bool bnet::BnetRunnable::Configure(dabc::Module* m, dabc::MemoryPool* pool, int numrecs)
{
   fActiveNodes = new bool[NumNodes()];
   for (int n=0;n<NumNodes();n++)
      fActiveNodes[n] = true;

   fNumRecs = numrecs;
   fRecs = new OperRec[fNumRecs];       // operation records
   fFreeRecs.Allocate(fNumRecs);
   fSubmRecs.Allocate(fNumRecs);
   fAcceptedRecs.Allocate(fNumRecs);
   fPoolRecs.Allocate(fNumRecs);
   fScheduledRecs.Allocate(fNumRecs);
   fImmediateRecs.Allocate(fNumRecs);

   fNumRunningRecs = 0;
   fRunningRecs.resize(fNumRecs, false);
   fCompletedRecs.Allocate(fNumRecs);
   fReplyedRecs.Allocate(fNumRecs);
   fReplySignalled = false;
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

int bnet::BnetRunnable::SubmitCmd(int cmdid, bool isexec, void* args, int argssize)
{
   CheckModuleThrd();

//   DOUT0(("Submitting cmd %d", cmdid));

   int recid = PrepareOperation(kind_None, 0);
   if (recid<0) {
      EOUT(("Cannot prepeare command operation"));
      return -1;
   }

   OperRec* rec = GetRec(recid);
   rec->skind = skind_Command;

   // 0 - command does not require confirmation and will be automatically released when completed
   // 1 - command will be returned back to the module thread where tgtnode will be switched to 2
   rec->tgtnode = isexec ? 1 : 0;

   rec->tgtindx = cmdid;
   rec->header = args;
   rec->hdrsize = argssize;
   rec->err = false;

   if (!SubmitRec(recid)) {
      ReleaseRec(recid);
      return -1;
   }

   return recid;
}

bool bnet::BnetRunnable::ExecuteCmd(int cmdid, void* args, int argssize)
{
   CheckModuleThrd();

   int recid = SubmitCmd(cmdid, true, args, argssize);
   if (recid<0) return false;

   OperRec* rec = GetRec(recid);

   dabc::TimeStamp tm = dabc::Now();
   bool res(false);

   while (!tm.Expired(5.)) {
      dabc::mgr.Sleep(0.001);

//      DOUT0(("Waiting command %d tm %5.3f", cmdid, tm.SpentTillNow()));

      if (rec->tgtnode==2) {
         res = !rec->err;
         break;
      }
   }

   if (rec->tgtnode==1) {
      EOUT(("Command not finished at specified time interval"));
      rec->tgtnode = 0; // indicate that record should be released
   } else {
      ReleaseRec(recid);
//      DOUT0(("Command executed %d", cmdid));
   }

   return res;
}


bool bnet::BnetRunnable::ExecuteConfigSync(int* args)
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

bool bnet::BnetRunnable::ExecuteTransportCommand(int cmdid, void* args, int argssize)
{
   CheckTransportThrd();

//   DOUT0(("ExecuteTransportCommand %d", cmdid));

   switch (cmdid) {
      case cmd_None:
         return true;

      case cmd_Exit:
         fMainLoopActive = false;
         return true;

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

bool bnet::BnetRunnable::PreprocessSpecialKind(int recid, OperRec* rec)
{
   switch (rec->skind) {
      case skind_None:
      case skind_Pool:
      case skind_Refill:
         return true;
      case skind_Command:
         rec->err = !ExecuteTransportCommand(rec->tgtindx, rec->header, rec->hdrsize);
         fCompletedRecs.Push(recid);
         return false; // do not try to submit record

      case skind_SyncMasterSend: {
         // DOUT1(("Prepare PreprocessSpecialKind skind_SyncMasterSend"));

         // send packet immediately only very first time,
         // in other cases remember id to use it when next reply comes from the slave

         if (fSyncMasterRec>=0) {
            EOUT(("How it could happend - fSyncMasterRec submitted twice!!!"));
         }

         // send operation is ready, but we need to wait that recv operation is done
         if (!fSyncRecvDone) {
            fSyncMasterRec = recid;
            return false;
         }

         // send

         if (fSyncCycle==0) DOUT1(("Send first master packet"));

         TimeSyncMessage* msg = (TimeSyncMessage*) rec->header;
         msg->master_time = 0.;
         msg->slave_shift = 0.;
         msg->slave_recv = 0;
         msg->slave_send = 0;
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
         fSyncRecvDone = false;

         return true;
      }
      case skind_SyncMasterRecv:
         //DOUT0(("Prepare PreprocessSpecialKind skind_SyncMasterRecv repeat = %d", rec->repeatcnt));
         return true;
      case skind_SyncSlaveSend:
         //DOUT0(("Prepare PreprocessSpecialKind skind_SyncSlaveSend"));

         if (fSyncRecvDone /* && (fSyncSlaveRec==recid) */) {
            // send slave reply only when record is ready, means we not fulfill time constrains
            // but packet must be send anyway
            // EOUT(("Reply on the master request with long delay"));

            OperRec* recout = GetRec(recid);
            TimeSyncMessage* msg_out = (TimeSyncMessage*) recout->header;
            msg_out->master_time = 0;
            msg_out->slave_shift = 0;
            msg_out->slave_recv = fSyncSlaveRecvTime; // time irrelevant here
            msg_out->slave_send = fStamping(); // time irrelevant here
            msg_out->msgid = fSyncCycle++;
            fSyncSlaveRec = -1;
            fSyncRecvDone = false;
         } else {
            // normal situation - just remember recid to use (reactiavte) it when packet from master received
            fSyncSlaveRec = recid;
            return false;
         }

         return true;
      case skind_SyncSlaveRecv:
         //DOUT0(("Prepare PreprocessSpecialKind skind_SyncSlaveRecv"));
         return true;

      default:
         return true;
   }

   return true;
}

void  bnet::BnetRunnable::PostprocessSpecialKind(int recid)
{
   OperRec* rec = GetRec(recid);

   //DOUT0(("Complete skind:%d err:%s", rec->skind, DBOOL(rec->err)));

   switch (rec->skind) {
      case skind_None:
      case skind_Pool:
         return;
      case skind_Refill: {
         if (fPoolRecs.Empty()) {
            EOUT(("Pool is empty - cannot refill queue!!!"));
            exit(165);
         }

         int newid = fPoolRecs.Pop();
         OperRec* newrec = GetRec(newid);

         newrec->skind = skind_Refill;
         newrec->tgtindx = rec->tgtindx;
         newrec->tgtnode = rec->tgtnode;
         newrec->SetImmediateTime();

         fImmediateRecs.Push(newid);

         return;
      }
      case skind_Command:
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
            fSync_m_to_s.push_back(rcv->slave_recv - fSyncSendTime);
            fSync_s_to_m.push_back(recv_time - rcv->slave_send);
         }

         if (rcv->msgid != fSyncCycle) {
            EOUT(("Mismatch in ID %d %d", rcv->msgid, fSyncCycle));
         }

         fSyncCycle++;
         fSyncRecvDone = true;

         // reactivate send operation
         if (fSyncMasterRec>=0) {
            fScheduledRecs.Push(fSyncMasterRec);
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

         if (fSyncSlaveRec>=0) {
            EOUT(("How can be completed reserved record!!!"));
            fSyncSlaveRec = -1;
         }

         return;
      case skind_SyncSlaveRecv: {
         // DOUT1(("skind_SyncSlaveRecv completed %d sendrec %d", fSyncCycle, fSyncSlaveRec));

         fSyncSlaveRecvTime = fStamping();

         // if (fSyncCycle==0) DOUT0(("Slave receive first packet sendrec:%d", fSyncSlaveRec));

         // DOUT1(("Slave receive syncpacket: cycle:%d recid:%d", fSyncCycle, fSyncSlaveRec));
         // DOUT0(("Receive master packet on the slave err = %s", DBOOL(rec->err)));

         TimeSyncMessage* msg_in = (TimeSyncMessage*) rec->header;

         if (fSyncCycle!=msg_in->msgid)
            EOUT(("Missmatch of sync cycle %u %u on the slave", fSyncCycle, msg_in->msgid));

         if (msg_in->master_time>0) {
            fStamping.ChangeShift(msg_in->master_time - fSyncSlaveRecvTime);
         }

         if (msg_in->slave_shift!=0.) {
            fStamping.ChangeShift(msg_in->slave_shift);
            if (msg_in->slave_scale!=1.)
               fStamping.ChangeScale(msg_in->slave_scale);
         }

         fSyncRecvDone = true;

         if (fSyncSlaveRec>=0) {
            // reactivate send operation, will be called very soon
            fScheduledRecs.Push(fSyncSlaveRec);
            fSyncSlaveRec = -1;
         }
         return;
      }
      default:
         return;
   }
}

void* bnet::BnetRunnable::MainLoop()
{
   DOUT0(("Enter BnetRunnable::MainLoop()"));

   fMainLoopActive = true;

   dabc::PosixThread::PrintAffinity("bnet-transport");

   // last time when in/out queue was checked
   double last_check_time(0.), last_yield_time(0.), last_tm(0);

   fLoopMaxCnt = 0;

   while (fMainLoopActive) {

      double till_next_oper = 1.; // calculate time until next operation

      double currtm = fStamping();
      if (last_tm>0) {
         fLoopTime.Fill(currtm - last_tm); // for statistic
         if (currtm - last_tm > 0.001) fLoopMaxCnt++;

         if (currtm - last_tm > 0.01) DOUT1(("LARGE DELAY %5.2f ms", (currtm - last_tm)*1e3));
      }
      last_tm = currtm;


      // first of all, submit scheduled operation as soon as possible (according to schedule)
      int selectedid(-1);
      OperRec* selectedrec(0);

      if (!fScheduledRecs.Empty()) {
         OperRec* rec = GetRec(fScheduledRecs.Front());

          // TODO: introduce strict time limit - after some delay operation should be skipt
         if ((rec->oper_time <= currtm) && !rec->IsQueueOk())
            EOUT(("Queue problem with record %d tgtnode %d late %4.3f ms", fScheduledRecs.Front(), rec->tgtnode, (currtm - rec->oper_time) * 1000.));

         if ((rec->oper_time <= currtm) && rec->IsQueueOk()) {
            selectedid = fScheduledRecs.Pop();
            selectedrec = rec;
            till_next_oper = 0; // we should recheck time to next operation again
         } else {
            till_next_oper = rec->oper_time - currtm;

            // DOUT1(("Record %d till next %7.6f", fScheduledRecs.Front(), till_next_oper));
         }
      }

      // Second, if exists check that immeadiate operation are executed - they have less priority that scheduled
      // TODO: miminal time interval (here 10 microsec) make configurable or adjustable
      if ((selectedid < 0) && (till_next_oper>1e-5) && !fImmediateRecs.Empty()) {
         OperRec* rec = GetRec(fImmediateRecs.Front());

          // TODO: introduce strict time limit - after some delay operation should be skipt
         if (!rec->IsQueueOk())
            EOUT(("Queue problem with immediate record %d for node %d", fImmediateRecs.Front(), rec->tgtnode));
         else {
            selectedid = fImmediateRecs.Pop();
            selectedrec = rec;
            till_next_oper = 0.;
         }
      }

      // Third - EXECUTE operation which is selected

      if (selectedrec)
         if ((selectedrec->skind==skind_None) || PreprocessSpecialKind(selectedid,selectedrec))
            PerformOperation(selectedid, currtm);

      // if next operation comes in 0.005 ms do not start any other activities
      if (till_next_oper<0.000005) continue;

      // copy all accepted recs into different queues
      while (!fAcceptedRecs.Empty()) {
         int recid = fAcceptedRecs.Pop();
         OperRec* rec = GetRec(recid);

         if (rec->skind == skind_Pool) fPoolRecs.Push(recid); else
         if (rec->IsImmediateTime()) fImmediateRecs.Push(recid); else
         fScheduledRecs.Push(recid);
      }

      if (fNumRunningRecs>0) {
         double wait_time(0.00001), fast_time(0.00001);

         // do not wait at all when new operation need to be submitted
         if (!fScheduledRecs.Empty() && (till_next_oper<fast_time)) {
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

            if (rec->skind!=skind_None)
               PostprocessSpecialKind(recid);

            // decrement appropriate queue counter
            rec->dec_queuelen();

            rec->compl_time = fStamping();

            if ((--rec->repeatcnt <= 0) || rec->err) {
               fCompletedRecs.Push(recid);
               // DOUT1(("Rec %d completed signalled %s size %u", recid, DBOOL(fReplySignalled), fCompletedRecs.Size()));
            } else {
               // reinject record again
               fAcceptedRecs.Push(recid);
            }
         }
      }

      // if (fNumRunningRecs==0) last_tm = 0; // do not account loops where no operation submitted

      // if next operation comes in 0.01 ms do not start any other activities
      if (till_next_oper<0.00001) continue;

      // if we have running records, do not check queues very often - 1 ms should be enough
      if (fNumRunningRecs>0)
         if (currtm < last_check_time + 0.001) continue;

      // start from rare yield - better switch context ourself than let its doing by system at unpredictable point
      if (currtm > last_yield_time + 0.005) {
         sched_yield();
         last_yield_time = currtm;
         continue;
      }

      // if there are no very urgent operations we could enter locking area
      // and even set condition

      last_check_time = currtm;

      // last_tm = 0; // exclude locking from accounted loop time

      // probably, we should lock mutex not very often
      // TODO: check if next operation very close in time - than go to begin of the loop

      bool isdoreply(false);

      {
         dabc::LockGuard lock(fMutex);

         // copy all submitted records to thread
         while (!fSubmRecs.Empty()) {
            fAcceptedRecs.Push(fSubmRecs.Pop());
            till_next_oper = 0.; // indicate that next operation can be very soon
         }

         // copy to shared queue executed records
         while (!fCompletedRecs.Empty()) {
            if (!fReplySignalled) { isdoreply = true; fReplySignalled = true; }
            fReplyedRecs.Push(fCompletedRecs.Pop());
         }

         // if we have nothing to do for very long time, enter wait method for relaxing thread consumption
         if (!isdoreply && (fNumRunningRecs==0) && (till_next_oper>0.1)) {
            fCondition._DoReset();
            fCondition._DoWait(0.01);
            last_tm = 0; // do not account time when we doing wait
            while (!fSubmRecs.Empty())
               fAcceptedRecs.Push(fSubmRecs.Pop());
         }

      } // end of locked mutex

      if (isdoreply && fReplyItem) {
         // DOUT1(("Fire reply event size:%u", fReplyedRecs.Size()));
         fReplyItem->FireUserEvent();
      }
   }

   DOUT0(("Exit BnetRunnable::MainLoop()"));

   return 0;
}

bool bnet::BnetRunnable::SetActiveNodes(void* data, int datasize)
{
   return ExecuteCmd(cmd_ActiveNodes, data, datasize);
}

bool bnet::BnetRunnable::CreateQPs(void* recs, int recssize)
{
   return ExecuteCmd(cmd_CreateQP, recs, recssize);
}

bool bnet::BnetRunnable::ConnectQPs(void* recs, int recssize)
{
   return ExecuteCmd(cmd_ConnectQP, recs, recssize);
}

bool bnet::BnetRunnable::ConfigSync(bool master, int nrepeat, bool dosync, bool doscale)
{
   int args[4];
   args[0] = master ? 1 : 0;
   args[1] = nrepeat;
   args[2] = dosync ? 1 : 0;
   args[3] = doscale ? 1 : 0;

   return ExecuteCmd(cmd_ConfigSync, args, sizeof(args));
}

bool bnet::BnetRunnable::ConfigQueueLimits(int send_limit, int recv_limit)
{
   // limits used from module thread, assigned to record when it is submitted
   fSendQueueLimit = send_limit;
   fRecvQueueLimit = recv_limit;

   return true;
}


bool bnet::BnetRunnable::GetSync(TimeStamping& stamp)
{
   double args[2];

   if (!ExecuteCmd(cmd_GetSync, args, sizeof(args))) return false;

   stamp.SetCoeff(args);

   return true;
}

bool bnet::BnetRunnable::ResetStatistic()
{
   return ExecuteCmd(cmd_ResetStat);
}

bool bnet::BnetRunnable::GetStatistic(double& mean_loop, double& max_loop, int& long_cnt)
{
   double args[3];

   if (!ExecuteCmd(cmd_GetStat, args, sizeof(args))) return false;

   max_loop = args[0];
   mean_loop = args[1];
   long_cnt = (int) args[2];

   return true;
}

bool bnet::BnetRunnable::CloseQPs()
{
   return ExecuteCmd(cmd_CloseQP);
}

bool bnet::BnetRunnable::StopRunnable()
{
   // one exit command is submitted, runnable will not be
   return SubmitCmd(cmd_Exit) >= 0;
}

int bnet::BnetRunnable::PrepareOperation(OperKind kind, int hdrsize, dabc::Buffer buf)
{
   CheckModuleThrd();

   if (fFreeRecs.Empty()) return -1;

   // this is only example, has no use

   int recid = fFreeRecs.Pop();

   OperRec* rec = GetRec(recid);

   rec->kind = kind;
   rec->skind = skind_None;

   rec->SetImmediateTime(); // operation will be submitted as soon as possible
   rec->is_time = 0.;
   rec->compl_time = 0.;
   rec->tgtnode = 0;  // id of node to submit
   rec->tgtindx = 0;  // second id for the target, LID number in case of IB
   rec->repeatcnt = 1;  // repeat operation once
   rec->err = false;
   rec->header = 0;
   rec->hdrsize = 0;

   // special case for operation like command
   if ((kind == kind_None) && buf.null()) return recid;

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

bool bnet::BnetRunnable::ReleaseRec(int recid)
{
   CheckModuleThrd();

   OperRec* rec = GetRec(recid);

   if (rec == 0) {
      EOUT(("Wrong RECID"));
      return false;
   }

   if (fFreeRecs.Full()) {
      EOUT(("internal problem - free records are full"));
      return false;
   }

   rec->reset();

   fFreeRecs.Push(recid);

   return true;
}

bool bnet::BnetRunnable::SubmitRec(int recid)
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


int bnet::BnetRunnable::GetCompleted(bool resetsignalled)
{
   // TODO first : first implement with condition
   // TODO second: fire event to the module - when it works asynchronousely

   CheckModuleThrd();

   int recid(-1);

   while (fMainLoopActive) {

      if (recid>=0) {
         GetRec(recid)->reset();
         fFreeRecs.Push(recid);
         recid = -1;
      }

      dabc::LockGuard lock(fMutex);

      if (fReplyedRecs.Empty()) break;

      recid = fReplyedRecs.Pop();
      OperRec* rec = GetRec(recid);

      if (rec->IsCommand())  {
         // this is case when command submitted without waiting of reply -
         // it should be freed outside locked area
         if (rec->tgtnode==0) continue;

         // this means that record finish its execution and should be processed in ExecuteCmd method
         if (rec->tgtnode==1) {
            rec->tgtnode = 2;
            recid = -1;
            continue;
         }

         EOUT(("Strange value of rec->tgtnode %d for command %d", rec->tgtnode, rec->tgtindx));
      }
      return recid;
   }

   // from this moment we decide that module was not signalled
   // TODO: probably, this flag should be reset only when no items in the queue
   if (resetsignalled) fReplySignalled = false;

   return -1;
}
