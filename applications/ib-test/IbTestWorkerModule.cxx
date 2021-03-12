#include "IbTestWorkerModule.h"

#include "IbTestDefines.h"

#include <algorithm>
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>

#include "dabc/timing.h"
#include "dabc/logging.h"
#include "dabc/Queue.h"
#include "dabc/Manager.h"
#include "dabc/Pointer.h"

#ifdef WITH_GPU
#include "IbTestGPU.h"
#endif


void TimeStamping::ChangeShift(double shift)
{
   fTimeShift += shift;
}

void TimeStamping::ChangeScale(double koef)
{
   fTimeShift += (get(dabc::Now()) - fTimeShift)*(1. - koef);
   fTimeScale *= koef;
}


void DoublesVector::Sort()
{
   std::sort(begin(), end());
}

double DoublesVector::Mean(double max_cut)
{
   unsigned right = lrint(max_cut*(size()-1));

   double sum(0);
   for (unsigned n=0; n<=right; n++) sum+=at(n);
   return right>0 ? sum / (right+1) : 0.;
}

double DoublesVector::Dev(double max_cut)
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

double DoublesVector::Min()
{
   return size()>0 ? at(0) : 0.;
}

double DoublesVector::Max()
{
   return size()>0 ? at(size()-1) : 0.;
}



IbTestWorkerModule::IbTestWorkerModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleSync(name, cmd),
   fStamping()
{
   fNodeNumber = cmd.GetInt("NodeNumber", 0);
   fNumNodes = cmd.GetInt("NumNodes", 1);

   fNumLids = Cfg("TestNumLids", cmd).AsInt(1);
   if (fNumLids>IBTEST_MAXLID) fNumLids = IBTEST_MAXLID;

   int nports = cmd.GetInt("NumPorts", 1);

   int connect_packet_size = 1024 + NumNodes() * NumLids() * sizeof(IbTestConnRec);

   fCmdBufferSize = 16*1024;
   while (fCmdBufferSize < connect_packet_size) fCmdBufferSize*=2;

   fCmdDataBuffer = new char[fCmdBufferSize];

   fTestKind = Cfg("TestKind", cmd).AsStr();
   fTestPoolSize = Cfg("TestPoolSize", cmd).AsInt(250);

   EnsurePorts(nports, nports, "SendPool");

   for (int n=0;n<nports;n++)
      BindPorts(InputName(n), OutputName(n));

   fActiveNodes.clear();
   fActiveNodes.resize(fNumNodes, true);

   fTestScheduleFile = Cfg("TestSchedule",cmd).AsStr();
   if (!fTestScheduleFile.empty())
      fPreparedSch.ReadFromFile(fTestScheduleFile);

   fResults = 0;

   fCmdDelay = 0.;

   for (int n=0;n<IBTEST_MAXLID;n++) {
      fQPs[n] = 0;
      fSendQueue[n] = 0;
      fRecvQueue[n] = 0;
   }
   fTotalSendQueue = 0;
   fTotalRecvQueue = 0;
   fTotalNumBuffers = 0;

   fCQ = 0;

   fPool = 0;
   fBufferSize = 0;

   fSyncTimes = 0;

   fMultiCQ = 0;
   fMultiQP = 0;
   fMultiPool = 0;
   fMultiBufferSize = 0;
   fMultiRecvQueueSize = 0;
   fMultiRecvQueue = 0;
   fMultiSendQueueSize = 0;
   fMultiSendQueue = 0;
   fMultiKind = 0;

   fRecvRatemeter = 0;
   fSendRatemeter = 0;
   fWorkRatemeter = 0;

   fTrendingInterval = 0;
}

IbTestWorkerModule::~IbTestWorkerModule()
{
   DOUT2("Calling ~IbTestReceiverModule destructor name:%s", GetName());

   if (fCmdDataBuffer) {
      delete [] fCmdDataBuffer;
      fCmdDataBuffer = 0;
   }

   if (fRecvRatemeter) {
      delete fRecvRatemeter;
      fRecvRatemeter = 0;
   }

   if (fSendRatemeter) {
      delete fSendRatemeter;
      fSendRatemeter = 0;
   }

   if (fWorkRatemeter) {
      delete fWorkRatemeter;
      fWorkRatemeter = 0;
   }

   AllocResults(0);

   if (fSyncTimes) {
      delete [] fSyncTimes;
      fSyncTimes = 0;
   }
}


void IbTestWorkerModule::AllocResults(int size)
{
   if (fResults!=0) delete[] fResults;
   fResults = 0;
   if (size>0) {
      fResults = new double[size];
      for (int n=0;n<size;n++) fResults[n] = 0.;
   }
}

int IbTestWorkerModule::ExecuteCommand(dabc::Command cmd)
{
   return ModuleSync::ExecuteCommand(cmd);
}

void IbTestWorkerModule::BeforeModuleStart()
{
   DOUT2("IbTestWorkerModule starting");

#ifdef WITH_VERBS

   if (!fIbContext.OpenVerbs(true)) {
      EOUT("Cannot open verbs context - exit");
      dabc::mgr.StopApplication();
   }

#endif
}

void IbTestWorkerModule::AfterModuleStop()
{
   DOUT2("IbTestWorkerModule finished");

   CloseQPs();

#ifdef WITH_VERBS
   fIbContext.Release();
#endif
}

int IbTestWorkerModule::NodeSendQueue(int node) const
{
   if ((node<0) || (node>=NumNodes())) return 0;
   int sum(0);
   for (int lid=0;lid<NumLids();lid++)
      if (fSendQueue[lid]) sum+=fSendQueue[lid][node];
   return sum;
}

int IbTestWorkerModule::NodeRecvQueue(int node) const
{
   if ((node<0) || (node>=NumNodes())) return 0;
   int sum(0);
   for (int lid=0;lid<NumLids();lid++)
      if (fRecvQueue[lid]) sum+=fRecvQueue[lid][node];
   return sum;
}

bool IbTestWorkerModule::CreateQPs(void* data)
{
   CloseQPs();
   if (data==0) return true;

#ifdef WITH_VERBS

   ibv_qp_type qp_type = Cfg("TestReliable").AsBool(true) ? IBV_QPT_RC : IBV_QPT_UC;

   if (qp_type == IBV_QPT_UC) DOUT0("Testing unreliable connections");

   int qpdepth = 128;

   IbTestConnRec* recs = (IbTestConnRec*) data;

   if (fIbContext.null() || (recs==0)) return false;

   fCQ = new verbs::ComplQueue(fIbContext, NumNodes() * qpdepth * 6, 0, true);

   for (int lid = 0; lid<NumLids(); lid++) {

      fQPs[lid] = new verbs::QueuePair* [NumNodes()];
      for (int n=0;n<NumNodes();n++) fQPs[lid][n] = 0;

      for (int node=0;node<NumNodes();node++) {

         int indx = lid * NumNodes() + node;

         recs[indx].lid = fIbContext.lid() + lid;

         if ((node == Node()) || (!fActiveNodes[node])) {
            recs[indx].qp = 0;
            recs[indx].psn = 0;
         } else {
            fQPs[lid][node] = new verbs::QueuePair(fIbContext, qp_type, fCQ, qpdepth, 2, fCQ, qpdepth, 2);
            if (fQPs[lid][node]->qp()==0) return false;
            recs[indx].qp = fQPs[lid][node]->qp_num();
            recs[indx].psn = fQPs[lid][node]->local_psn();

            //         DOUT0("Create QP %d -> %d  %04x:%08x:%08x", Node(), node, recs[node].lid, recs[node].qp, recs[node].psn);
         }
      }
   }

#endif

   return true;
}

bool IbTestWorkerModule::ConnectQPs(void* data)
{
#ifdef WITH_VERBS

   IbTestConnRec* recs = (IbTestConnRec*) data;

   if (recs==0) return false;

   for (int lid=0; lid<NumLids(); lid++) {

      for (int node=0;node<NumNodes();node++) {

         int indx = lid * NumNodes() + node;

         if ((fQPs[lid][node]==0) || !fActiveNodes[node]) continue;

         // try to block LMC bits
         // if (IsSlave()) recs[indx].lid = (recs[indx].lid / 16) * 16;


         // FIXME: one should deliver destination port as well
         if (!fQPs[lid][node]->Connect(recs[indx].lid, recs[indx].qp, recs[indx].psn, lid)) return false;

         DOUT3("Connect QP[%d,%d] -> with  %04x:%08x:%08x", lid, node, recs[indx].lid, recs[indx].qp, recs[indx].psn);
      }

   }

#endif

   return true;
}



bool IbTestWorkerModule::CloseQPs()
{

#ifdef WITH_VERBS

   if (fMultiQP!=0) { delete fMultiQP; fMultiQP = 0; }
   if (fMultiCQ!=0) { delete fMultiCQ; fMultiCQ = 0; }
   if (fMultiPool!=0) { delete fMultiPool; fMultiPool = 0; }

   for (int lid=0; lid<NumLids(); lid++) {
      if (fQPs[lid]!=0) {
         for (int n=0;n<NumNodes();n++) delete fQPs[lid][n];
         delete[] fQPs[lid];
         fQPs[lid] = 0;
      }

      delete[] fSendQueue[lid]; fSendQueue[lid] = 0;
      delete[] fRecvQueue[lid]; fRecvQueue[lid] = 0;

   }

   if (fCQ!=0) {
      delete fCQ;
      fCQ = 0;
   }


   if (fPool) {
      delete fPool;
      fPool = 0;
   }




#endif

   return true;
}

bool IbTestWorkerModule::CreateCommPool(int64_t* pars)
{

#ifdef WITH_VERBS

   if (fPool!=0) { delete fPool; fPool = 0; }

   fPool = new verbs::MemoryPool(fIbContext, "Pool", pars[0], pars[1], false);

   fBufferSize = pars[1];

   for (int lid=0; lid < NumLids(); lid++) {
      if (fSendQueue[lid]==0)
         fSendQueue[lid] = new int[NumNodes()];
      if (fRecvQueue[lid]==0)
         fRecvQueue[lid] = new int[NumNodes()];

      for (int n=0;n<NumNodes();n++) {
         fSendQueue[lid][n] = 0;
         fRecvQueue[lid][n] = 0;
      }
   }

   fTotalSendQueue = 0;
   fTotalRecvQueue = 0;

   fTotalNumBuffers = pars[0];

//   DOUT0("Create comm pool %p  size %u X %u", fPool, pars[0], pars[1]);


   return fPool!=0;

#endif

   return true;
}


bool IbTestWorkerModule::Pool_Post(bool issend, int bufindx, int lid, int nremote, int size)
{
#ifdef WITH_VERBS

   if ((fPool==0) || (lid>=NumLids()) || (fQPs[lid]==0) || (fQPs[lid][nremote]==0) || (bufindx<0)) {

      return false;
   }

   bool res = false;
   // we expect maximum 1000000 buffers,
   // we expect maximum 1000 nodes
   // we expect maximum 100 lids

   if ((bufindx>=1000000) || (nremote>=1000) || (lid>=100)) { EOUT("Too large indexes !!!"); exit(5); }

   uint64_t arg = bufindx + nremote*1000000LLU + lid*1000000000LLU + (issend ? 100000000000LLU : 0);

   if (issend) {
      if (size==0) size = fBufferSize;
      struct ibv_send_wr* swr = fPool->GetSendWR(bufindx, size);
      swr->wr_id = arg;
      res = fQPs[lid][nremote]->Post_Send(swr);
//      DOUT0("Post send nremote %d buf %d swr %p ", nremote, bufindx, swr);

   } else {
      struct ibv_recv_wr* rwr = fPool->GetRecvWR(bufindx);
      rwr->wr_id = arg;

      res = fQPs[lid][nremote]->Post_Recv(rwr);

//      DOUT0("Post recv remote %d buf %d rwr %p ", nremote, bufindx, rwr);
   }

   if (res) {
      if (issend) { fSendQueue[lid][nremote]++; fTotalSendQueue++; }
             else { fRecvQueue[lid][nremote]++; fTotalRecvQueue++; }
   }

   return res;

#endif

   return true;
}

