#include "bnet/ReceiverModule.h"

#include "dabc/PoolHandle.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"
#include "dabc/Parameter.h"
#include "dabc/Application.h"

bnet::ReceiverModule::ReceiverModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleAsync(name, cmd),
   fPool(0),
   fRecvRate(),
   fSendNodes(),
   fInpCounter(0),
   fEventsPool(),
   fPoolTail(0),
   fPoolHead(0),
   fPoolSize(0),
   fEventsMap(),
   fLastEvents(),
   fPoolTailSending(-1)
{
   fCfgNumNodes = GetCfgInt(CfgNumNodes, 1, cmd);
   fCfgNodeId = GetCfgInt(CfgNodeId, 0, cmd);

   fPool = CreatePoolHandle(bnet::TransportPoolName);

   // output is always there
   CreateOutput("Output", fPool, ReceiverOutQueueSize, sizeof(bnet::SubEventNetHeader));

   DOUT1(("Create Receiver with %d inputs", fCfgNumNodes));

   for (int n=0;n<fCfgNumNodes;n++)
      CreateInput(FORMAT(("Input%d", n)), fPool, ReceiverQueueSize, sizeof(bnet::SubEventNetHeader));

   CreateRateParameter("DataRate", false, 1., "", "Output");

   const unsigned EventsPoolSize = (ReceiverQueueSize + ReceiverOutQueueSize);

   for (unsigned n=0;n<EventsPoolSize;n++) {
      RecvEventData* ev = new RecvEventData(fCfgNumNodes);
      fEventsPool.push_back(ev);
   }

   for (int n=0;n<fCfgNumNodes;n++)
      fLastEvents.push_back(0);

   fSendingCounter = fCfgNumNodes;
   for (int n=0;n<fCfgNumNodes;n++)
      fCurrEvent.push_back(0);
   fStopSending = false;
}

bnet::ReceiverModule::~ReceiverModule()
{
   fEventsMap.clear();

   //!!!!!!!!! release used buffers

   for (unsigned n=0;n<fEventsPool.size();n++)
      delete fEventsPool[n];
   fEventsPool.clear();

   fPoolTail = 0;
   fPoolHead = 0;
   fPoolSize = 0;
   fPoolTailSending = -1;

   fLastEvents.clear();
}

int bnet::ReceiverModule::ExecuteCommand(dabc::Command* cmd)
{
   if (cmd->IsName("Configure")) {

      DOUT3(("Get command Configure"));

      fSendNodes.Reset(cmd->GetStr(parSendMask), fCfgNumNodes);

      fInpCounter = fSendNodes.size();

      for (int node=0;node<fCfgNumNodes;node++)
        if (!fSendNodes.HasNode(node)) {
           dabc::Port* port = Input(node);
           if (port->IsConnected()) {
              DOUT1(("@@@@@@@ Disconnecting sender node %d", node));
//              port->Disconnect();
           }
        }

      DOUT3(("Receiever Configure done ninp = %d", NumInputs()));
   } else
   if (cmd->IsName("GetConfig")) {
      std::string str = "";
      for (int node=0;node<fCfgNumNodes;node++) {
         dabc::Port* port = Input(node);
         if ((port!=0) && port->IsConnected())
            str += "x";
         else
            str += "o";
      }

      cmd->SetStr(parSendMask, str.c_str());
   } else

      return dabc::ModuleAsync::ExecuteCommand(cmd);

   return cmd_true;
}

