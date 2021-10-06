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

#include "mbs/ServerTransport.h"

#include <unistd.h>

#include "dabc/Manager.h"
#include "dabc/DataTransport.h"

mbs::ServerOutputAddon::ServerOutputAddon(int fd, int kind, dabc::EventsIteratorRef& iter, uint32_t subid) :
   dabc::SocketIOAddon(fd, false, true),
   dabc::DataOutput(dabc::Url()),
   fState(oInit),
   fKind(kind),
   fSendBuffers(0),
   fDroppedBuffers(0),
   fIter(iter),
   fEvHdr(),
   fSubHdr(),
   fEvCounter(0),
   fSubevId(subid),
   fHasExtraRequest(false)
{
   DOUT3("Create MBS server addon fd:%d kind:%s", fd, mbs::ServerKindToStr(kind));
}

mbs::ServerOutputAddon::~ServerOutputAddon()
{
   DOUT3("Destroy ServerOutputAddon %p", this);
}

void mbs::ServerOutputAddon::FillServInfo(int32_t maxbytes, bool isnewformat)
{
   memset(&fServInfo, 0, sizeof(fServInfo));

   fServInfo.iEndian = 1;              // byte order. Set to 1 by sender
   fServInfo.iMaxBytes = maxbytes;     // maximum buffer size
   fServInfo.iBuffers = 1;             // buffers per stream
   fServInfo.iStreams = isnewformat ? 0 : 1;     // number of streams (could be set to -1 to indicate variable length buffers, size l_free[1])
}

void mbs::ServerOutputAddon::OnThreadAssigned()
{
   dabc::SocketIOAddon::OnThreadAssigned();

   if (fState == oInit)
      StartSend(&fServInfo, sizeof(fServInfo));

   memset(f_sbuf, 0, sizeof(f_sbuf));
   StartRecv(f_sbuf, 12);

//   ActivateTimeout(1.);
}

double mbs::ServerOutputAddon::ProcessTimeout(double last_diff)
{
//   DOUT0("mbs::ServerOutputAddon::ProcessTimeout inp:%s out:%s", DBOOL(IsDoingInput()), DBOOL(IsDoingOutput()));
   return 1.;
}


void mbs::ServerOutputAddon::OnSendCompleted()
{
//   DOUT0("mbs::ServerOutputAddon::OnSendCompleted  inp:%s out:%s", DBOOL(IsDoingInput()), DBOOL(IsDoingOutput()));

   switch (fState) {
      case oInit:
         DOUT4("Send info completed");
         if (fKind == mbs::StreamServer)
            fState = oWaitingReq;
         else
            fState = oWaitingBuffer;
         return;

      case oInitReq:
         fState = oWaitingBuffer;
         return;

      case oSendingEvents: {
         dabc::EventsIterator* iter = fIter();

         unsigned evsize = iter->EventSize();
         if (evsize % 2) evsize++;

         fSubHdr.InitFull(fSubevId);
         fSubHdr.SetRawDataSize(evsize);

         fEvHdr.Init(fEvCounter++);
         fEvHdr.SetFullSize(evsize + sizeof(fEvHdr) + sizeof(fSubHdr));

         StartSend(&fEvHdr, sizeof(fEvHdr), &fSubHdr, sizeof(fSubHdr), iter->Event(), evsize);

         if (!iter->NextEvent())
            // if there are no move event - close iterator and wait for completion
            fState = oSendingLastEvent;

         return;
      }

      case oSendingLastEvent:
         fIter()->Close(); // close here to ensure memory for send operation
         /* no break */

      case oSendingBuffer:
         if (fKind == mbs::StreamServer)
            fState = fHasExtraRequest ? oWaitingBuffer : oWaitingReq;
         else
            fState = oWaitingBuffer;
         fHasExtraRequest = false;
         MakeCallback(dabc::do_Ok);
         return;

      case oDoingClose:
      case oError:
         // ignore all input events in such case
         return;

      default:
         EOUT("Send complete at wrong state %d", fState);
         break;
   }
}