bool IbTestWorkerModule::Pool_Post_Mcast(bool issend, int bufindx, int size)
{
#ifdef WITH_VERBS
   if ((fMultiPool==0) || (fMultiQP==0) || (bufindx<0)) return false;

   bool res = false;
   uint64_t arg = bufindx + (issend ? 1000000U : 0);

   if (issend) {
      struct ibv_send_wr* swr = fMultiPool->GetSendWR(bufindx, size);
      swr->wr_id = arg;
      res = fMultiQP->Post_Send(swr);
//      DOUT0("Post send nremote %d buf %d swr %p ", nremote, bufindx, swr);

   } else {
      struct ibv_recv_wr* rwr = fMultiPool->GetRecvWR(bufindx);
      rwr->wr_id = arg;

      res = fMultiQP->Post_Recv(rwr);

//      DOUT0("Post recv remote %d buf %d rwr %p ", nremote, bufindx, rwr);
   }

   if (res) {
      if (issend) fMultiSendQueue++;
             else fMultiRecvQueue++;
   }

#endif


   return true;

}

verbs::ComplQueue* IbTestWorkerModule::Pool_CQ_Check(bool &iserror, double waittime)
{
   // returns CQ, which gets event, if nothing, return 0

   iserror = false;
   verbs::ComplQueue* cq = 0;
   int res = 0;

#ifdef WITH_VERBS

   if (fCQ!=0) {
      cq = fCQ;
      if (TotalRecvQueue()+TotalSendQueue() > 0)
         res = cq->Wait(waittime);
      else
         res = 0;
   }

#endif

   iserror = (res==2);

   return (res==1) ? cq : 0;
}

int IbTestWorkerModule::Pool_Check(int &bufindx, int& lid, int &nremote, double waittime, double fasttime)
{
   // Returns: 0 - if nothing
   //         -1 - if error
   //          1 - when receiving is complete
   //         10 - when sending is complete

   bufindx = -1;
   nremote = -1;
   lid = -1;

#ifdef WITH_VERBS

   // bool iserror = false;
   // verbs::ComplQueue* cq = Pool_CQ_Check(iserror, waittime);
   // if (cq==0) return 0;
   // if (iserror) return -1;

   verbs::ComplQueue* cq = fCQ;
   if (cq==0) return -1;

   int res = cq->Wait(waittime, fasttime);

   if (res==2) return -1;
   if (res!=1) return 0;

   uint64_t arg = cq->arg();

   bufindx = arg % 1000000LLU;
   nremote = (arg / 1000000LLU) % 1000;
   lid = (arg / 1000000000LLU) % 100;

   if ((nremote>=NumNodes()) || (lid>=NumLids())) {
      EOUT("Wrong result id buf:%d lid:%d node:%d", bufindx, lid, nremote);
      exit(6);
   }

   bool issend = (arg / 100000000000LLU) > 0;

   if (issend) { fSendQueue[lid][nremote]--; fTotalSendQueue--; }
         else { fRecvQueue[lid][nremote]--; fTotalRecvQueue--; }

   return issend ? 10 : 1;

#endif

   return 0;
}


int IbTestWorkerModule::Pool_Check_Mcast(int &bufindx, double waittime, double fasttime)
{
   bufindx = -1;

#ifdef WITH_VERBS

   // bool iserror = false;
   // verbs::ComplQueue* cq = Pool_CQ_Check(iserror, waittime);
   // if (cq==0) return 0;
   // if (iserror) return -1;

   verbs::ComplQueue* cq = fMultiCQ;
   if (cq==0) return -1;

   int res = cq->Wait(waittime, fasttime);

   if (res==2) return -1;
   if (res!=1) return 0;

   uint64_t arg = cq->arg();

   bufindx = arg % 1000000U;
   bool issend = (arg / 1000000U) > 0;

   if (issend) fMultiSendQueue--;
          else fMultiRecvQueue--;

   return issend ? 10 : 1;

#endif

   return 0;
}

bool IbTestWorkerModule::IsMulticastSupported()
{
#ifdef WITH_VERBS
   return fIbContext.IsMulticast();

#endif
   return false;
}



int IbTestWorkerModule::GetExclusiveIndx(verbs::MemoryPool* pool)
{
   if (pool==0) pool = fPool;
#ifdef WITH_VERBS
   unsigned indx;
   if (pool && pool->TakeRawBuffer(indx)) return indx;
#endif
   return -1;
}

void* IbTestWorkerModule::GetPoolBuffer(int indx, verbs::MemoryPool* pool)
{
   if (pool==0) pool = fPool;
#ifdef WITH_VERBS
   if (pool && (indx>=0)) return pool->GetSendBufferLocation(indx);
#endif
   return 0;
}

void IbTestWorkerModule::ReleaseExclusive(int indx, verbs::MemoryPool* pool)
{
   if (pool==0) pool = fPool;
#ifdef WITH_VERBS
   if (pool && (indx>=0)) pool->ReleaseRawBuffer(indx);
#endif
}


bool IbTestWorkerModule::SlaveTimeSync(int64_t* cmddata)
{

#ifdef WITH_VERBS

   int numcycles = cmddata[0];
   int maxqueuelen = cmddata[1];
   int sync_lid = cmddata[2];
   bool time_sync_short = cmddata[3] > 0;

   if (fPool==0) return false;

   DOUT3("Start SlaveTimeSync");

   int sendbufindx = GetExclusiveIndx();
   IbTestTymeSyncMessage* msg_out = (IbTestTymeSyncMessage*) GetPoolBuffer(sendbufindx);


   if (!time_sync_short)
      while (RecvQueue(sync_lid,0)<maxqueuelen) {
         if (!Pool_Post(false, GetExclusiveIndx(), sync_lid, 0)) return false;
      }

   double now;

   dabc::Average slave_oper;


   int repeatcounter = 0, nsendcomplited = 0;

   while (repeatcounter<numcycles) {

      now = fStamping();

      double stoptm = now;

      if (time_sync_short)
         if (!Pool_Post(false, GetExclusiveIndx(), sync_lid, 0)) return false;

      // very first time wait longer, while it may take some time to come
      if (repeatcounter==0) stoptm+=10.; else stoptm+=0.1;

      int res(0), resnode(0), resindx(-1), reslid(-1);

      while ((res==0) && (now < stoptm)) {
         res = Pool_Check(resindx, reslid, resnode, stoptm-now, 0.001);

         now = fStamping();

         if (res==10) {
            if ((resindx==sendbufindx) && (resnode==0) && (reslid==sync_lid)) {
               res = 0;
               nsendcomplited++;
            } else {
              EOUT("Error when complete send sync message check_resindx:%s resnode:%d reslid:%d", DBOOL(resindx==sendbufindx), resnode, reslid);
              return false;
            }
         }
      }

      if ((res!=1) || (resnode!=0) || (reslid!=sync_lid)) {
         EOUT("Error when receive %d sync message res:%d resindx:%d resnode:%d reslid:%d", repeatcounter, res, resindx, resnode, reslid);
         return false;
      }

      IbTestTymeSyncMessage* msg_in = (IbTestTymeSyncMessage*) GetPoolBuffer(resindx);

      // if (repeatcounter==0) DOUT0("Get first sync message from master");

      msg_out->master_time = 0;
      msg_out->slave_shift = 0;
      msg_out->slave_time = now;
      msg_out->msgid = msg_in->msgid;

      if (!Pool_Post(true, sendbufindx, sync_lid, 0, sizeof(IbTestTymeSyncMessage)))
         return false;

      slave_oper.Fill((fStamping() - now)*1e6);

      if (msg_in->master_time>0) {
         fStamping.ChangeShift(msg_in->master_time - now);
      }

      if (msg_in->slave_shift!=0.) {
         fStamping.ChangeShift(msg_in->slave_shift);
         if (msg_in->slave_scale!=1.)
            fStamping.ChangeScale(msg_in->slave_scale);
      }

      if (!time_sync_short && (repeatcounter + RecvQueue(sync_lid,0) + 1 < numcycles))
         Pool_Post(false, resindx, sync_lid, 0);
      else
         ReleaseExclusive(resindx);

      repeatcounter++;
   }

//   slave_oper.Show("Slave operation", true);

   double stoptm = fStamping() + 0.1;
   while ((nsendcomplited<numcycles) && (fStamping()<stoptm)) {
      int res(0), resnode(0), resindx(0), reslid(0);
      res = Pool_Check(resindx, reslid, resnode, stoptm - fStamping());
      if ((res==10) && (resindx==sendbufindx) && (resnode==0)) nsendcomplited++;
   }

   if (nsendcomplited<numcycles) EOUT("Not all send operations completed %d", numcycles - nsendcomplited);

   ReleaseExclusive(sendbufindx);


#endif

   // delay for other nodes
   WorkerSleep(0.001);

   return true;
}


bool IbTestWorkerModule::MasterCommandRequest(int cmdid, void* cmddata, int cmddatasize, void* allresults, int resultpernode)
{
    // Request from master side to move in specific command
    // Actually, appropriate functions should run at about the
    // same time while special delay is calculated before and informed
    // each slave how long it should be delayed

   // if module not working, do not try to do anytrhing else
   if (!ModuleWorking(0.)) return false;

   bool incremental_data = false;
   if (cmddatasize<0) {
      cmddatasize = -cmddatasize;
      incremental_data = true;
   }

   int fullpacketsize = sizeof(IbTestCommandMessage) + cmddatasize;

   if (resultpernode > cmddatasize) {
      // special case, to use same buffer on slave for reply,
      // original buffer size should be not less than requested
      // therefore we just append fake data
      fullpacketsize = sizeof(IbTestCommandMessage) + resultpernode;
   }

   if (fullpacketsize>fCmdBufferSize) {
      EOUT("Too big command data size %d", cmddatasize);
      return false;
   }

   dabc::Buffer buf0;

   for(int node=0;node<NumNodes();node++) if (fActiveNodes[node]) {

      dabc::Buffer buf = TakeBuffer(0, 1.);
      if (buf.null()) { EOUT("No empty buffer"); return false; }

      IbTestCommandMessage* msg = (IbTestCommandMessage*) buf.SegmentPtr(0);

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
//         DOUT0("Send buffer to slave %d", node);
         Send(node-1, buf, 1.);
      }
   }

   PreprocessSlaveCommand(buf0);

   std::vector<bool> replies;
   replies.resize(NumNodes(), false);
   bool finished = false;

   while (ModuleWorking() && !finished) {
      dabc::Buffer buf;
      buf << buf0;
      int nodeid = 0;

      if (buf.null()) {
         unsigned port(0);
         nodeid = -1;
         buf = RecvFromAny(&port, 4.);
         if (!buf.null()) nodeid = port + 1;
      }

      if (buf.null() || (nodeid<0)) {
         for (unsigned n=0;n<replies.size();n++)
            if (fActiveNodes[n] && !replies[n]) EOUT("Cannot get any reply from node %u", n);
         return false;
      }

//      DOUT0("Recv reply from slave %d", nodeid);

      replies[nodeid] = true;

      IbTestCommandMessage* rcv = (IbTestCommandMessage*) buf.SegmentPtr(0);

      if (rcv->magic!=IBTEST_CMD_MAGIC) { EOUT("Wrong magic"); return false; }
      if (rcv->cmdid!=cmdid) { EOUT("Wrong ID"); return false; }
      if (rcv->node != nodeid) { EOUT("Wrong node"); return false; }

      if ((allresults!=0) && (resultpernode>0) && (rcv->cmddatasize == resultpernode)) {
         // DOUT0("Copy from node %d buffer %d", nodeid, resultpernode);
         memcpy((uint8_t*)allresults + nodeid*resultpernode, rcv->cmddata(), resultpernode);
      }

      finished = true;

      for (unsigned n=0;n<replies.size();n++)
         if (fActiveNodes[n] && !replies[n]) finished = false;
   }

   // Should we call slave method here ???

   return true;
}