bnet::ReceiverModule::RecvEventData* bnet::ReceiverModule::ProvideEventData(bnet::EventId evid, bool force)
{
   if (fPoolSize==0) {
      if (!force) return 0;

      fPoolSize = 1;
      fPoolTail = 0;
      fPoolHead = 0;

      RecvEventData* evnt = fEventsPool[fPoolHead];
      evnt->evid = evid;

      fEventsMap[evid] = fPoolHead;

      return evnt;
   }

   if (evid<fEventsPool[fPoolTail]->evid) {
      EOUT(("Access to event which should not longer exists"));
      return 0;
   }

   if (evid > fEventsPool[fPoolHead]->evid) {
      if (!force) return 0;

      if (fPoolSize==fEventsPool.size()) {
         EOUT(("No more space for events"));
         return 0;
      }

      fPoolSize++;
      fPoolHead = (fPoolHead+1) % fEventsPool.size();

      RecvEventData* evnt = fEventsPool[fPoolHead];
      evnt->evid = evid;

      fEventsMap[evid] = fPoolHead;

      return evnt;
   }

   if (evid==fEventsPool[fPoolTail]->evid) return fEventsPool[fPoolTail];

   if (evid==fEventsPool[fPoolHead]->evid) return fEventsPool[fPoolHead];

   EventsMap::iterator iter = fEventsMap.find(evid);

   if (iter==fEventsMap.end()) {
      if (force) EOUT(("Cannot produce new entry in the middle of event range"));
      return 0;
   }

   RecvEventData* evnt = fEventsPool[iter->second];
   if (evnt->evid != evid) {
      EOUT(("Wrong events map"));
      return 0;
   }
   return evnt;
}


bool bnet::ReceiverModule::DoInputRead(int nodeid)
{
   if (Input(nodeid)->InputBlocked()) return false;

   dabc::Buffer* buf = 0;

   Input(nodeid)->Recv(buf);

   if (buf==0) return false;

   fRecvRate.Packet(buf->GetTotalSize());

   if (buf->GetHeaderSize() < sizeof(SubEventNetHeader)) {
      EOUT(("Wrong header in net packet"));
      dabc::Buffer::Release(buf);
      return false;
   }

   if (buf->GetTypeId() == dabc::mbt_EOF) {
      DOUT1(("Get EOF from input %d", nodeid));
      dabc::Buffer::Release(buf);
      return true;
   }

   SubEventNetHeader* hdr = (SubEventNetHeader*) buf->GetHeader();

   if (fLastEvents[nodeid]!=0)
     if (fLastEvents[nodeid] >= hdr->evid) {
         EOUT(("Event sequence over input %d is broken lastid:%llu now:%llu", nodeid, fLastEvents[nodeid], hdr->evid));
         dabc::Buffer::Release(buf);
         return false;
     }

   RecvEventData* evnt = ProvideEventData(hdr->evid, true);
   if (evnt==0) {
      EOUT(("Cannot process event data"));
      dabc::Buffer::Release(buf);
      return false;
   }

   if (evnt->bufs[nodeid] != 0) {
      EOUT(("Buffer for this event from this input already exists !!!"));
      dabc::Buffer::Release(buf);
      return false;
   }

   DOUT3(("Get event %llu from node %d", hdr->evid, nodeid));

   evnt->bufs[nodeid] = buf;
   fLastEvents[nodeid] = hdr->evid;

   return true;
}


bnet::EventId bnet::ReceiverModule::DefineMinimumLastEvent()
{
   EventId minevid = 0;

   // first, check that event id is minimum
   for (unsigned n=0;n<fSendNodes.size(); n++) {
      int nodeid = fSendNodes[n];

      if (fLastEvents[nodeid]==0) return fLastEvents[nodeid];

      if ((minevid==0) || (fLastEvents[nodeid]<minevid))
         minevid = fLastEvents[nodeid];
   }

   return minevid;
}