void mbs::ServerOutputAddon::OnRecvCompleted()
{
//   DOUT0("mbs::ServerOutputAddon::OnRecvCompleted %s  inp:%s out:%s", f_sbuf, DBOOL(IsDoingInput()), DBOOL(IsDoingOutput()));

   if (strcmp(f_sbuf, "CLOSE")==0) {
      OnSocketError(0, "get CLOSE event"); // do same as connection was closed
      return;
   }

   if (strcmp(f_sbuf, "GETEVT")!=0)
     EOUT("Wrong request string %s", f_sbuf);

   memset(f_sbuf, 0, sizeof(f_sbuf));
   StartRecv(f_sbuf, 12);
   fHasExtraRequest = false;

   switch (fState) {
      case oInit:
         // get request before send of server info was completed
         EOUT("Get data request before send server info was completed");
         fState = oInitReq;
         break;
      case oWaitingBuffer:
         // second request - ignore it
         break;
      case oWaitingReq:
         // normal situation
         fState = oWaitingBuffer;
         break;
      case oWaitingReqBack:
         MakeCallback(dabc::do_Ok);
         fState = oWaitingBuffer;
         break;
      case oSendingEvents:
      case oSendingLastEvent:
      case oSendingBuffer:
         fHasExtraRequest = true;
         break;
      default:
         EOUT("Get request at wrong state %d", fState);
         fState = oError;
         SubmitWorkerCmd(dabc::Command("CloseTransport"));
   }
}

void mbs::ServerOutputAddon::MakeCallback(unsigned arg)
{
   dabc::OutputTransport* tr = dynamic_cast<dabc::OutputTransport*> (fWorker());

   if (tr==0) {
      EOUT("Didnot found OutputTransport on other side worker %p", fWorker());
      fState = oError;
      SubmitWorkerCmd(dabc::Command("CloseTransport"));
   } else {
      // DOUT0("Activate CallBack with arg %u", arg);
      tr->Write_CallBack(arg);
   }
}


unsigned mbs::ServerOutputAddon::Write_Check()
{
//   DOUT0("mbs::ServerOutputAddon::Write_Check at state %d", fState);

   switch (fState) {
      // when initialization was not completed, try after timeout
      case oInit:
      case oInitReq:
         return dabc::do_RepeatTimeOut;

      // when stream server waits request, we need to inform transport as soon as possible
      case oWaitingReq:
         fState = oWaitingReqBack;
         return dabc::do_CallBack;

      case oWaitingBuffer:
         return dabc::do_Ok;

      case oError:
         return dabc::do_Error;

      default:
         EOUT("Write_Check at wrong state %d", fState);
         fState = oError;
         return dabc::do_Error;
   }

   return dabc::do_RepeatTimeOut;
}

unsigned mbs::ServerOutputAddon::Write_Buffer(dabc::Buffer& buf)
{
   if (fState != oWaitingBuffer) return dabc::do_Error;

//   DOUT0("mbs::ServerOutputAddon::Write_Buffer %u at state %d", buf.GetTotalSize(), fState);

   unsigned sendsize = buf.GetTotalSize();
   unsigned events = 0;

   if (sendsize == 0) return dabc::do_Skip;

   if (!fIter.null()) {
      dabc::EventsIterator *iter = fIter();
      sendsize = 0;
      iter->Assign(buf);
      unsigned rawsize = 0;

      while (iter->NextEvent()) {
         sendsize += sizeof(mbs::EventHeader) + sizeof(mbs::SubeventHeader);
         unsigned evsize = iter->EventSize();
         if (evsize % 2) evsize++;
         sendsize += evsize;
         rawsize += evsize;
         events++;
      }
      iter->Close();

      if (rawsize == 0) return dabc::do_Skip;

      iter->Assign(buf);
      iter->NextEvent(); // shift to first event
   }


   fHeader.Init(true);
   fHeader.SetUsedBufferSize(sendsize);
   fHeader.SetNumEvents(events);

   // error in evapi, must be + sizeof(mbs::BufferHeader)
   fHeader.SetFullSize(sendsize - sizeof(mbs::BufferHeader));

   if (fIter.null()) {
      fState = oSendingBuffer;
      StartNetSend(&fHeader, sizeof(fHeader), buf);
   } else {
      fState = oSendingEvents;
      StartSend(&fHeader, sizeof(fHeader));
   }

   return dabc::do_CallBack;
}