bool IbTestWorkerModule::MasterCollectActiveNodes()
{
   fActiveNodes.clear();
   fActiveNodes.push_back(true); // master itself is active

   uint8_t buff[NumNodes()];

   int cnt = 1;
   buff[0] = 1;

   for(int node=1;node<NumNodes();node++) {
      bool active = IsInputConnected(node-1);
      fActiveNodes.push_back(active);
      if (active) cnt++;
      buff[node] = active ? 1 : 0;
   }

   DOUT0("There are %d active nodes, %d missing", cnt, NumNodes() - cnt);

   if (!MasterCommandRequest(IBTEST_CMD_ACTIVENODES, buff, NumNodes())) return false;

   return cnt == NumNodes();
}





bool IbTestWorkerModule::CalibrateCommandsChannel(int nloop)
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

   DOUT0("Command loop = %5.3f ms", fCmdDelay*1e3);

   return true;
}

bool IbTestWorkerModule::MasterConnectQPs()
{
   dabc::TimeStamp tm = dabc::Now();

   IbTestConnRec* recs = new IbTestConnRec[NumNodes() * NumNodes() * NumLids()];
   memset(recs, 0, NumNodes() * NumNodes() * NumLids() * sizeof(IbTestConnRec));

   // own records will be add automatically
   if (!MasterCommandRequest(IBTEST_CMD_CREATEQP, 0, 0, recs, NumNodes()*NumLids()*sizeof(IbTestConnRec))) {
      EOUT("Cannot create and collect QPs");
      delete[] recs;
      return false;
   }

   // now we should resort connection records, loop over targets

   DOUT0("First collect all QPs info");

   IbTestConnRec* conn = new IbTestConnRec[NumNodes() * NumNodes() * NumLids()];
   memset(conn, 0, NumNodes() * NumNodes() * NumLids() * sizeof(IbTestConnRec));

   for (int n1 = 0; n1 < NumNodes(); n1++)
      for (int n2 = 0; n2 < NumNodes(); n2++)
         for (int lid=0; lid<NumLids(); lid++) {

            int src = n2 * NumNodes() * NumLids() + lid * NumNodes() + n1;

            int tgt = n1 * NumNodes() * NumLids() + lid * NumNodes() + n2;

            memcpy(conn + tgt, recs + src, sizeof(IbTestConnRec));

//         DOUT0("Try to connect %d -> %d %04x:%08x:%08x", n1, n2, conn[tgt].lid, conn[tgt].qp, conn[tgt].psn);
      }

   bool res = MasterCommandRequest(IBTEST_CMD_CONNECTQP, conn, -NumNodes()*NumLids()*sizeof(IbTestConnRec));

   delete[] recs;

   delete[] conn;

   DOUT0("Establish connections in %5.4f s ", tm.SpentTillNow());

   return res;
}

bool IbTestWorkerModule::MasterCallExit()
{
   return MasterCommandRequest(IBTEST_CMD_EXIT);
}

int IbTestWorkerModule::PreprocessSlaveCommand(dabc::Buffer& buf)
{
   if (buf.null()) return IBTEST_CMD_EXIT;

   IbTestCommandMessage* msg = (IbTestCommandMessage*) buf.SegmentPtr(0);

   if (msg->magic!=IBTEST_CMD_MAGIC) {
       EOUT("Magic id is not correct");
       return IBTEST_CMD_EXIT;
   }

   int cmdid = msg->cmdid;

   fCmdDataSize = msg->cmddatasize;
   if (fCmdDataSize > fCmdBufferSize) {
      EOUT("command data too big for buffer!!!");
      fCmdDataSize = fCmdBufferSize;
   }

   memcpy(fCmdDataBuffer, msg->cmddata(), fCmdDataSize);
//   double delay = msg->delay;
//   dabc::TimeStamp recvtm = dabc::Now();

   msg->node = dabc::mgr()->NodeId();
   msg->cmddatasize = 0;
   int sendpacketsize = sizeof(IbTestCommandMessage);

   bool cmd_res(false);

   switch (cmdid) {
      case IBTEST_CMD_TEST:

#ifdef WITH_VERBS
         if (fCQ) fCQ->AcknoledgeEvents();
#endif
         cmd_res = true;
         break;

      case IBTEST_CMD_POOL:
         cmd_res = CreateCommPool((int64_t*)msg->cmddata());
         break;

      case IBTEST_CMD_COLLECT:
         if ((msg->getresults > 0) && (fResults!=0)) {
            msg->cmddatasize = msg->getresults;
            sendpacketsize += msg->cmddatasize;
            memcpy(msg->cmddata(), fResults, msg->getresults);
            cmd_res = true;
         }
         break;

      case IBTEST_CMD_CREATEQP:
         cmd_res = CreateQPs(msg->cmddata());
         msg->cmddatasize = msg->getresults;
         sendpacketsize += msg->cmddatasize;
         break;

      case IBTEST_CMD_CONNECTQP:
         cmd_res = ConnectQPs(msg->cmddata());
         break;

      case IBTEST_CMD_CLOSEQP:
         cmd_res = CloseQPs();
         break;

      case IBTEST_CMD_ASKQUEUE:
         ProcessAskQueue();
         cmd_res = true;
         break;

      default:
         cmd_res = true;
         break;
   }

/*
   if ((msg_in->getresults > 0) && (cmdid==TEST3_CMD_COLLRATE)) {
      msg_out->cmddatasize = msg_in->getresults* sizeof(double);
      sendpacketsize += msg_out->cmddatasize;

      int64_t firstpoint = ((int64_t*) msg_in->cmddata)[0];
      int64_t recvrate = ((int64_t*) msg_in->cmddata)[1];
      Ratemeter* mtr = recvrate>0 ? fRecvRatemeter : fSendRatemeter;
      if ((mtr!=0) && (fWorkRatemeter!=0))
         for(int n=0;n<msg_in->getresults;n++) {
            double val = mtr->GetPoint(firstpoint + n);
            if (val==0)
               if (fWorkRatemeter->GetPoint(firstpoint + n)<=0) val = -1;
            ((double*)(msg_out->cmddata))[n] = val;
         }
   }
*/

//   if (delay>0)
//     while (dabc::Now() < recvtm + delay);

   buf.SetTotalSize(sendpacketsize);

   if (!cmd_res) EOUT("Command %d failed", cmdid);

   return cmdid;
}

bool IbTestWorkerModule::ExecuteSlaveCommand(int cmdid)
{
   switch (cmdid) {
      case IBTEST_CMD_NONE:
         EOUT("Error in WaitForCommand");
         return false;

      case IBTEST_CMD_TIMESYNC:
         return SlaveTimeSync((int64_t*)fCmdDataBuffer);

      case IBTEST_CMD_SLEEP:
         WorkerSleep(((int64_t*)fCmdDataBuffer)[0]);
         break;

      case IBTEST_CMD_EXIT:
         WorkerSleep(0.1);
         DOUT2("Propose worker to stop");
         // Stop(); // stop module
         dabc::mgr.StopApplication();
         break;

      case IBTEST_CMD_CLEANUP:
         ProcessCleanup((int64_t*)fCmdDataBuffer);
         break;

      case IBTEST_CMD_ALLTOALL:
         return ExecuteAllToAll((double*)fCmdDataBuffer);

      case IBTEST_CMD_CHAOTIC:
//            cmd_res = ExecuteChaotic((int64_t*)cmddata);
         break;

      case IBTEST_CMD_INITMCAST:
//            cmd_res = ExecuteInitMulticast((int64_t*)cmddata);
         break;

      case IBTEST_CMD_MCAST:
//            cmd_res = ExecuteMulticast((int64_t*)cmddata);
         break;
      case IBTEST_CMD_RDMA:
//            cmd_res = ExecuteRDMA((int64_t*)cmddata);
         break;

      case IBTEST_CMD_TIMING:
         return ExecuteTiming((double*)fCmdDataBuffer);

      case IBTEST_CMD_ACTIVENODES: {
         uint8_t* buff = (uint8_t*) fCmdDataBuffer;

         fActiveNodes.clear();
         for (int n=0;n<fCmdDataSize;n++)
            fActiveNodes.push_back(buff[n]>0);

         break;
      }

      case IBTEST_CMD_TESTGPU:
         return ExecuteTestGPU((double*)fCmdDataBuffer);

   }

   return true;
}

void IbTestWorkerModule::SlaveMainLoop()
{
   // slave is only good for reaction on the commands
   while (ModuleWorking()) {
      dabc::Buffer buf = Recv(0, 1.);
      if (buf.null()) continue;

      int cmdid = PreprocessSlaveCommand(buf);

      Send(buf, 1.);

      ExecuteSlaveCommand(cmdid);
   }
}

bool IbTestWorkerModule::MasterCloseConnections()
{
   dabc::TimeStamp tm = dabc::Now();

   if (!MasterCommandRequest(IBTEST_CMD_CLOSEQP)) return false;

   // this is just ensure that all nodes are on ready state
   if (!MasterCommandRequest(IBTEST_CMD_TEST)) return false;

   DOUT0("Comm ports are closed in %5.3fs", tm.SpentTillNow());

   return true;
}


bool IbTestWorkerModule::MasterCreatePool(int numbuffers, int buffersize)
{
   dabc::TimeStamp tm = dabc::Now();

   int64_t pars[2];
   pars[0] = numbuffers;
   pars[1] = buffersize;

   if (!MasterCommandRequest(IBTEST_CMD_POOL, pars, sizeof(pars))) return false;

   // this is just ensure that all nodes are on ready state
   if (!MasterCommandRequest(IBTEST_CMD_TEST)) return false;

   DOUT0("Pool is created after %6.5f", tm.SpentTillNow());

   return true;
}

