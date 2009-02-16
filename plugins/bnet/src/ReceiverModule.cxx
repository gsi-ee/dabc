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
   fInpCounter(0)
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

   fSendingCounter = fCfgNumNodes;
   for (int n=0;n<fCfgNumNodes;n++)
      fCurrEvent.push_back(0);
   fStopSending = false;
}

bnet::ReceiverModule::~ReceiverModule()
{
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

         DOUT5(("Get event %llu from node %d bufsize %u", hdr->evid, nodeid, buf->GetDataSize()));
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
         EOUT(("Skip from input %d event %llu (max = %llu) pending %d", minid, fCurrEvent[minid], fCurrEvent[maxid], Input(minid)->InputPending()));

         Input(minid)->SkipInputBuffers(1);
         fCurrEvent[minid] = 0;

         continue;
      }

      // this will starts sending data to the output
      fSendingCounter = 0;

      DOUT5(("Start retranslating of event %llu", fCurrEvent[minid]));
   }

   if (fStopSending) {
      if (!Output(0)->CanSend()) return;

      DOUT1(("First, skip all EOL from all inputs"));

      for (unsigned n=0; n < fSendNodes.size(); n++) {
         int nodeid = fSendNodes[n];
         if (fCurrEvent[nodeid]==0)
            Input(nodeid)->SkipInputBuffers(1);
      }

      DOUT1(("Send EOL and stop Receiver module"));

      dabc::Buffer* outbuf = fPool->TakeEmptyBuffer();
      outbuf->SetTypeId(dabc::mbt_EOL);
      Output(0)->Send(outbuf);

      fStopSending = false;

      Stop();

      dabc::mgr()->GetApp()->InvokeCheckModulesCmd();

      return;
   }

   // this is simple loop to retranslate events from inputs to output
   while (Output(0)->CanSend() && (fSendingCounter<fSendNodes.size())) {
      int nodeid = fSendNodes[fSendingCounter++];

      if (!Input(nodeid)->IsConnected())
         EOUT(("Input %d not connected, why come here ?", nodeid));

      dabc::Buffer* buf = Input(nodeid)->Recv();

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