void mbs::ServerOutputAddon::OnSocketError(int err, const std::string &info)
{
   switch (fState) {
      case oSendingEvents:  // only at this states callback is required to inform transport that data should be closed
      case oSendingBuffer:
      case oWaitingReqBack:
         fState = (err==0) ? oDoingClose : oError;
         MakeCallback(dabc::do_Close);
         return;

      case oDoingClose: return;
      case oError: return;

      default:
         fState = (err==0) ? oDoingClose : oError;
         SubmitWorkerCmd(dabc::Command("CloseTransport"));
   }
}

// ===============================================================================

mbs::ServerTransport::ServerTransport(dabc::Command cmd, const dabc::PortRef& outport, int kind, int portnum, dabc::SocketServerAddon* connaddon, const dabc::Url& url) :
   dabc::Transport(cmd, 0, outport),
   fKind(kind),
   fPortNum(portnum),
   fSlaveQueueLength(5),
   fClientsLimit(0),
   fDoingClose(0),
   fBlocking(false),
   fDeliverAll(false),
   fIterKind(),
   fSubevId(0x1f)
{
   // this addon handles connection
   AssignAddon(connaddon);

   // TODO: queue length one can take from configuration
   fSlaveQueueLength = 5;

   if (url.HasOption("limit"))
      fClientsLimit = url.GetOptionInt("limit", fClientsLimit);

   if (url.HasOption("iter"))
      fIterKind = url.GetOptionStr("iter");

   if (url.HasOption("subid"))
      fSubevId = (unsigned) url.GetOptionInt("subid", fSubevId);

   // by default transport server is blocking and stream is unblocking
   // blocking has two meaning:
   // - when no connections are there, either block input or not
   // - when at least one connection established, block input until all output are ready
   fBlocking = (fKind == mbs::TransportServer);

   fDeliverAll = (fKind == mbs::TransportServer);

   if (url.HasOption("nonblock")) fBlocking = false; else
   if (url.HasOption("blocking")) fBlocking = true;

   if (url.HasOption("deliverall")) fDeliverAll = true;

   DOUT0("Create MBS server fd:%d kind:%s port:%d limit:%d blocking:%s deliverall:%s",
         connaddon->Socket(), mbs::ServerKindToStr(fKind), fPortNum, fClientsLimit, DBOOL(fBlocking), DBOOL(fDeliverAll));

   if (fClientsLimit>0) DOUT0("Set client limit for MBS server to %d", fClientsLimit);

//   DOUT0("mbs::ServerTransport   isinp=%s", DBOOL(connaddon->IsDoingInput()));
}

mbs::ServerTransport::~ServerTransport()
{
}

bool mbs::ServerTransport::StartTransport()
{
   return dabc::Transport::StartTransport();
}

bool mbs::ServerTransport::StopTransport()
{
   return dabc::Transport::StopTransport();
}