bool IbTestWorkerModule::MasterTimeSync(bool dosynchronisation, int numcycles, bool doscaling, bool debug_output)
{
   // first request all salves to start sync

   int maxqueuelen = 20;
   if (maxqueuelen > numcycles) maxqueuelen = numcycles;

   if (!MasterCreatePool(maxqueuelen+10, 1024)) return false;

   int sync_lid = NumLids() / 2;

   bool time_sync_short = Cfg("TestTimeSyncShort").AsBool(false);

   int64_t pars[4];
   pars[0] = numcycles;
   pars[1] = maxqueuelen;
   pars[2] = sync_lid;
   pars[3] = time_sync_short ? 1 : 0;
   if (!MasterCommandRequest(IBTEST_CMD_TIMESYNC, pars, sizeof(pars))) return false;

   if (fSyncTimes==0) {
      fSyncTimes = new double[NumNodes()];
      for (int n=0;n<NumNodes();n++)
         fSyncTimes[n] = 0.;
   }

   double starttm = fStamping();

   int sendbufindx = GetExclusiveIndx();
   IbTestTymeSyncMessage* msg = (IbTestTymeSyncMessage*) GetPoolBuffer(sendbufindx);

//   DOUT0("Send buffer index = %d buf %p", sendbufindx, msg);

   int repeatcounter = 0;

   DoublesVector m_to_s, s_to_m;

   dabc::Average get_shift;

//   dabc::Average a1;
//   a1.AllocateHist(50, 1., 6.);

   double max_cut = 0.7;

   double time_shift;
   double set_time_scale, set_time_shift;

   for(int nremote=1;nremote<NumNodes();nremote++) {

      if (!fActiveNodes[nremote]) continue;

      DOUT2("Start with node %d", nremote);

      // first fill receiving queue - only if it is not short case with single buffer
      if (!time_sync_short)
         while (RecvQueue(sync_lid, nremote)<maxqueuelen) {
            if (!Pool_Post(false, GetExclusiveIndx(), sync_lid, nremote)) return false;
         }

      repeatcounter = 0;

//      a1.Reset();
      m_to_s.clear();
      s_to_m.clear();

      time_shift = 0.;
      set_time_shift = 0.;  set_time_scale = 1.;

      while (repeatcounter < numcycles) {

         msg->master_time = 0.;
         msg->slave_shift = 0.;
         msg->slave_time = 0;
         msg->slave_scale = 1.;
         msg->msgid = repeatcounter;

         bool needreset = false;

         // apply fine shift when 1/3 of work is done
         if ((repeatcounter==numcycles*2/3) && dosynchronisation) {
            s_to_m.Sort(); m_to_s.Sort();
            time_shift = (s_to_m.Mean(max_cut) - m_to_s.Mean(max_cut)) / 2.;

            set_time_shift = time_shift;
            msg->slave_shift = time_shift;
            double sync_t = fStamping();
            if (doscaling) {
               msg->slave_scale = 1./(1.-time_shift/(sync_t - fSyncTimes[nremote]));
               set_time_scale = msg->slave_scale;
            }
            fSyncTimes[nremote] = sync_t;

            needreset = true;
         }

         // change slave time with first sync message
         if ((repeatcounter==0) && dosynchronisation && !doscaling) {
            msg->master_time = fStamping() + 0.000010;
            needreset = true;
         }

         // from here we start measure time
         double send_time = fStamping();
         double recv_time = send_time;

         // if we try to submit recv buffer only when it is necessary
         if (time_sync_short)
            if (!Pool_Post(false, GetExclusiveIndx(), sync_lid, nremote)) return false;

         // sent data and do not wait any completion
         if (!Pool_Post(true, sendbufindx, sync_lid, nremote, sizeof(IbTestTymeSyncMessage)))
            return false;

         // to check how int64_t working post_send_buffer, measure time again
         double now = fStamping();
         double stoptm = now + 0.05;

         int res_recv_index(-1), res_send_index(-1);
         while ((now<stoptm) && ((res_recv_index<0) || (res_send_index<0))) {
            int resindx, resnode, reslid;
            int res = Pool_Check(resindx, reslid, resnode, stoptm - now, 0.001);

            now = fStamping();

            if ((res>0) && ((resnode!=nremote) || (reslid != sync_lid))) {
              EOUT("Error node number %d or lid %d when complete %s operation", resnode, reslid, ((res==10) ? "send" : "receive") );
              return false;
            }
            if (res==10) { res_send_index = resindx; } else
            if (res==1) { recv_time = now; res_recv_index = resindx; }
         }

          if (res_recv_index<0) {
             EOUT("TimeSync: Error when complete receive operation");
             return false;
          }

          IbTestTymeSyncMessage* rcv = (IbTestTymeSyncMessage*) GetPoolBuffer(res_recv_index);

          // use only those round-trip packets, which are not too far from mean value
          if (needreset) {
             m_to_s.clear();
//             a1.Reset();
             s_to_m.clear();
             time_shift = 0.;
          } else
          if (repeatcounter>0) {
//             a1.Fill((rcv->slave_time - send_time)*1e6);
             m_to_s.push_back(rcv->slave_time - send_time);
             s_to_m.push_back(recv_time - rcv->slave_time);
          }

          if (rcv->msgid!=repeatcounter) {
             EOUT("Mismatch in ID %d %d", rcv->msgid, repeatcounter);
          }

          if (!time_sync_short && (repeatcounter + RecvQueue(sync_lid, nremote) + 1 < numcycles))
             Pool_Post(false, res_recv_index, sync_lid, nremote);
          else
             ReleaseExclusive(res_recv_index);

          if (res_send_index!=sendbufindx) {
             EOUT("TimeSync: Error index when complete send sync message %d != %d", res_send_index, sendbufindx);
             return false;
          }

          repeatcounter++;

//           MicroDelay(10);
      }

      m_to_s.Sort(); s_to_m.Sort();
      time_shift = (s_to_m.Mean(max_cut) - m_to_s.Mean(max_cut)) / 2.;

      DOUT0("Round trip to %2d: %5.2f microsec", nremote, m_to_s.Mean(max_cut)*1e6 + s_to_m.Mean(max_cut)*1e6);
      DOUT0("   Master -> Slave  : %5.2f  +- %4.2f (max = %5.2f min = %5.2f)", m_to_s.Mean(max_cut)*1e6, m_to_s.Dev(max_cut)*1e6, m_to_s.Max()*1e6, m_to_s.Min()*1e6);
      DOUT0("   Slave  -> Master : %5.2f  +- %4.2f (max = %5.2f min = %5.2f)", s_to_m.Mean(max_cut)*1e6, s_to_m.Dev(max_cut)*1e6, s_to_m.Max()*1e6, s_to_m.Min()*1e6);
//      if (nremote==1) a1.ShowHist();

      if (debug_output) {
         std::string fname1 = dabc::format("m_to_s%d.txt", nremote);
         std::ofstream f1(fname1.c_str());
         for (unsigned n=0;n<m_to_s.size();n++)
            f1 << m_to_s[n]*1e6 << std::endl;

         std::string fname2 = dabc::format("s%d_to_m.txt", nremote);
         std::ofstream f2(fname2.c_str());

         for (unsigned n=0;n<s_to_m.size();n++)
            f2 << s_to_m[n]*1e6 << std::endl;
      }


      if (dosynchronisation)
         DOUT0("   SET: Shift = %5.2f  Coef = %12.10f", set_time_shift*1e6, set_time_scale);
      else {
         DOUT0("   GET: Shift = %5.2f", time_shift*1e6);
         get_shift.Fill(time_shift*1e6);
      }
   }


   ReleaseExclusive(sendbufindx);

   if (!dosynchronisation) get_shift.Show("GET shift", true);

   DOUT0("Tyme sync done in %5.4f sec", fStamping() - starttm);

   return MasterCommandRequest(IBTEST_CMD_TEST);
}



bool IbTestWorkerModule::MasterTiming()
{
   // here we test how IB is working if
   // send operation happens before recv.
   // First node starts send operation to all other,
   // than we waiting 1 sec and start rec operation on other nodes.
   // Measured time, which happens after post_recv and first coming packet.

   double arguments[2];

   arguments[0] = fStamping()+1.;
   arguments[1] = 10;

   if (!MasterCreatePool(lrint(arguments[1])*NumNodes(), 1024*64)) return false;

   DOUT0("=====================================");
   DOUT0("MasterTiming()");

   if (!MasterCommandRequest(IBTEST_CMD_TIMING, arguments, sizeof(arguments))) return false;

   if (!ExecuteTiming(arguments)) return false;

   int setsize = 2;
   double allres[setsize*NumNodes()];

   if (!MasterCommandRequest(IBTEST_CMD_COLLECT, 0, 0, allres, setsize*sizeof(double))) return false;

   MasterCleanup(0);

   return true;
}


bool IbTestWorkerModule::ExecuteTiming(double* arguments)
{
   AllocResults(2);

   fResults[0] = 0;
   fResults[1] = 1;

   double starttm = arguments[0];
   int numtestbuf = lrint(arguments[1]);

   if (Node()==0) {
      DOUT0("Send from first node to all other %d buffers", numtestbuf);
      for (int k=0;k<numtestbuf;k++)
        for (int node=1;node<NumNodes();node++)
          Pool_Post(true, GetExclusiveIndx(), 0, node);
   }

   DOUT0("Wait %4.2f s", starttm-fStamping());

   while (fStamping()<starttm);

   if (Node()>0) {
//      if (Node()==2) MicroSleep(100000);
      for (int k=0;k<numtestbuf;k++)
         Pool_Post(false, GetExclusiveIndx(), 0, 0);
   }

   double start(0), stop(0);

   int noper = numtestbuf;
   if (Node()==0) noper*=(NumNodes()-1);

   double now = fStamping();

   while ((noper > 0) && (now < starttm + 5.)) {
      int res, resindx, resnode, reslid;
      res = Pool_Check(resindx, reslid, resnode, 0.1);
      now = fStamping();
      if (res<0) return false;
      if (res==0) continue;
      noper--;
      ReleaseExclusive(resindx);
      if (start==0) start = now;
      stop = now;
   }

   if (Node()>0)
      DOUT0("Start: %7.5f s  Full time = %7.5f s rate %5.1f MB/s", (start-starttm), (stop-start), numtestbuf*64.*1024./(stop-start)*1e-6);
   else
      DOUT0("ExecuteTiming() done");

   return true;
}


