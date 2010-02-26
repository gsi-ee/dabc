/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "bnet/SenderModule.h"

#include "dabc/PoolHandle.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"
#include "dabc/Application.h"

bnet::SenderModule::SenderModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleAsync(name, cmd),
   fPool(0),
   fMap(),
   fNewEvents(),
   fReadyEvents(),
   fStandalone(true),
   fRecvNodes(),

   fCtrlPool(0),
   fCtrlPort(0),
   fCtrlBufSize(0),
   fCtrlOutTime(0.)
{
   fCfgNumNodes = GetCfgInt(CfgNumNodes, 1, cmd);
   fCfgNodeId = GetCfgInt(CfgNodeId, 0, cmd);

   fPool = CreatePoolHandle(bnet::TransportPoolName);

   CreateInput("Input", fPool, SenderInQueueSize, sizeof(bnet::EventId));

   DOUT1(("Create Sender with %d outputs", fCfgNumNodes));

   for (int n=0;n<fCfgNumNodes;n++)
      CreateOutput(FORMAT(("Output%d", n)), fPool, SenderQueueSize, sizeof(bnet::SubEventNetHeader));

   fCtrlPool = CreatePoolHandle(bnet::ControlPoolName);
   fCtrlBufSize = GetCfgInt(xmlCtrlBuffer, 2*1024, cmd);
   fCtrlPort = CreateIOPort("CtrlPort", fCtrlPool, CtrlInpQueueSize, CtrlOutQueueSize);
   fCtrlOutTime = 0.;

   CreateRateParameter("DataRate", false, 1., "Input", "");
}

bnet::SenderModule::~SenderModule()
{
   while (fMap.size() > 0) {
      SenderMap::iterator it = fMap.begin();
      dabc::Buffer::Release(it->second.buf);
      it->second.buf = 0;
      fMap.erase(it);
   }
}

int bnet::SenderModule::ExecuteCommand(dabc::Command* cmd)
{
   if (cmd->IsName("Configure")) {

      DOUT3(("Get command Configure"));

      fStandalone = cmd->GetBool("Standalone");
      fRecvNodes.Reset(cmd->GetStr(parRecvMask), fCfgNumNodes);

      for (int node=0;node<fCfgNumNodes;node++)
        if (!fRecvNodes.HasNode(node)) {
           dabc::Port* port = Output(node);
           if (port->IsConnected()) {
              DOUT1(("@@@@@@@ Disconnecting receiver node %d", node));
              port->Disconnect();
           }
        }


      fCtrlOutTime = 0.;

      DOUT3(("Sender Configure done nout = %d standalone %s", NumOutputs(), DBOOL(fStandalone)));

   } else
   if (cmd->IsName("GetConfig")) {
      std::string str;
      for (int node=0;node<fCfgNumNodes;node++) {
         dabc::Port* port = Output(node);
         if ((port!=0) && port->IsConnected())
            str += "x";
         else
            str += "o";
      }
      cmd->SetStr(parRecvMask, str.c_str());
   } else
      return dabc::ModuleAsync::ExecuteCommand(cmd);

   return dabc::cmd_true;
}

void bnet::SenderModule::ReactOnDisconnect(dabc::Port* port)
{
   DOUT1(("Get disconnect event for port %s", port->GetName()));
   std::string NewRecvMask;
   for (int n=0;n<fCfgNumNodes;n++)
      NewRecvMask += Output(n)->IsConnected() ? "x" : "o";

   std::string oldmask = GetCfgStr(CfgRecvMask, "");

   if (NewRecvMask == oldmask) {
      DOUT1(("Recv Mask is not changed - still %s", NewRecvMask.c_str()));
      return;
   }

   DOUT1(("RecvMaskChangeDetected old:%s new:%s", oldmask.c_str(), NewRecvMask.c_str()));

   dabc::Application* app = dabc::mgr()->GetApp();
   if (app==0) return;

   dabc::Parameter* par = app->FindPar(parRecvStatus);
   if (par) par->InvokeChange(NewRecvMask.c_str());
       else EOUT(("Did not fount RecvStatus parameter"));
}