int mbs::ServerTransport::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("SocketConnect")) {

      int fd = cmd.GetInt("fd", -1);

      if (fd<=0) return dabc::cmd_false;

      int numconn(0);
      int portindx = -1;
      for (unsigned n=0;n<NumOutputs();n++)
         if (IsOutputConnected(n)) {
            numconn++;
         } else {
            if (portindx<0) portindx = n;
         }

      if ((fClientsLimit>0) && (numconn>=fClientsLimit)) {
         DOUT0("Reject connection %d, maximum number %d is achieved ", fd, numconn);
         close(fd);
         return dabc::cmd_true;
      }

      DOUT3("Get new connection request with fd %d canrecv %s", fd, DBOOL(CanRecv()));

      dabc::EventsIteratorRef iter;

      if (!fIterKind.empty()) {
         iter = dabc::mgr.CreateObject(fIterKind,"iter");
         if (iter.null()) {
            EOUT("Fail to create events iterator %s", fIterKind.c_str());
            close(fd);
            return dabc::cmd_true;
         }
      }

      dabc::SocketThread::SetNoDelaySocket(fd);

      ServerOutputAddon *addon = new ServerOutputAddon(fd, fKind, iter, fSubevId);
      // FIXME: should we configure buffer size or could one ignore it???
      addon->FillServInfo(0x400000, true);


      if (portindx<0) portindx = CreateOutput(dabc::format("Slave%u",NumOutputs()), fSlaveQueueLength);

      dabc::TransportRef tr = new dabc::OutputTransport(dabc::Command(), FindPort(OutputName(portindx)), addon, true);

      tr()->AssignToThread(thread(), true);

      // transport will be started automatically

      dabc::LocalTransport::ConnectPorts(FindPort(OutputName(portindx)), tr.InputPort(), cmd);

      DOUT3("mbs::ServerTransport create new connection at running=%s", DBOOL(isTransportRunning()));

      return dabc::cmd_true;
   }

   if (cmd.IsName("GetTransportStatistic")) {

      int cnt = 0;
      std::vector<uint64_t> cansend;
      for(unsigned n=0;n<NumOutputs();n++) {
         if (IsOutputConnected(n)) {
            cnt++; cansend.push_back(NumCanSend(n));
         }
      }

      cmd.SetInt("NumClients", cnt);
      cmd.SetInt("NumCanRecv", NumCanRecv());

      if (cnt==1) cmd.SetField("NumCanSend", cansend[0]); else
      if (cnt>1) cmd.SetField("NumCanSend", cansend); else
      cmd.SetField("NumCanSend", 0);

      cmd.SetStr("MbsKind", mbs::ServerKindToStr(fKind));
      cmd.SetInt("MbsPort", fPortNum);
      cmd.SetStr("MbsInfo", dabc::format("%s:%d NumClients:%d", mbs::ServerKindToStr(fKind), fPortNum, cnt));

      return dabc::cmd_true;
   }

   return dabc::Transport::ExecuteCommand(cmd);
}

bool mbs::ServerTransport::SendNextBuffer()
{
   if (!CanRecv()) return false;

   // unconnected transport server will block until any connection is established
   if ((NumOutputs()==0) && fBlocking /*&& (fKind == mbs::TransportServer) */) return false;

   bool allcansend = CanSendToAllOutputs(true);

//   if (!allcansend)
//      DOUT1("mbs::ServerTransport::ProcessRecv  numout:%u cansend:%s numwork:%u", NumOutputs(), DBOOL(allcansend), numoutputs);

   // in case of transport buffer all outputs should be
   // ready to get next buffer. Otherwise input port will be blocked
   if (!allcansend) {
      // if server must deliver all events, than wait (default for transport, can be enabled for stream)
      if (fDeliverAll) return false;
      // if server do not blocks, first wait until input queue will be filled
      if (!RecvQueueFull()) {
         // DOUT0("mbs::ServerTransport::ProcessRecv let input queue to be filled size:%u", NumCanRecv());
         // dabc::SetDebugLevel(1);
         SignalRecvWhenFull();
         return false;
      }
      // DOUT0("TRY TO SEND EVEN WHEN NOT POSSIBLE");
   }

   // this is normal situation when buffer can be send to all outputs

   dabc::Buffer buf = Recv();

   bool iseof = (buf.GetTypeId() == dabc::mbt_EOF);

   SendToAllOutputs(buf);

   if (iseof) {
      DOUT2("Server transport saw EOF buffer");
      fDoingClose = 1;

      if ((NumOutputs()==0) || !fBlocking) {
         DOUT2("One could close server transport immediately");
         CloseTransport(false);
         fDoingClose = 2;
         return false;
      }
   }

   return fDoingClose == 0;
}


void mbs::ServerTransport::ProcessConnectionActivated(const std::string &name, bool on)
{
   if (name==InputName()) {
      dabc::Transport::ProcessConnectionActivated(name, on);
   } else {
      DOUT2("mbs::ServerTransport detect new client on %s %s", name.c_str(), (on ? "CONNECTED" : "DISCONNECTED") );

      if (on) {
         ProcessInputEvent(0);
         return;
      }

      if (!on) FindPort(name).Disconnect();

      if (fDoingClose == 1) {
         bool isany = false;
         for (unsigned n=0;n<NumOutputs();n++)
            if (IsOutputConnected(n)) isany = true;
         if (!isany) {
            DOUT2("Close server transport while all clients are closed");
            CloseTransport(false);
            fDoingClose = 2;
         }
      }
   }
}