bool IbTestWorkerModule::ExecuteAllToAll(double* arguments)
{
   int bufsize = (int) arguments[0];
//   int NumUsedNodes = arguments[1];
   int max_sending_queue = (int) arguments[2];
   int max_receiving_queue = (int) arguments[3];
   double starttime = arguments[4];
   double stoptime = arguments[5];
   int patternid = (int) arguments[6];
   double datarate = arguments[7];
   double ratemeterinterval = arguments[8];
   bool canskipoperation = arguments[9] > 0;

   const unsigned gpuqueuesize(50); // queue which is prepared for IB operation
   bool dogpuwrite = arguments[10] > 0;
   bool dogpuread = arguments[11] > 0;

   // when receive operation should be prepared before send is started
   // we believe that 100 microsec is enough
   double recv_pre_time = 0.0001;

   AllocResults(14);

   DOUT2("ExecuteAllToAll - start");

   dabc::Ratemeter sendrate, recvrate, multirate;
   dabc::Ratemeter** fIndividualRates = 0;
   double fMeasureInterval = 0; // set 0 to disable individual time measurements

   if (fMeasureInterval>0) {
       fIndividualRates = new dabc::Ratemeter*[NumNodes()];
       for (int n=0;n<NumNodes();n++) {
          fIndividualRates[n] = 0;
          if (n!=Node()) {
              fIndividualRates[n] = new dabc::Ratemeter;
              fIndividualRates[n]->DoMeasure(fMeasureInterval, 10000, starttime);
          }
       }
   }

   // this is interval for ratemeter, which can be requested later from master
   if (ratemeterinterval>0) {
       if (fRecvRatemeter==0) fRecvRatemeter = new dabc::Ratemeter;
       if (fSendRatemeter==0) fSendRatemeter = new dabc::Ratemeter;
       if (fWorkRatemeter==0) fWorkRatemeter = new dabc::Ratemeter;

       long npoints = lrint((stoptime-starttime) / ratemeterinterval);
       if (npoints>1000000) npoints=1000000;
       fRecvRatemeter->DoMeasure(ratemeterinterval, npoints, starttime);
       fSendRatemeter->DoMeasure(ratemeterinterval, npoints, starttime);
       fWorkRatemeter->DoMeasure(ratemeterinterval, npoints, starttime);
   } else {
      if (fRecvRatemeter) { delete fRecvRatemeter; fRecvRatemeter = 0; }
      if (fSendRatemeter) { delete fSendRatemeter; fSendRatemeter = 0; }
      if (fWorkRatemeter) { delete fWorkRatemeter; fWorkRatemeter = 0; }
   }

   // schedule variables

   for (int lid=0;lid<NumLids();lid++)
      for (int n=0;n<NumNodes();n++) {
         if (SendQueue(lid,n)>0) EOUT("!!!! Problem in SendQueue[%d,%d]=%d",lid,n,SendQueue(lid,n));
         if (RecvQueue(lid,n)>0) EOUT("!!!! Problem in RecvQueue[%d,%d]=%d",lid,n,RecvQueue(lid,n));
      }

   // schstep is in seconds, but datarate in MBytes/sec, therefore 1e-6
   double schstep = 1e-6 * bufsize / datarate;

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

   DOUT4("ExecuteAllToAll: Define schedule");

   fSendSch.FillReceiveSchedule(fRecvSch);

   for (unsigned node=0; node<fActiveNodes.size(); node++)
      if (!fActiveNodes[node]) {
         fSendSch.ExcludeInactiveNode(node);
         fRecvSch.ExcludeInactiveNode(node);
      }

   if (patternid==6) // disable receive operation 1->0 to check that rest data will be transformed
      for (int nslot=0; nslot<fRecvSch.numSlots(); nslot++)
         if (fRecvSch.Item(nslot, 0).node == 1) {
            // DOUT0("Disable recv operation 1->0");
            fRecvSch.Item(nslot, 0).Reset();
          }

   double send_basetm(starttime), recv_basetm(starttime);
   int send_slot(-1), recv_slot(-1);

   bool dosending = fSendSch.ShiftToNextOperation(Node(), send_basetm, send_slot);
   bool doreceiving = fRecvSch.ShiftToNextOperation(Node(), recv_basetm, recv_slot);

   int send_total_limit = TotalNumBuffers();
   int recv_total_limit = TotalNumBuffers();

   // gave most buffers for receiving
   if (dosending && doreceiving) {
      send_total_limit = TotalNumBuffers() / 10 - 1;
      recv_total_limit = TotalNumBuffers() / 10 * 9 - 1;
   }

//   if (IsMaster()) {
//      fSendSch.Print(0);
//      fRecvSch.Print(0);
//   }

   int res, resindx, resnode, reslid;

   dabc::Average send_start, send_compl, recv_compl, loop_time;

   double lastcmdchecktime = fStamping();

   DOUT2("ExecuteAllToAll: Prepare first operations dosend %s dorecv %s remains: %5.3fs", DBOOL(dosending), DBOOL(doreceiving), starttime - fStamping());

   // counter for must have send operations over each channel
   IbTestIntMatrix sendcounter(NumLids(), NumNodes());

   // actual received id from sender
   IbTestIntMatrix recvcounter(NumLids(), NumNodes());

   // number of recv skip operations one should do
   IbTestIntMatrix recvskipcounter(NumLids(), NumNodes());

   dabc::Queue<int> gpu_read(gpuqueuesize);
   dabc::Queue<int> gpu_write(gpuqueuesize);

#ifdef WITH_GPU

   const unsigned gpuopersize(5);   // number of simultaneous GPU operations

   dabc::Queue<int> gpu_readpoolindx(gpuopersize); // queue of memory pool buffer indexes, used in the reading from GPU
   dabc::Queue<int> gpu_writepoolindx(gpuopersize); // queue of memory pool buffer indexes, used in the writing to GPU
   dabc::Queue<opencl::QueueEvent> gpu_read_ev(gpuopersize); // current submitted read events
   dabc::Queue<opencl::QueueEvent> gpu_write_ev(gpuopersize); // current submitted write events

   opencl::ContextRef *gpu_ctx(0);
   opencl::Memory** gpu_mem(0);
   int gpu_mem_cnt(0), gpu_mem_num(0);
   long gpu_oper_cnt(0);
   opencl::CommandsQueue *gpu_read_queue(0), *gpu_write_queue(0);

   if (dogpuread || dogpuwrite) {
      gpu_ctx = new opencl::ContextRef;
      if (!gpu_ctx->OpenGPU()) return false;
      gpu_mem_num = gpu_ctx->getTotalMemory() / fBufferSize / 2;
      if (gpu_mem_num>100) gpu_mem_num = 100;
      if (gpu_mem_num<2) {
         EOUT("Too few memory on GPU for the test - break");
         return false;
      }

      gpu_mem = new opencl::Memory* [gpu_mem_num];

      for (int n=0;n<gpu_mem_num;n++) {
         gpu_mem[n] = new opencl::Memory(*gpu_ctx, fBufferSize);
         if (gpu_mem[n]->null()) {
            EOUT("Cannot allocate buffer num %d size %d on GPU", n, fBufferSize);
            return false;
         }
      }

      gpu_read_queue = new opencl::CommandsQueue(*gpu_ctx);
      gpu_write_queue = new opencl::CommandsQueue(*gpu_ctx);

      // initialize all buffers we have
      for (int indx=0;indx<fTotalNumBuffers;indx++) {
         opencl::QueueEvent evnt;

         if (!gpu_write_queue->SubmitWrite(evnt, *(gpu_mem[indx % gpu_mem_num]), GetPoolBuffer(indx), fBufferSize)) {
            EOUT("Failure when trying write memory %d from pool buffer %d", indx, indx % gpu_mem_num);
            break;
         }
         gpu_write_queue->WaitComplete(evnt, 1.0);

         if (!gpu_read_queue->SubmitRead(evnt, *(gpu_mem[indx % gpu_mem_num]), GetPoolBuffer(indx), fBufferSize)) {
            EOUT("Failure when trying read memory %d to pool buffer %d", indx, indx % gpu_mem_num);
            break;
         }
         gpu_read_queue->WaitComplete(evnt, 1.0);
      }

   }

#endif

   sendcounter.Fill(0);
   recvcounter.Fill(-1);
   recvskipcounter.Fill(0);


   long numlostpackets(0);

   long totalrecvpackets(0), numsendpackets(0), numcomplsend(0);

   int64_t sendmulticounter = 0;
   int64_t skipmulticounter = 0;
   int64_t numlostmulti = 0;
   int64_t totalrecvmulti = 0;
   int64_t skipsendcounter = 0;

   double last_tm(-1.), curr_tm(0.);

   dabc::CpuStatistic cpu_stat;

   double cq_waittime = 0.;
   if (patternid==-2) cq_waittime = 0.001;

   if (dogpuwrite>0 || dogpuread>0)
      DOUT0("ExecuteAllToAll: Entering main loop, remains: %5.3fs", starttime - fStamping());

   while ((curr_tm=fStamping()) < stoptime) {

      if ((last_tm>0) && (last_tm>starttime))
         loop_time.Fill(curr_tm - last_tm);
      last_tm = curr_tm;

      // just indicate that we were active at this time interval
      if (fWorkRatemeter) fWorkRatemeter->Packet(1, curr_tm);

      res = Pool_Check(resindx, reslid, resnode, cq_waittime);

//      bool wascompletion = false;

      if (res<0) {
         EOUT("Error when Pool_Check");
         break;
      } else
      if (res==1) { // start of receiving part
//          wascompletion = true;
          double* mem = (double*) GetPoolBuffer(resindx);
          double sendtime = *mem++;
          int sendcnt = (int) *mem++;

//          if (sendcnt!=recvcounter[resnode]+1)
//             EOUT("Receive error: source:%d last pkt:%ld  curr:%ld diff:%ld",resnode, recvcounter[resnode], sendcnt, sendcnt-recvcounter[resnode]);

          int lost = sendcnt - recvcounter(reslid, resnode) - 1;
          if (lost>0) {
             numlostpackets += lost;
             recvskipcounter(reslid, resnode) += lost;
          }

          recvcounter(reslid, resnode) = sendcnt;

          double recv_time = fStamping();

          recv_compl.Fill(recv_time - sendtime);

          recvrate.Packet(bufsize, recv_time);
          if ((fIndividualRates!=0) && (fIndividualRates[resnode]!=0))
             fIndividualRates[resnode]->Packet(bufsize, recv_time);

          if (fRecvRatemeter) fRecvRatemeter->Packet(bufsize, recv_time);

          totalrecvpackets++;

          if (dogpuwrite>0) {
             if (gpu_write.Full()) {
                ReleaseExclusive(resindx);
                EOUT("No more place in gpu_write queue");
             } else
                gpu_write.Push(resindx);
          } else {
             ReleaseExclusive(resindx);
          }
      } else      // end of receiving part
      if (res==10) {
//         wascompletion = true;
         double* mem = (double*) GetPoolBuffer(resindx);
         double sendtime = *mem;
         numcomplsend++;

         double sendcompltime = fStamping();

         send_compl.Fill(sendcompltime - sendtime);

         sendrate.Packet(bufsize, sendcompltime);

         if (fSendRatemeter) fSendRatemeter->Packet(bufsize, sendcompltime);

         ReleaseExclusive(resindx);
      }  // end of send completion part


      // this is submit of recv operation

      if (doreceiving && (TotalRecvQueue() < recv_total_limit)) {

         double next_recv_time = recv_basetm + fRecvSch.timeSlot(recv_slot);

         curr_tm = fStamping();

         if (next_recv_time - recv_pre_time < curr_tm) {
            int node = fRecvSch.Item(recv_slot, Node()).node;
            int lid = fRecvSch.Item(recv_slot, Node()).lid;
            bool did_submit = false;

            if ((recvskipcounter(lid, node) > 0) && (RecvQueue(lid, node)>0)) {
               // if packets over this channel were lost, we should not try
               // to receive them - they were skipped by sender most probably
               recvskipcounter(lid, node)--;
               did_submit = true;
            } else
            if (RecvQueue(lid, node) < max_receiving_queue) {
               int recvbufindx = GetExclusiveIndx();
               if (recvbufindx<0) {
                  EOUT("Not enough buffers");
                  return false;
               }

               if (!Pool_Post(false, recvbufindx, lid, node)) {
                  EOUT("Cannot submit receive operation to lid %d node %d", lid, node);
                  return false;
               }

               did_submit = true;
            } else
            if (canskipoperation) {
               // skip recv operation - other node may be waiting for me
               did_submit = true;
            }

            // shift to next operation, even if previous was not submitted.
            // dangerous but it is like it is - we should continue receiving otherwise input will be blocked
            // we should do here more smart solutions

            if (did_submit)
               doreceiving = fRecvSch.ShiftToNextOperation(Node(), recv_basetm, recv_slot);
         }
      }

      // this is submit of send operation
      if (dosending && (TotalSendQueue() < send_total_limit)) {
         double next_send_time = send_basetm + fSendSch.timeSlot(send_slot);

         curr_tm = fStamping();

         if ((next_send_time < curr_tm) && (curr_tm<stoptime-0.1)) {

            int node = fSendSch.Item(send_slot, Node()).node;
            int lid = fSendSch.Item(send_slot, Node()).lid;
            bool did_submit = false;

            if (canskipoperation && ((SendQueue(lid, node) >= max_sending_queue) || (TotalSendQueue() >= 2*max_sending_queue)) ) {
               did_submit = true;
               skipsendcounter++;
            } else
            if ( (SendQueue(lid, node) < max_sending_queue) && (TotalSendQueue() < 2*max_sending_queue) ) {

               int sendbufindx = -1;

               if (dogpuread>0) {
                  if (!gpu_read.Empty())
                     sendbufindx = gpu_read.Pop();
                  else
                     EOUT("No enough buffers in gpu_read queue");
               } else
                  sendbufindx = GetExclusiveIndx();
               if (sendbufindx<0) {
                  EOUT("Not enough buffers");
                  break;
               }
               double* mem = (double*) GetPoolBuffer(sendbufindx);
               *mem = curr_tm;
               mem++; *mem = sendcounter(lid,node);

               send_start.Fill(curr_tm - next_send_time);

               if (!Pool_Post(true, sendbufindx, lid, node, bufsize)) {
                  EOUT("ExecuteAllToAll cannot send to lid %d node %d", lid, node);
                  break;
               }

               numsendpackets++;
               did_submit = true;
            }

            // shift to next operation, in some cases even when previous operation was not performed
            if (did_submit) {
               sendcounter(lid,node)++; // this indicates that we perform operation and moved to next counter
               dosending = fSendSch.ShiftToNextOperation(Node(), send_basetm, send_slot);
            }
         }
      }

#ifdef WITH_GPU

      if (dogpuwrite) {

          // first check if already submitted events completed
          while (!gpu_write_ev.Empty()) {
             bool do_again(false);
             switch (gpu_write_queue->CheckComplete(gpu_write_ev.Front())) {
               case -1:
                  EOUT("GPU WaitWrite failed");
                  dogpuwrite = false;
                  gpu_write_ev.PopOnly();
                  ReleaseExclusive(gpu_writepoolindx.Pop());
                  break;
               case 1:
                  gpu_oper_cnt++;
                  gpu_write_ev.PopOnly();
                  ReleaseExclusive(gpu_writepoolindx.Pop());
                  do_again = true;
                  break;
               default:
                  break;
            }
            if (!do_again) break;
          }

          // and now try to submit as much write operations as possible
          while (!gpu_write_ev.Full() && !gpu_write.Empty()) {
             int indx = gpu_write.Pop();
             opencl::QueueEvent evnt;
             if (gpu_write_queue->SubmitWrite(evnt, *(gpu_mem[gpu_mem_cnt]), GetPoolBuffer(indx), fBufferSize)) {
                dogpuwrite = 2;
                gpu_mem_cnt = (gpu_mem_cnt+1) % gpu_mem_num;
                gpu_write_ev.Push(evnt);
                gpu_writepoolindx.Push(indx);
             } else {
                EOUT("GPU SubmitWrite failed - disable");
                dogpuwrite = false;
                ReleaseExclusive(indx);
             }
          }
      }

      if (dogpuread) {

         // first check if already submitted events are processed
         while (!gpu_read_ev.Empty()) {
            bool do_again(false);
            switch (gpu_read_queue->CheckComplete(gpu_read_ev.Front())) {
               case -1:
                  EOUT("GPU WaitRead failed");
                  ReleaseExclusive(gpu_readpoolindx.Pop());
                  gpu_read_ev.PopOnly();
                  dogpuread = false;
                  break;
               case 1:
                  gpu_read.Push(gpu_readpoolindx.Pop());
                  gpu_read_ev.PopOnly();
                  do_again = true;
                  gpu_oper_cnt++;
                  break;
               default:
                  break;
            }
            if (!do_again) break;
         }

         // do not start new read operations when already submitted could fill complete queue
         while (!gpu_readpoolindx.Full() && (gpu_readpoolindx.Size() + gpu_read.Size() < gpuqueuesize)) {

            int indx = GetExclusiveIndx();
            opencl::QueueEvent evnt;

            if (gpu_read_queue->SubmitRead(evnt, *(gpu_mem[gpu_mem_cnt]), GetPoolBuffer(indx), fBufferSize)) {
               gpu_readpoolindx.Push(indx);
               gpu_read_ev.Push(evnt);
               gpu_mem_cnt = (gpu_mem_cnt+1) % gpu_mem_num;
            } else {
               EOUT("GPU SubmitRead failed");
               dogpuread = false;
               break;
            }
         }
      }

#endif

      if (curr_tm > lastcmdchecktime + 0.1) {
         if (Node()>0 && CanRecv()) {
             // set stop time in 0.1 s to allow completion of all send operation
             if (curr_tm+0.1 < stoptime) stoptime = curr_tm+0.1;
             // disable send operation anyhow
             dosending = false;

             EOUT("Did emergence stop while command is arrived");
             // do not do immediate break
             // break;
         }
         lastcmdchecktime = curr_tm;
         if (curr_tm < starttime) cpu_stat.Reset();
      }
   }

   DOUT3("Total recv queue %ld limit %ld send queue %ld", TotalSendQueue(), recv_total_limit, TotalRecvQueue());
   DOUT3("Do recv %s curr_tm %8.7f next_tm %8.7f slot %d node %d lid %d queue %d",
         DBOOL(doreceiving), curr_tm, recv_basetm + fRecvSch.timeSlot(recv_slot), recv_slot,
         fRecvSch.Item(recv_slot, Node()).node, fRecvSch.Item(recv_slot, Node()).lid,
         RecvQueue(fRecvSch.Item(recv_slot, Node()).lid, fRecvSch.Item(recv_slot, Node()).node));
   DOUT3("Do send %s curr_tm %8.7f next_tm %8.7f slot %d node %d lid %d queue %d",
         DBOOL(dosending), curr_tm, send_basetm + fSendSch.timeSlot(send_slot), send_slot,
         fSendSch.Item(send_slot, Node()).node, fSendSch.Item(send_slot, Node()).lid,
         SendQueue(fSendSch.Item(send_slot, Node()).lid, fSendSch.Item(send_slot, Node()).node));

   DOUT3("Send operations diff %ld: submited %ld, completed %ld", numsendpackets-numcomplsend, numsendpackets, numcomplsend);

   cpu_stat.Measure();

   DOUT5("CPU utilization = %5.1f % ", cpu_stat.CPUutil()*100.);

   if (numsendpackets!=numcomplsend) {
      EOUT("Mismatch %d between submitted %ld and completed send operations %ld",numsendpackets-numcomplsend, numsendpackets, numcomplsend);

      for (int lid=0;lid<NumLids(); lid++)
        for (int node=0;node<NumNodes();node++) {
          if ((node==Node()) || !fActiveNodes[node]) continue;
          DOUT5("   Lid:%d Node:%d RecvQueue = %d SendQueue = %d recvskp %d", lid, node, RecvQueue(lid,node), SendQueue(lid,node), recvskipcounter(lid, node));
       }
   }

   fResults[0] = sendrate.GetRate();
   fResults[1] = recvrate.GetRate();
   fResults[2] = send_start.Mean();
   fResults[3] = send_compl.Mean();
   fResults[4] = numlostpackets;
   fResults[5] = totalrecvpackets;
   fResults[6] = numlostmulti;
   fResults[7] = totalrecvmulti;
   fResults[8] = multirate.GetRate();
   fResults[9] = cpu_stat.CPUutil();
   fResults[10] = loop_time.Mean();
   fResults[11] = loop_time.Max();
   fResults[12] = (numsendpackets+skipsendcounter) > 0 ? 1.*skipsendcounter / (numsendpackets + skipsendcounter) : 0.;
   fResults[13] = recv_compl.Mean();

   DOUT5("Multi: total %ld lost %ld", totalrecvmulti, numlostmulti);

   if (sendmulticounter+skipmulticounter>0)
      DOUT0("Multi send: %ld skipped %ld (%5.3f)",
             sendmulticounter, skipmulticounter,
             100.*skipmulticounter/(1.*sendmulticounter+skipmulticounter));

   if (fIndividualRates!=0) {

      char fname[100];
      sprintf(fname,"recv_rates_%d.txt",Node());

      dabc::Ratemeter::SaveRatesInFile(fname, fIndividualRates, NumNodes(), true);

      for (int n=0;n<NumNodes();n++)
         delete fIndividualRates[n];
      delete[] fIndividualRates;
   }

#ifdef WITH_GPU

   // wait completion of events when some of them not completed
   if (dogpuwrite) {
      while (!gpu_write_ev.Empty()) {
         gpu_write_queue->WaitComplete(gpu_write_ev.Front(), 1.);
         gpu_write_ev.PopOnly();
      }
      while (!gpu_writepoolindx.Empty())
         ReleaseExclusive(gpu_writepoolindx.Pop());
      while (!gpu_write.Empty())
         ReleaseExclusive(gpu_write.Pop());
   }
   if (dogpuread) {
      while (!gpu_read_ev.Empty()) {
         gpu_read_queue->WaitComplete(gpu_read_ev.Front(), 1.);
         gpu_read_ev.PopOnly();
      }
      while (!gpu_readpoolindx.Empty())
         ReleaseExclusive(gpu_readpoolindx.Pop());
      while (!gpu_read.Empty())
         ReleaseExclusive(gpu_read.Pop());
   }

   if (gpu_oper_cnt>0)
     DOUT0("gpu_oper_cnt = %ld recv_cnt %ld send cnt %ld", gpu_oper_cnt, totalrecvpackets, numcomplsend);

   // cleanup all GPU-specific parts
   delete gpu_read_queue;
   delete gpu_write_queue;

   for (int n=0;n<gpu_mem_num;n++)
      delete gpu_mem[n];

   delete[] gpu_mem;

   delete gpu_ctx;

#endif


   return true;
}