void bnet::ReceiverModule::ProcessEventNew(dabc::ModuleItem*, uint16_t)
{

   bool doreadsomething = false;

   bool poolblocked = false;

   do {

      doreadsomething = false;

      // !!!!!!! probably, check if not too many data read in once

      poolblocked = (fPoolSize >= fEventsPool.size() * 0.9) || (fPool->UsedRatio() > 0.8);
      if (poolblocked) break;

      EventId minevid = DefineMinimumLastEvent();

      // now check only those nodes, which has minimum last id
      for (unsigned n=0;n<fSendNodes.size(); n++) {
         int nodeid = fSendNodes[n];
         if (fLastEvents[nodeid]==minevid)
            if (DoInputRead(nodeid))
               doreadsomething = true;
      }

   } while (doreadsomething);


   do {

      while (fPoolTailSending>=0) {
         if (fPoolSize==0) { fPoolTailSending = -1; return; }

         RecvEventData* evnt = fEventsPool[fPoolTail];

         while ((fPoolTailSending < fCfgNumNodes) && (evnt->bufs[fPoolTailSending] == 0)) fPoolTailSending++;

         if (fPoolTailSending>=fCfgNumNodes) {
            fPoolTailSending = -1;
            fPoolTail = (fPoolTail+1) % fEventsPool.size();
            fPoolSize--;
            DOUT3(("Erase event %llu", evnt->evid));
            break;
         }

         // if cannot send, data, just exit
         if (Output(0)->OutputBlocked()) return;

         dabc::Buffer* buf = evnt->bufs[fPoolTailSending];
         evnt->bufs[fPoolTailSending] = 0;
         Output(0)->Send(buf);

         fPoolTailSending++;
      }

      // now check all events in pool that are below defined limit
      EventId minevid = DefineMinimumLastEvent();
      // one need at least one packet from each input for starting analysis
      if (minevid == 0) {
         if (poolblocked) EOUT(("Pool blocking already, but has no data to process !!!"));
         return;
      }

      while (fPoolSize>0) {
         RecvEventData* evnt = fEventsPool[fPoolTail];

         bool iscomplete = true;

         for (unsigned n=0;n<fSendNodes.size(); n++) {
            int nodeid = fSendNodes[n];
            if (evnt->bufs[nodeid] == 0) iscomplete = false;
         }

         // if last event in the pool is complete, just starts its sending
         if (iscomplete) {
            fPoolTailSending = 0;
            DOUT3(("Complete event %llu, start pushing (%d)", evnt->evid, fPoolSize));
            break;
         }

         // this is the case when we accumulating event and did not get it complete yet
         if (evnt->evid > minevid) {
//            if (poolblocked) EOUT(("Pool blocking already, but has no enough data to process !!!"));
            return;
         }

         // here on all inputs there were packets with bigger event id number,
         // thus we have no longer possibility to get packet for incomplete event

         for(unsigned n=0;n<evnt->bufs.size();n++)
            if (evnt->bufs[n]!=0) {
               dabc::Buffer::Release(evnt->bufs[n]);
               evnt->bufs[n] = 0;
            }
         fPoolTail = (fPoolTail+1) % fEventsPool.size();
         fPoolSize--;

         EOUT(("Exclude incomplete event %llu", evnt->evid));
      }

   } while (fPoolTailSending>=0);
}

