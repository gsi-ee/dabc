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

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/MemoryPool.h"
#include "dabc/DataTransport.h"

mbs::ServerOutputAddon::ServerOutputAddon(int fd, int kind) :
   dabc::SocketIOAddon(fd, false, true),
   dabc::DataOutput(dabc::Url()),
   fState(oInit),
   fKind(kind),
   fSendBuffers(0),
   fDroppedBuffers(0)
{
   DOUT0("Create MBS server addon fd:%d kind:%s", fd, mbs::ServerKindToStr(kind));
}

mbs::ServerOutputAddon::~ServerOutputAddon()
{
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
//   DOUT0("mbs::ServerOutputAddon::OnThreadAssigned - send info");

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
         DOUT0("Send info completed");
         if (fKind == mbs::StreamServer)
            fState = oWaitingReq;
         else
            fState = oWaitingBuffer;
         return;

      case oInitReq:
         fState = oWaitingBuffer;
         return;

      case oSendingBuffer:
         if (fKind == mbs::StreamServer)
            fState = oWaitingReq;
         else
            fState = oWaitingBuffer;
         MakeCallback(dabc::do_Ok);
         return;

      default:
         EOUT("Send complete at wrong state");
         break;
   }
}

void mbs::ServerOutputAddon::OnRecvCompleted()
{
//   DOUT0("mbs::ServerOutputAddon::OnRecvCompleted %s  inp:%s out:%s", f_sbuf, DBOOL(IsDoingInput()), DBOOL(IsDoingOutput()));

   if (strcmp(f_sbuf, "CLOSE")==0) {
      fState = oDoingClose;
      SubmitWorkerCmd(dabc::Command("CloseTransport"));
      return;
   }

   if (strcmp(f_sbuf, "GETEVT")!=0)
     EOUT("Wrong request string %s", f_sbuf);

   memset(f_sbuf, 0, sizeof(f_sbuf));
   StartRecv(f_sbuf, 12);

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

   fHeader.Init(true);
   fHeader.SetUsedBufferSize(buf.GetTotalSize());

   // error in evapi, must be + sizeof(mbs::BufferHeader)
   fHeader.SetFullSize(buf.GetTotalSize() - sizeof(mbs::BufferHeader));
   fState = oSendingBuffer;

   StartNetSend(&fHeader, sizeof(fHeader), buf);

   return dabc::do_CallBack;
}

// ===============================================================================

mbs::ServerTransport::ServerTransport(dabc::Command cmd, const dabc::PortRef& outport, int kind, dabc::SocketServerAddon* connaddon, int limit, bool blocking) :
   dabc::Transport(cmd, 0, outport),
   fKind(kind),
   fSlaveQueueLength(5),
   fClientsLimit(limit),
   fDoingClose(0),
   fBlocking(blocking)
{
   // this addon handles connection
   AssignAddon(connaddon);

   // TODO: queue length one can take from configuration
   fSlaveQueueLength = 5;

   if (fClientsLimit>0) DOUT0("Set client limit for MBS server to %d", fClientsLimit);

//   DOUT0("mbs::ServerTransport   isinp=%s", DBOOL(connaddon->IsDoingInput()));
}

mbs::ServerTransport::~ServerTransport()
{
}

void mbs::ServerTransport::ObjectCleanup()
{
   dabc::Transport::ObjectCleanup();
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

      DOUT0("Get new connection request with fd %d", fd);

      ServerOutputAddon* addon = new ServerOutputAddon(fd, fKind);
      // FIXME: should we configure buffer size or could one ignore it???
      addon->FillServInfo(0x100000, true);

      if (portindx<0) portindx = CreateOutput(dabc::format("Slave%u",NumOutputs()), fSlaveQueueLength);

      dabc::TransportRef tr = new dabc::OutputTransport(dabc::Command(), FindPort(OutputName(portindx)), addon, false, addon);

      tr()->AssignToThread(thread(), true);

      // transport will be started automatically

      dabc::LocalTransport::ConnectPorts(FindPort(OutputName(portindx)), tr.InputPort());

      DOUT0("mbs::ServerTransport create new connection at running=%s", DBOOL(isTransportRunning()));

//      dabc::SetDebugLevel(1);

      return dabc::cmd_true;
   }


   return dabc::Transport::ExecuteCommand(cmd);
}

bool mbs::ServerTransport::SendNextBuffer()
{
   if (!CanRecv()) return false;

   // unconnected transport server will block until any connection is established
   if ((NumOutputs()==0) && fBlocking && (fKind == mbs::TransportServer)) return false;

   bool allcansend = CanSendToAllOutputs(true);

//   if (!allcansend)
//      DOUT1("mbs::ServerTransport::ProcessRecv  numout:%u cansend:%s numwork:%u", NumOutputs(), DBOOL(allcansend), numoutputs);

   // in case of transport buffer all outputs should be
   // ready to get next buffer. Otherwise input port will be blocked
   if (!allcansend) {
      // blocking transport server will wait until all clients can get next buffer
      if ((fKind == mbs::TransportServer) && fBlocking) return false;
      // if server do not blocks, first wait until input queue will be filled
      if (!RecvQueueFull()) {
         DOUT3("mbs::ServerTransport::ProcessRecv LET input queue to be filled size:%u", NumCanRecv());
         //dabc::SetDebugLevel(1);
         SignalRecvWhenFull();
         return false;
      }
   }

   // this is normal situation when buffer can be send to all outputs

   dabc::Buffer buf = Recv();

   bool iseof = (buf.GetTypeId() == dabc::mbt_EOF);

   SendToAllOutputs(buf);

   if (iseof) {
      DOUT0("Server transport saw EOF buffer");
      fDoingClose = 1;

      if ((NumOutputs()==0) || !fBlocking) {
         DOUT0("One could close server transport immediately");
         CloseTransport(false);
         fDoingClose = 2;
         return false;
      }
   }

   return fDoingClose == 0;
}


void mbs::ServerTransport::ProcessConnectionActivated(const std::string& name, bool on)
{
   if (name==InputName()) {
      dabc::Transport::ProcessConnectionActivated(name, on);
   } else {
      DOUT0("mbs::ServerTransport detect new client on %s %s", name.c_str(), (on ? "CONNECTED" : "DISCONNECTED") );

      if (on) {
         ProcessInputEvent();
         return;
      }


      if (!on) FindPort(name).Disconnect();

      if (fDoingClose == 1) {
         bool isany = false;
         for (unsigned n=0;n<NumOutputs();n++)
            if (IsOutputConnected(n)) isany = true;
         if (!isany) {
            DOUT1("Close server transport while all clients are closed");
            CloseTransport(false);
            fDoingClose = 2;
         }
      }
   }
}