bool IbTestWorkerModule::MasterTestGPU(int bufsize, int testtime, bool testwrite, bool testread)
{
   // allocate 512 MB to exclude any cashing effects
   int numbuffers = 512*1024*1024/bufsize;

   if (!MasterCreatePool(numbuffers, bufsize)) return false;

   double arguments[3];

   arguments[0] = testtime; // how long
   arguments[1] = testwrite ? 1 : 0;   // do writing
   arguments[2] = testread ? 1 : 0;   // do reading

   DOUT0("=========================================");
   DOUT0("TestGPU size:%d write:%s read:%s", bufsize, DBOOL(testwrite), DBOOL(testread));

   if (!MasterCommandRequest(IBTEST_CMD_TESTGPU, arguments, sizeof(arguments))) return false;

   if (!ExecuteTestGPU(arguments)) return false;

   int setsize = 2;
   double allres[setsize*NumNodes()];
   for (int n=0;n<setsize*NumNodes();n++) allres[n] = 0.;

   if (!MasterCommandRequest(IBTEST_CMD_COLLECT, 0, 0, allres, setsize*sizeof(double))) return false;

//   DOUT((1,"Results of all-to-all test");
   DOUT0("  # |      Node |   Write |   Read ");
   double sum1(0.), sum2(0.);

   for (int n=0;n<NumNodes();n++) {

       DOUT0("%3d |%10s | %7.1f | %7.1f",
             n, dabc::mgr()->GetNodeAddress(n).c_str(),
             allres[n*setsize+0], allres[n*setsize+1]);
       sum1 += allres[n*setsize+0];
       sum2 += allres[n*setsize+1];
   }

   DOUT0("    |  Average  | %7.1f | %7.1f", sum1/NumNodes(), sum2/NumNodes());

   return MasterCommandRequest(IBTEST_CMD_TEST);
}