void bnet::SenderModule::StandaloneProcessEvent(dabc::ModuleItem* item, uint16_t item_evid)
{

   if ((item_evid == dabc::evntPortDisconnect)) {
      ReactOnDisconnect((dabc::Port*) item);
   }

   while(Input(0)->CanRecv()) {

      // only check if queue is not overflowed
      // if transport is disconnected, port will skip buffer which we send over it
      for (unsigned n=0;n<fRecvNodes.size();n++) {
         dabc::Port* port = Output(fRecvNodes[n]);

         if (!port->CanSend()) {
            DOUT5(("Output queue %d %d pend:%d blocked, leave", n, fRecvNodes[n], port->OutputPending()));
            return;
         }
      }

      dabc::Buffer* buf = Input(0)->Recv();

      if (buf==0) {
         EOUT(("Fail to receive data from Combiner module"));
         return;
      }

      if (buf->GetTypeId() == dabc::mbt_EOF) {
         DOUT1(("Find EOF in input"));
         dabc::Buffer::Release(buf);
         continue;
      }

      if (buf->GetTypeId() == dabc::mbt_EOL) {
         DOUT1(("Sender finds EOL in the input, broadcast to all receivers"));

         for (unsigned n=0;n<fRecvNodes.size();n++) {
            dabc::Port* port = Output(fRecvNodes[n]);

            if (!port->IsConnected()) continue;

            dabc::Buffer* outbuf = buf;
            buf = 0;
            if (outbuf==0) {
               outbuf = fPool->TakeEmptyBuffer();
               outbuf->SetTypeId(dabc::mbt_EOL);
            }
            port->Send(outbuf);
         }

         dabc::Buffer::Release(buf);

         DOUT1(("Stop module and return from the event processing"));

         Stop();

         dabc::mgr()->GetApp()->InvokeCheckModulesCmd();

         return;

      }

      bnet::EventId* header = (bnet::EventId*) buf->GetHeader();
      if (header==0) {
         EOUT(("Fail to get header from buffer"));
         exit(1);
      }

      bnet::EventId evid = *header;

      int tgtnode = 0;

      if (fRecvNodes.size()==0)
         EOUT(("Receive nodes mask is not specified"));
      else
        tgtnode = fRecvNodes[evid % fRecvNodes.size()];

      dabc::Port* out = Output(tgtnode);

      // if output port is not connected, skip buffer at all
      if (!out->IsConnected()) {
         dabc::Buffer::Release(buf);
         continue;
      }

//      DOUT1(("SenderModule Recv buf %p total = %u tgtnode = %d", buf, buf->GetTotalSize(), tgtnode));

      buf->SetHeaderSize(sizeof(bnet::SubEventNetHeader));
      bnet::SubEventNetHeader* hdr = (bnet::SubEventNetHeader*) buf->GetHeader();

      hdr->evid = evid;
      hdr->srcnode = fCfgNodeId;
      hdr->tgtnode = tgtnode;
      hdr->pktid = 0;

      DOUT5(("Send event %llu to node %d", evid, tgtnode));

      fSendRate.Packet(buf->GetTotalSize());

      out->Send(buf);
   }
}

void bnet::SenderModule::ProcessUserEvent(dabc::ModuleItem* item, uint16_t evid)
{
   if (fStandalone)
      StandaloneProcessEvent(item, evid);
   else
      dabc::ModuleAsync::ProcessUserEvent(item, evid);
}