void bnet::ReceiverModule::ProcessUserEvent(dabc::ModuleItem*, uint16_t)
{
   while ((fSendingCounter >= fSendNodes.size()) && !fStopSending) {

      bool iscompleteset = true;

      unsigned numeol = 0;

      for (unsigned n=0; n < fSendNodes.size(); n++) {
         int nodeid = fSendNodes[n];

         if (fCurrEvent[nodeid]>0) continue;

         dabc::Buffer* buf = Input(nodeid)->InputBuffer(0);

         if (buf==0) {
            iscompleteset = false;
            continue;
         }

         if (buf->GetTypeId() == dabc::mbt_EOL) {
            numeol++;
            DOUT5(("Receiver found EOL buffer from node %d n:%u numeol:%u", nodeid, n, numeol));
            continue;
         }

         bool skipbuffer = false;

         if (buf->GetTypeId()==dabc::mbt_EOF) {
            DOUT1(("Find EOF packet in input %d", nodeid));
            skipbuffer = true;
         } else
         if (buf->GetHeaderSize() < sizeof(SubEventNetHeader)) {
            EOUT(("Wrong header in net packet"));
            skipbuffer = true;
         }

         if (skipbuffer) {
            Input(nodeid)->SkipInputBuffers(1);
            iscompleteset = false;
            continue;
         }

         SubEventNetHeader* hdr = (SubEventNetHeader*) buf->GetHeader();

         fCurrEvent[nodeid] = hdr->evid;

         DOUT5(("Get event %llu from node %d", hdr->evid, nodeid));
      }

      // we need either EOL or normal event at each input
      if (!iscompleteset) return;

      if (numeol>0) {
         if (numeol<fSendNodes.size())
           EOUT(("Number of EOL %u less than number of inputs %u", numeol, fSendNodes.size()));

         fStopSending = true;
         break;
      }

      int minid = fSendNodes[0];
      int maxid = fSendNodes[0];

      for (unsigned n=1; n < fSendNodes.size(); n++) {
         int nodeid = fSendNodes[n];

         if (fCurrEvent[nodeid] < fCurrEvent[minid]) minid = nodeid; else
         if (fCurrEvent[nodeid] > fCurrEvent[maxid]) maxid = nodeid;
      }

      if (fCurrEvent[minid] != fCurrEvent[maxid]) {
         EOUT(("Skip buffer %p from input %d with event %llu (max = %llu) pending %d", Input(minid)->InputBuffer(0), minid, fCurrEvent[minid], fCurrEvent[maxid], Input(minid)->InputPending()));

         Input(minid)->SkipInputBuffers(1);
         fCurrEvent[minid] = 0;

         continue;
      }

      // this will starts sending data to the output
      fSendingCounter = 0;

      DOUT5(("Start retranslating of event %llu", fCurrEvent[minid]));
   }

   if (fStopSending) {
      if (Output(0)->OutputBlocked()) return;

      DOUT1(("First, skip all EOL from all inputs"));

      for (unsigned n=0; n < fSendNodes.size(); n++) {
         int nodeid = fSendNodes[n];
         if (fCurrEvent[nodeid]==0)
            Input(nodeid)->SkipInputBuffers(1);
      }

      DOUT1(("Send EOL and stop Receiever module"));

      dabc::Buffer* outbuf = fPool->TakeEmptyBuffer();
      outbuf->SetTypeId(dabc::mbt_EOL);
      Output(0)->Send(outbuf);

      fStopSending = false;

      Stop();

      dabc::mgr()->GetApp()->InvokeCheckModulesCmd();

//      dabc::mgr()->Submit(dabc::mgr()->LocalCmd(new dabc::Command("CheckModulesStatus"), dabc::xmlAppDfltName));

      return;
   }

   // this is simple loop to retranslate events from inputs to output
   while (!Output(0)->OutputBlocked() && (fSendingCounter<fSendNodes.size())) {
      int nodeid = fSendNodes[fSendingCounter++];
      dabc::Buffer* buf = 0;

      if (!Input(nodeid)->IsConnected()) {
         EOUT(("Input %d not connected, why come here ?", nodeid));
      }

      Input(nodeid)->Recv(buf);

//      DOUT1(("Retranslate buf %p evid %llu from node %d", buf, fCurrEvent[nodeid], nodeid));

      if (buf==0) {
         EOUT(("No buffer when expected"));
      } else {
         fRecvRate.Packet(buf->GetTotalSize());
         Output(0)->Send(buf);
      }

      fCurrEvent[nodeid] = 0;
   }
}


void bnet::ReceiverModule::ProcessEvent2(dabc::ModuleItem*, uint16_t)
{
   if (fInpCounter == fSendNodes.size()) {
      bool blocked = false;

      for (unsigned n=0; n < fSendNodes.size(); n++)
        if (Input(fSendNodes[n])->InputBlocked()) blocked = true;

      if (blocked) return;

      fInpCounter = 0;
   }

   while ((fInpCounter < fSendNodes.size()) && !Output(0)->OutputBlocked()) {
      dabc::Buffer* buf = 0;

//      DOUT1(("Recv buffer from input %d %p", fInpCounter, Input(fInpCounter)));

      Input(fSendNodes[fInpCounter])->Recv(buf);

      fRecvRate.Packet(buf->GetTotalSize());

      Output(0)->Send(buf);

      fInpCounter++;
   }
}

void bnet::ReceiverModule::BeforeModuleStart()
{
   fRecvRate.Reset();

   DOUT5(("bnet::ReceiverModule::BeforeModuleStart"));

}

void bnet::ReceiverModule::AfterModuleStop()
{
   DOUT1(("ReceiverModule Rate %5.1f Network %5.1f",
      fRecvRate.GetRate(), fRecvRate.GetRate() / fSendNodes.size() * (fSendNodes.size()-1)));
}