bool IbTestWorkerModule::ExecuteTestGPU(double* arguments)
{
   AllocResults(2);

#ifdef WITH_GPU

   double duration = arguments[0];
   bool dowrite_oper = arguments[1] > 0.;
   bool doread_oper = arguments[2] > 0.;

   const int queue_size = 2;

   if ((fBufferSize==0) || (fPool==0)) {
      EOUT("No suitable local memory for GPU tests");
      return false;
   }

   opencl::ContextRef ctx;
   if (!ctx.OpenGPU()) return false;

   { // localize all allocations inside brackets - context will be destroyed as last

   int dowrite[queue_size], doread[queue_size];

   int pool_rbuf[queue_size], pool_wbuf[queue_size];

   opencl::Memory read_buf[queue_size], write_buf[queue_size];

   opencl::QueueEvent write_ev[queue_size], read_ev[queue_size];

   opencl::CommandsQueue wqueue(ctx), rqueue(ctx);

   for (int cnt=0;cnt<queue_size;cnt++) {
      dowrite[cnt] = dowrite_oper ? 1 : 0;
      doread[cnt] = doread_oper ? 1 : 0;
      read_buf[cnt].Allocate(ctx, fBufferSize);
      write_buf[cnt].Allocate(ctx, fBufferSize);
      pool_rbuf[cnt] = -1;
      pool_wbuf[cnt] = -1;
   }

   dabc::Ratemeter write_rate, read_rate;

   dabc::TimeStamp start = dabc::Now();

   int cnt1(0), cnt2(0);

   while (!start.Expired(duration)) {

      for (int cnt=0;cnt<queue_size;cnt++)
         switch (dowrite[cnt]) {
            case 1: // submit;
               pool_wbuf[cnt] = GetExclusiveIndx();
               if (wqueue.SubmitWrite(write_ev[cnt], write_buf[cnt], GetPoolBuffer(pool_wbuf[cnt]), fBufferSize)) {
                  dowrite[cnt] = 2;
               } else {
                  EOUT("SubmitWrite failed");
                  ReleaseExclusive(pool_wbuf[cnt]);
                  dowrite[cnt] = 0;
               }
               break;
            case 2: // wait
               switch (wqueue.CheckComplete(write_ev[cnt])) {
                  case -1: EOUT("WaitWrite failed"); dowrite[cnt] = 0; break;
                  case 1: dowrite[cnt] = 1; ReleaseExclusive(pool_wbuf[cnt]); if (cnt1++>queue_size) write_rate.Packet(fBufferSize); break;
                  default: break;
               }
               break;
            default:
               break;
         }

      for (int cnt=0;cnt<queue_size;cnt++)
         switch (doread[cnt]) {
            case 1: // submit;
               pool_rbuf[cnt] = GetExclusiveIndx();
               if (rqueue.SubmitRead(read_ev[cnt], read_buf[cnt], GetPoolBuffer(pool_rbuf[cnt]), fBufferSize)) {
                  doread[cnt] = 2;
               } else {
                  EOUT("SubmitRead failed");
                  doread[cnt] = 0;
                  ReleaseExclusive(pool_rbuf[cnt]);
               }
               break;
            case 2: // wait
               switch (rqueue.CheckComplete(read_ev[cnt])) {
                  case -1: EOUT("WaitRead failed"); doread[cnt] = 0; break;
                  case 1: doread[cnt] = 1; ReleaseExclusive(pool_rbuf[cnt]); if (cnt2++>queue_size) read_rate.Packet(fBufferSize); break;
                  default: break;
               }
               break;
            default:
               break;
         }
   }

   // wait completion of events when some of them not completed
   for (int cnt=0;cnt<queue_size;cnt++) {
      if (dowrite[cnt] == 2) {
         wqueue.WaitComplete(write_ev[cnt], 1.);
         ReleaseExclusive(pool_wbuf[cnt]);
      }
      if (doread[cnt] == 2) {
         rqueue.WaitComplete(read_ev[cnt], 1.);
         ReleaseExclusive(pool_rbuf[cnt]);
      }
    }

   fResults[0] = write_rate.GetRate();
   fResults[1] = read_rate.GetRate();

   } // end of extra brakets for context

   ctx.Release();

#endif

   return true;
}