void bnet::SenderModule::ProcessInputEvent(dabc::Port* port)
{
   dabc::Buffer* buf = 0;

//   DOUT1(("ProcessInputEvent %s port %s", GetName(), port->GetName()));

   if (!port->CanRecv()) return;

   if (port == fCtrlPort) {

      buf = port->Recv();

      if (buf==0) return;

     if (buf->GetTypeId() != bnet::mbt_EvAssign) {
        EOUT(("Wrong buffer id %d", buf->GetTypeId()));
        dabc::Buffer::Release(buf);
        return;
     }

     EventAssignRec* evs = (EventAssignRec*) buf->GetDataLocation();
     int number = buf->GetDataSize() / sizeof(EventAssignRec);

//     DOUT1(("Get assignment for %d events", number));

     for (int n=0;n<number;n++) {

//        DOUT1(("Event %d", evs[n].evid));

        SenderRec* rec = &(fMap.find(evs[n].evid)->second);
        rec->tgtnode = evs[n].tgtnode;
        fReadyEvents.push_back(evs[n].evid);
     }

     dabc::Buffer::Release(buf);

     CheckReadyEvents();

   } else {

      // this is a way to prevent overflow of transport pool
      // just block input if too many data is collected
      if (fPool->UsedRatio() > 0.8) return;

      buf = port->Recv();

      if (buf==0) return;

      bnet::EventId* header = (bnet::EventId*) buf->GetHeader();
      if (header==0) {
         EOUT(("Fail to get header from buffer"));
         exit(1);
      }

      bnet::EventId eventid = *header;

      fMap.insert(SenderPair(eventid, SenderRec(buf)));

      fNewEvents.push_back(eventid);

      CheckNewEvents();
   }
}

void bnet::SenderModule::ProcessOutputEvent(dabc::Port* port)
{
//   DOUT1(("ProcessOutputEvent %s port %s", GetName(), port->GetName()));

   if (port==fCtrlPort) CheckNewEvents();
                   else CheckReadyEvents();

   ProcessInputEvent(Input(0));
}

void bnet::SenderModule::CheckNewEvents()
{
   if (!fCtrlPort->CanSend() || (fNewEvents.size()==0)) return;

   if (fNewEvents.size() * sizeof(EventInfoRec) < fCtrlBufSize) {
      double tm = ThrdTimeStamp();
      if (fCtrlOutTime == 0.) fCtrlOutTime = tm;
      if (tm-fCtrlOutTime < 1e5) return;
   }

//   DOUT1(("Take ctrl buf %d %p %p",fCtrlBufSize, fCtrlPool, fCtrlPool->getPool()));

   dabc::Buffer* outbuf = fCtrlPool->TakeBuffer(fCtrlBufSize);
   if (outbuf==0) {
      EOUT(("Cannot get control buffer."));
      return;
   }

//   DOUT1(("Res ctrl buf %p %d", outbuf, outbuf->GetTotalSize()));


   EventInfoRec* info = (EventInfoRec*) outbuf->GetDataLocation();

   unsigned number = 0;

   while ((number*sizeof(EventInfoRec) < fCtrlBufSize) &&
          (fNewEvents.size() > 0)) {

      EventId evid = fNewEvents.front();
      fNewEvents.pop_front();

      SenderRec* rec = &(fMap.find(evid)->second);

      info[number].evid = evid;
      info[number].evsize = rec->buf->GetTotalSize();

      number++;
   }

   outbuf->SetDataSize(number*sizeof(EventInfoRec));
   outbuf->SetTypeId(mbt_EvInfo);

   fCtrlPort->Send(outbuf);
}

void bnet::SenderModule::CheckReadyEvents()
{
   while (fReadyEvents.size()>0) {

      EventId evid = fReadyEvents.front();

      SenderMap::iterator it = fMap.find(evid);

      int tgtnode = it->second.tgtnode;

      if (!Output(tgtnode)->CanSend()) return;

      dabc::Buffer* buf = it->second.buf;
      it->second.buf = 0;
      fMap.erase(it);
      fReadyEvents.pop_front();

      buf->SetHeaderSize(sizeof(bnet::SubEventNetHeader));
      bnet::SubEventNetHeader* hdr = (bnet::SubEventNetHeader*) buf->GetHeader();

      hdr->evid = evid;
      hdr->srcnode = fCfgNodeId;
      hdr->tgtnode = tgtnode;
      hdr->pktid = 0;

      Output(tgtnode)->Send(buf);
   }
}


void bnet::SenderModule::BeforeModuleStart()
{
   fSendRate.Reset();

   DOUT1(("fRecvNodes.size() = %d", fRecvNodes.size()));
}

void bnet::SenderModule::AfterModuleStop()
{
   DOUT1(("SenderModule Rate %5.1f", fSendRate.GetRate()));
}
