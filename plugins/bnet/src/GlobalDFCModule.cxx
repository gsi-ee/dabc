#include "bnet/GlobalDFCModule.h"

#include "bnet/ClusterApplication.h"

#include "dabc/Port.h"
#include "dabc/logging.h"
#include "dabc/PoolHandle.h"

bnet::GlobalDFCModule::GlobalDFCModule(dabc::Manager* m, const char* name, ClusterApplication* plugin) : 
   dabc::ModuleAsync(m, name),
   fPool(0),
   fMap(),
   fCfgNumNodes(plugin->CfgNumNodes()),
   fSendNodes(),
   fRecvNodes()
{
   fPool = CreatePool(plugin->ControlPoolName());

   fBufferSize = plugin->ControlBufferSize();
   
   CreatePoolUsageParameter("CtrlPoolUsage", 1., plugin->ControlPoolName());
   
   CreateTimer("HeartBeat", 0.1); // every 100 milisec one timer event

   for (int n=0;n<fCfgNumNodes;n++) 
     CreatePort(FORMAT(("Sender%d", n)), fPool, CtrlInpQueueSize, CtrlOutQueueSize);
   
   fTargetCounter = 0;
   
   fLastSendTime = dabc::NullTimeStamp;
}

bnet::GlobalDFCModule::~GlobalDFCModule()
{
}

int bnet::GlobalDFCModule::ExecuteCommand(dabc::Command* cmd)
{
   if (cmd->IsName("Configure")) {
       
       DOUT3(("Controller Get command Configure"));
       
      // now, create everything that we need
      fSendNodes.Reset(cmd->GetStr("SendMask"), fCfgNumNodes);
      fRecvNodes.Reset(cmd->GetStr("RecvMask"), fCfgNumNodes);
      
      DOUT1(("Controller Configure done nout = %d", NumOutputs())); 
       
   } else
      return dabc::ModuleAsync::ExecuteCommand(cmd);

   return true; 
}

void bnet::GlobalDFCModule::ProcessInputEvent(dabc::Port* port)
{
   int node = -1;
   
   for (int n=0;n<fCfgNumNodes;n++) 
      if (Input(n)==port) { node = n; break; }
   if (node<0) {
      EOUT(("Internal error")); 
      return;   
   }
    
   bool isanynew = false; 
    
   while (!port->InputBlocked()) {
      
      dabc::Buffer* buf = 0;
      if (!port->Recv(buf)) {
         EOUT(("Internal error"));
         break;   
      }
      
      if (buf->GetTypeId() != mbt_EvInfo) {
         EOUT(("Wrong buffer type"));
         dabc::Buffer::Release(buf);
         break;   
      }
      
      EventInfoRec* info = (EventInfoRec*) buf->GetDataLocation();
      int number = buf->GetDataSize() / sizeof(EventInfoRec);

//      DOUT1(("Get packet from node %d number = %d", node, number));
      
      for (int n=0;n<number;n++) {
         ControllerMap::iterator it = fMap.find(info[n].evid);
         if (it == fMap.end()) {
            fMap.insert(ControllerPair(info[n].evid, ControllerRec(fCfgNumNodes)));
            it = fMap.find(info[n].evid);
         }
         
         if (it->second.sizes[node]<0) {
            it->second.sizes[node] = info[n].evsize;
            it->second.nsizes++;
         } else {
            EOUT(("Duplicate info over event %llu from node %d", info[n].evid, node)); 
         }
         
         if (it->second.nsizes == fSendNodes.size()) {
            isanynew = true; 
            fReadyEvnts.push_back(info[n].evid);
//            DOUT1(("Event ready %lld", info[n].evid));
         }
      }
      
      dabc::Buffer::Release(buf);
   }
   
   if (isanynew) TrySendEventsAssignment(false);
}

void bnet::GlobalDFCModule::ProcessOutputEvent(dabc::Port* port)
{
   TrySendEventsAssignment(false);
}

void bnet::GlobalDFCModule::ProcessTimerEvent(dabc::Timer* timer)
{
   dabc::TimeStamp_t tm = ThrdTimeStamp();
   if (dabc::IsNullTime(fLastSendTime)) fLastSendTime = tm;
   
//   DOUT1(("ProcessTimerEvent ready = %d", fReadyEvnts.size()));
   
   if (dabc::TimeDistance(fLastSendTime, tm) > 1.) TrySendEventsAssignment(true);

//   DOUT1(("ProcessTimerEvent done ready = %d", fReadyEvnts.size()));

}

void bnet::GlobalDFCModule::TrySendEventsAssignment(bool force)
{
   // first, check if none of outputs is blocked
   for (unsigned n=0;n<fSendNodes.size();n++) 
      if (Output(fSendNodes[n])->OutputBlocked()) return;
   
   // second, check if any new status is there
   unsigned maxcnt = fBufferSize / sizeof(EventAssignRec);
   unsigned limit = force ? 1 : 50;
   if (limit>maxcnt) limit = maxcnt;
   if (fReadyEvnts.size() < limit) return;
   
   // third, check if memory pool is not yet blocked
   dabc::Buffer* buf = fPool->TakeBuffer(fBufferSize, false);
   if (buf==0) return;
   
   EventAssignRec* evas = (EventAssignRec*) buf->GetDataLocation();
   
//   DOUT1(("Fill response size = %d maxcnt = %d", fReadyEvnts.size(), maxcnt));
   
   unsigned number = 0;
   
   // we ignore size information for the moment, just
   // simple round-robin as it was
   while ((fReadyEvnts.size() > 0) && (number<maxcnt)) {
      EventId evid = fReadyEvnts.front();   
      
//      DOUT1(("Event ready %lld", evid));
      
      fReadyEvnts.pop_front();
      
      ControllerMap::iterator it = fMap.find(evid);
      
      if (it != fMap.end())
         fMap.erase(it);
      else
         EOUT(("Wrong event id %lld", evid));   
      
      evas[number].evid = evid;
      evas[number].tgtnode = fRecvNodes[fTargetCounter++ % fRecvNodes.size()];
      
      number++;
   }
   
   buf->SetDataSize(number * sizeof(EventAssignRec));
   buf->SetTypeId(bnet::mbt_EvAssign);

   for (unsigned n=0; n<fSendNodes.size();n++) {
      dabc::Buffer* sendbuf = buf->MakeReference();
      if (sendbuf==0) {
         EOUT(("Not able to make new reference !!!!!"));
         exit(1);   
      } 

//      DOUT1(("Sending response to node %d size %d", n, sendbuf->GetTotalSize()));
      
      Output(fSendNodes[n])->Send(sendbuf);
   }
   
   dabc::Buffer::Release(buf);

   fLastSendTime = ThrdTimeStamp();
}