bool IbTestWorkerModule::MasterAllToAll(int full_pattern,
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

   bool needmulticast = false; // (pattern==5) || (pattern==6) || (pattern==7);

   if (needmulticast && !IsMulticastSupported()) {
      EOUT("Transport not supports multicast");
      return true;
   }

   // number of buffers just depends from buffer size and amount of memory which we want to use
   // lets take 256 MB as magic number
   // buffers can be distributed between sending and receiving equally

   int numbuffers = fTestPoolSize*1024*1024 / bufsize;

   if (!MasterCreatePool(numbuffers, bufsize)) return false;

   if (needmulticast) {
      if ((pattern==5) || (pattern==6) || (pattern==7)) {
         if (!fromperfmtest || (fMultiQP==0))
            if (!MasterInitMulticast(1, 2048, 20, 50)) return false;
      } else {
         // all nodes, except first can receive (and send) multicast messages
         DOUT3("First create multicast groups on nodes [%d, %d]",1, NumNodes()-1);
         if (!MasterInitMulticast(1, 2048, 4, 20, 1, NumNodes()-1)) return false;
         // first node will be able only to send multicasts
         DOUT3("Now create multicast sender on node 0");
         if (!MasterInitMulticast(2, 2048, 4, 20, 0, 0)) return false;
         DOUT3("Now multicast is prepared");
      }
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


   double arguments[12];

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
   arguments[10] = Cfg("TestWrite").AsBool(false) ? 1 : 0;
   arguments[11] = Cfg("TestRead").AsBool(false) ? 1 : 0;

   // shift start/stop time for 3 seconds, when GPU is used - buffers registrations takes too much time
   if ((arguments[10]>0) || (arguments[11]>0)) { arguments[4]+=3.; arguments[5]+=3.; }

   DOUT0("=====================================");
   DOUT0("%s pattern:%d size:%d rate:%d send:%d recv:%d nodes:%d canskip:%s",
         pattern_name, pattern, bufsize, datarate, max_sending_queue, max_receiving_queue, NumNodes(), DBOOL(allowed_to_skip));

   if (!MasterCommandRequest(IBTEST_CMD_ALLTOALL, arguments, sizeof(arguments))) return false;

   if (!ExecuteAllToAll(arguments)) return false;

   int setsize = 14;
   double allres[setsize*NumNodes()];
   for (int n=0;n<setsize*NumNodes();n++) allres[n] = 0.;

   if (!MasterCommandRequest(IBTEST_CMD_COLLECT, 0, 0, allres, setsize*sizeof(double))) return false;

//   DOUT((1,"Results of all-to-all test");
   DOUT0("  # |      Node |   Send |   Recv |   Start |S_Compl|R_Compl| Lost | Skip | Loop | Max ms | CPU  | Multicast ");
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

       DOUT0("%3d |%10s |%7.1f |%7.1f |%8.1f |%6.0f |%6.0f |%5.0f |%s |%5.2f |%7.2f |%s |%5.1f (%5.0f)",
             n, dabc::mgr()->GetNodeAddress(n).c_str(),
           allres[n*setsize+0], allres[n*setsize+1], allres[n*setsize+2]*1e6, allres[n*setsize+3]*1e6, allres[n*setsize+13]*1e6, allres[n*setsize+4], sbuf1, allres[n*setsize+10]*1e6, allres[n*setsize+11]*1e3, cpuinfobuf, allres[n*setsize+8], allres[n*setsize+7]);
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
   DOUT0("          Total |%7.1f |%7.1f | Skip: %4.0f/%4.0f = %3.1f percent",
          sum1send, sum2recv, totallost, totalrecv, 100*(totalrecv>0. ? totallost/(totalrecv+totallost) : 0.));
   if ((totalmulti > 0) && (sumcnt > 0))
      DOUT0("Multicast %4.0f/%4.0f = %7.6f%s  Rate = %4.1f",
         totalmultilost, totalmulti, totalmultilost/totalmulti*100., "%", sum3multi/sumcnt);

   MasterCleanup(0);

   totalskipped = sumcnt > 0 ? totalskipped / sumcnt : 0.;
   if (totalskipped > 0.01) isskipok = false;

   if (needmulticast && !fromperfmtest)
      if (!MasterInitMulticast(0)) return false;

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


bool IbTestWorkerModule::MasterInitMulticast(
                            int mode,
                            int bufsize,
                            int send_queue,
                            int recv_queue,
                            int firstnode,
                            int lastnode)
{
   return false;
}




void IbTestWorkerModule::ProcessAskQueue()
{
   // method is used to calculate how many queues entries are existing

#ifdef WITH_VERBS

   int resindx, resnode, reslid, res;

   bool isanydata = false;

   double tm = fStamping();

   do {

      isanydata = false;
      res = Pool_Check(resindx, reslid, resnode, 0.001);

      if (res<0) {
           EOUT("Error when Pool_Check");
      } else
      if (res==1) { // start of receiving part
         ReleaseExclusive(resindx);
         isanydata = true;
      } else      // end of receiving part
      if (res==10) {
        ReleaseExclusive(resindx);
        isanydata = true;
      }  // end of send complition part

      // here a part where milticast data can be received
      if (fMultiCQ && fMultiPool)
         if (fMultiCQ->Poll()==1) {
            isanydata = true;
            int bufindx = fMultiCQ->arg() % 1000000;
            ReleaseExclusive(bufindx, fMultiPool);

            if (fMultiCQ->arg()>=1000000)
               fMultiSendQueue--;
            else
               fMultiRecvQueue--;

         }
   } while (isanydata && (fStamping()<tm+0.01));

#endif

   AllocResults(3);
   fResults[0] = fMultiQP ? fMultiSendQueue : 0;
   fResults[1] = fMultiQP ? fMultiRecvQueue : 0;
   fResults[2] = fMultiQP ? fMultiRecvQueue : 0;
   fResults[0] += TotalSendQueue();
   fResults[1] += TotalRecvQueue();
}

void IbTestWorkerModule::MasterCleanup(int mainnode)
{
   double tm = fStamping();

   int setsize = 3;
   double allres[NumNodes()*setsize];
   for (int n=0;n<NumNodes()*setsize;n++)
      allres[n] = 0.;

   long maxmultiqueue(0);

   double sumsend(0.), sumrecv(0.), summulti(0.);

   if (!MasterCommandRequest(IBTEST_CMD_ASKQUEUE)) return;

   if (!MasterCommandRequest(IBTEST_CMD_COLLECT, 0, 0, allres, setsize*sizeof(double))) return;

   for (int n=0;n<NumNodes();n++)
      if (fActiveNodes[n]) {
         // DOUT0("   Node %d send %3.0f recv %3.0f", n, allres[n*setsize], allres[n*setsize + 1]);
         sumsend  += allres[n*setsize];
         sumrecv  += allres[n*setsize + 1];
         summulti += allres[n*setsize + 2];
         if (allres[n*setsize + 2] > maxmultiqueue)
            maxmultiqueue = lrint(allres[n*setsize + 2]);
      }

   //       DOUT0("Total Tm %5.1f Queue sizes %3.0f %3.0f", fStamping()-tm, sumsend, sumrecv);

   if (sumsend<0) {
      EOUT("!!! PROBLEM with QUEUES !!!!");
      for (int lid=0; lid<NumLids(); lid++)
         for(int n=1;n<NumNodes();n++)
            EOUT("To node (%d,%d) send queue %d",lid, n, SendQueue(lid, n));
   }

   if ((sumsend!=0.) || (sumrecv!=0.)) {

      int64_t pars[2];
      pars[0] = mainnode;
      pars[1] = maxmultiqueue;

      if (!MasterCommandRequest(IBTEST_CMD_CLEANUP, pars, sizeof(pars))) return;

      ProcessCleanup(pars);
   }

   // for syncronisation
   if (!MasterCommandRequest(IBTEST_CMD_TEST)) return;

   DOUT0("Queues are cleaned in %5.4f s", fStamping() - tm);
}

void IbTestWorkerModule::ProcessCleanup(int64_t* pars)
{
//   DOUT((1,"Start cleanup %8.0f %x", starttm, fPool));

//   DOUT((0,"Start cleanup mcast send = %d recv = %d", fMultiSendQueue, fMultiRecvQueue));

   if (fPool==0) return;

   int64_t mainnode = pars[0];
   int64_t maxmultiqueue = pars[1];

   int64_t multisend = 0;

   // send 2 times more mcast packet then necessary
   if ((fMultiKind / 10 == 1) && fMultiQP && (Node()==mainnode))
      multisend = maxmultiqueue*2;

   double starttm = fStamping();

   std::vector<double> needsendinfo;
   needsendinfo.resize(NumNodes(), starttm);
   std::vector<bool>  needrecvinfo; // indicates if we should recv info from nodes
   needrecvinfo.resize(NumNodes(), true);

   WorkerSleep(0.001);
//   DOUT((1,"Cleaned queues");

   int res, resindx, resnode, reslid;


   int* sendoper[IBTEST_MAXLID];
   int* recvoper[IBTEST_MAXLID];
   for (int lid=0;lid<NumLids();lid++) {
      sendoper[lid] = new int[NumNodes()];
      recvoper[lid] = new int[NumNodes()];
      for (int node=0;node<NumNodes();node++) {
         sendoper[lid][node] = 0;
         recvoper[lid][node] = 0;
      }
   }

   bool is_ok = false;

   double now = fStamping();

   // Main loop, not more than 7.5 sec
   while ((now = fStamping()) < starttm + 7.5) {

      res = Pool_Check(resindx, reslid, resnode, 0.001);

      if (res==1) {
         // process recv compl event
         int* mem = (int*) GetPoolBuffer(resindx);
         if ((reslid==0) && (*mem++==-7654321)) {
            if (!needrecvinfo[resnode]) EOUT("We do not want to recv info from node %d", resnode);

            DOUT3("Get cleanup info from node %d", resnode);
            for (int lid=0; lid<NumLids(); lid++)
               sendoper[lid][resnode] = *mem++;
            for (int lid=0; lid<NumLids(); lid++)
               recvoper[lid][resnode] = *mem++;

            needrecvinfo[resnode] = *mem++ > 0;
         }

         // if still necessary packet does not arrives, resubmit packet for waiting
         if ((reslid==0) && needrecvinfo[resnode] && (RecvQueue(0, resnode)==0))
            Pool_Post(false, resindx, 0, resnode);
         else
            ReleaseExclusive(resindx);
      } else
      if (res==10) {
         // process send complition event
         ReleaseExclusive(resindx);
      } else
      if (res<0) {
         EOUT("Problem in Pool_Check");
      }

      // prepare buffers (if necessary) to recieve info from other nodes
      // it is enough to receive one buffer per remote node, which contains all queues for all lids
      for (int node=0;node<NumNodes();node++)
         if ((node!=Node()) && needrecvinfo[node] && (RecvQueue(0, node)==0) && fActiveNodes[node]) {
            resindx = GetExclusiveIndx();
            if (resindx<0) break;

            DOUT3("Post buffer for recv cleanup info from node %d", node);
            Pool_Post(false, resindx, 0, node);
         }


      // now send packet, which inform how many data is required
      for (int node=0;node<NumNodes();node++) {
         if ((node==Node()) || !fActiveNodes[node] || !needsendinfo[node]) continue;

         if ((needsendinfo[node] <= 0.) || (needsendinfo[node] > now)) continue;

         // receiver should itself clear first lid queue until we send any info to it
         if (SendQueue(0, node) > 0) continue;

         resindx = GetExclusiveIndx();
         if (resindx<0) break;

         int* mem = (int*) GetPoolBuffer(resindx);
         *mem++ = -7654321;

         // one need to send one buffer less for lid==0 while we should receive this buffer automatically with cleanup info
         for (int lid=0;lid<NumLids();lid++)
            *mem++ = RecvQueue(lid, node) - ((lid==0) && needrecvinfo[node] ? 1 : 0);
         // we also send how many items in the send queue, can be that receiver does not submit them

         bool issendagain = false;
         for (int lid=0;lid<NumLids();lid++) {
            *mem++ = SendQueue(lid, node);
            if (SendQueue(lid, node)>0) issendagain = true;
         }

         // inform other side that we repeat sending again
         *mem++ = issendagain ? 1 : 0;

         DOUT3("Send cleanup info to node %d", node);
         Pool_Post(true, resindx, 0, node);

         // repeat operation after 0.1 sec to ensure that everything went well
         needsendinfo[node] = issendagain ? (fStamping() + 0.1) : 0.;
      }

      // send data to other nodes
      for (int lid=0;lid<NumLids(); lid++)
         for (int node=0;node<NumNodes();node++) {
            if ((node==Node()) || (SendQueue(lid,node)>0) || (sendoper[lid][node]<=0)) continue;
            resindx = GetExclusiveIndx();
            if (resindx<0) break; // one need many buffers, therefore it is not that bad
            int* mem = (int*) GetPoolBuffer(resindx);
            *mem = 55555; // do not missup other nodes
            if (Pool_Post(true, resindx, lid, node)) sendoper[lid][node]--;
         }

      // receive data from other nodes
      for (int lid=0;lid<NumLids(); lid++)
         for (int node=0;node<NumNodes();node++) {
            if ((node==Node()) || (RecvQueue(lid,node)>0) || (recvoper[lid][node]==0)) continue;
            resindx = GetExclusiveIndx();
            if (resindx<0) break; // one need many buffers, therefore it is not that bad
            if (Pool_Post(false, resindx, lid, node)) recvoper[lid][node]--;
         }

      if ((multisend>0) && (fMultiSendQueue<fMultiSendQueueSize)) {
         int sendbufindx = GetExclusiveIndx(fMultiPool);
         if (sendbufindx>=0) {
            Pool_Post_Mcast(true, sendbufindx);
            multisend--;
         }
      }

      // here a part where milticast data can be received
      if (fMultiCQ && fMultiPool) {
         int bufindx;
         if (Pool_Check_Mcast(bufindx, 0.001)>0)
            ReleaseExclusive(bufindx, fMultiPool);
      }

      if (fMultiCQ && fMultiPool)
         is_ok = (fMultiSendQueue==0) && (fMultiRecvQueue==0);

      bool isownpending(false), isotherspending(false), iscontrolpending(false);

      for (int node=0;node<NumNodes();node++) {
         if ((node==Node()) || !fActiveNodes[node]) continue;
         if (needrecvinfo[node] || (needsendinfo[node]>0.)) iscontrolpending = true;

         for (int lid=0;lid<NumLids(); lid++) {
            if ((RecvQueue(lid,node)>0) || (SendQueue(lid,node)>0)) isownpending = true;
            if ((sendoper[lid][node]>0) || (recvoper[lid][node]>0)) isotherspending = true;
         }
      }

      is_ok = !isownpending && !isotherspending && !iscontrolpending;

      if (is_ok) {
        DOUT2("Cleanup finished after %5.4f s", fStamping()-starttm);
        break;
      }
   }

   if (!is_ok) {
      EOUT("Cleanup broken");
      for (int lid=0;lid<NumLids(); lid++)
         for (int node=0;node<NumNodes();node++) {
            if ((node==Node()) || !fActiveNodes[node]) continue;
            if ((RecvQueue(lid,node)>0) || (SendQueue(lid,node)>0) || (sendoper[lid][node]>0) || (recvoper[lid][node]>0))
               DOUT3("   Lid:%d Node:%d RecvQueue = %d SendQueue = %d sendoper = %d recvoper = %d",
                  lid, node, RecvQueue(lid,node), SendQueue(lid,node), sendoper[lid][node], recvoper[lid][node]);
         }
   }

//   DOUT0("Stop cleanup mcast send = %d recv = %d", fMultiSendQueue, fMultiRecvQueue);

   for (int lid=0;lid<NumLids();lid++) {
      delete[] sendoper[lid];
      delete[] recvoper[lid];
   }
}

void IbTestWorkerModule::PerformTimingTest()
{
   for (int n=0; n<5; n++) {
      DOUT0("SLEEP 10 sec N=%d", n);
      WorkerSleep(10.);
      if (n%5 == 0)
         MasterTimeSync(true, 200, true);
      else
         MasterTimeSync(false, 200, false);
   }
}

void IbTestWorkerModule::PerformTestGPU()
{
   int buffersize = Cfg("TestBufferSize").AsInt(128*1024);
   int testtime = Cfg("TestTime").AsInt(10);
   bool testwrite = Cfg("TestWrite").AsBool(true);
   bool testread = Cfg("TestRead").AsBool(true);

   MasterTestGPU(buffersize, testtime, testwrite, testread);

  for (int bufsize=4096; bufsize<=4096*1024; bufsize*=2) {
//      MasterTestGPU(bufsize, 5., false, true);
//      MasterTestGPU(bufsize, 5., true, false);
      MasterTestGPU(bufsize, 5, true, true);
  }

}


void IbTestWorkerModule::PerformSingleRouteTest()
{
   DOUT0("This is small suite to test single route performance");

   MasterAllToAll(15, 128*1024, 10., 1000, 5, 10);

   MasterAllToAll(15, 1024*1024, 10., 1000, 5, 10);

   MasterAllToAll(5, 128*1024, 10., 940, 5, 10);

   MasterAllToAll(5, 128*1024, 10., 1900, 5, 10);

   MasterAllToAll(5, 1024*1024, 10., 1945, 5, 10);
}

void IbTestWorkerModule::PerformNormalTest()
{
//   MasterTiming();

//   DOUT0("SLEEP 1 sec");
//   WorkerSleep(1.);

   int outq = Cfg("TestOutputQueue").AsInt(5);
   int inpq = Cfg("TestInputQueue").AsInt(10);
   int buffersize = Cfg("TestBufferSize").AsInt(128*1024);
   int rate = Cfg("TestRate").AsInt(1000);
   int testtime = Cfg("TestTime").AsInt(10);
   int testpattern = Cfg("TestPattern").AsInt(0);

//   fTrendingInterval = 0.0002;
   MasterAllToAll(testpattern, buffersize, testtime, rate, outq, inpq);
//   if (fWorkRatemeter) fWorkRatemeter->SaveInFile("mywork1K.txt");
//   fTrendingInterval = 0;

   MasterAllToAll(testpattern, buffersize, testtime, rate, outq, inpq);

//   MasterAllToAll(0, 2*1024,   5., 100., 5, 10);

//   MasterAllToAll(-1, 128*1024, 5., 1000., 5, 10);

//   MasterAllToAll(0,  128*1024, 5., 500., 5, 10);

//   MasterAllToAll(-1, 1024*1024, 5., 900., 2, 4);

//   MasterAllToAll(0, 1024*1024, 5., 2300., 2, 4);
}

void IbTestWorkerModule::MainLoop()
{
   if (IsSlave()) {
      SlaveMainLoop();
      return;
   }

   DOUT0("Entering mainloop master: %s test = %s", DBOOL(IsMaster()), fTestKind.c_str());

   // here we are doing our test sequence (like in former verbs tests)

   MasterCollectActiveNodes();

   CalibrateCommandsChannel();

   if (fTestKind == "OnlyConnect") {

      DOUT0("----------------- TRY ONLY COMMAND CHANNEL ------------------- ");

      CalibrateCommandsChannel(30);

      MasterCallExit();

      dabc::mgr.StopApplication();
      return;
   }


   DOUT0("----------------- TRY CONN ------------------- ");

   MasterConnectQPs();

   DOUT0("----------------- DID CONN !!! ------------------- ");


   MasterTimeSync(true, 200, false, fTestKind == "DSimple");

   if ((fTestKind != "Simple") && (fTestKind != "DSimple")) {

      DOUT0("SLEEP 10 sec");
      WorkerSleep(10.);

      MasterTimeSync(true, 200, true);

      if (fTestKind == "SimpleSync") {
         DOUT0("Sleep 10 sec more before end");
         WorkerSleep(10.);
      } else
      if (fTestKind == "TimeSync") {
         PerformTimingTest();
      } else
      if (fTestKind == "TestGPU") {
         PerformTestGPU();
      } else
      if (fTestKind == "SingleRoute") {
         PerformSingleRouteTest();
      } else{
         PerformNormalTest();
      }

      MasterTimeSync(false, 500, false);
   }

   MasterCloseConnections();

   MasterCallExit();

   dabc::mgr.StopApplication();
}


