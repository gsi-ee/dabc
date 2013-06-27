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

#include "dabc/SocketCommandChannel.h"

#include <stdlib.h>
#include <unistd.h>
#include <list>

#include "dabc/timing.h"
#include "dabc/Manager.h"
#include "dabc/Application.h"
#include "dabc/Configuration.h"
#include "dabc/Url.h"

// TODO: All classes here should be factorized so that socket processor can be replaced by
//       the verbs processor and socket addresses replaced by verbs addresses
//       Thus, it should be possible to replace socket by IB in command channel



dabc::SocketCommandClient::SocketCommandClient(Reference parent, const std::string& name,
                                               SocketAddon* addon,
                                               const std::string& hostname) :
   dabc::Worker(MakePair(parent, name)),
   fIsClient(!hostname.empty()),
   fRemoteHostName(hostname),
   fConnected(true),
   fCmds(fIsClient ? CommandsQueue::kindPostponed : CommandsQueue::kindNone),
   fState(stConnecting),
   fSendHdr(),
   fSendBuf(),
   fRecvHdr(),
   fRecvBuf(0),
   fRecvBufSize(0),
   fMainCmd(),
   fExeCmd(),
   fSendQueue(),
   fRecvState(recvInit),
   fRemoteObserver(false),
   fRemReqTime(),
   fRemoteHierarchy()
{
   AssignAddon(addon);

   SocketIOAddon* io = dynamic_cast<SocketIOAddon*> (addon);

   // we are getting I/O in the constructor, means connection is established
   if (io!=0) {
      fRecvState = recvHeader;
      io->StartRecv(&fRecvHdr, sizeof(fRecvHdr));
   }
}

dabc::SocketCommandClient::~SocketCommandClient()
{
   EnsureRecvBuffer(0);
}

void dabc::SocketCommandClient::OnThreadAssigned()
{
   ActivateTimeout(1.);
}


bool dabc::SocketCommandClient::EnsureRecvBuffer(unsigned strsize)
{
   if ((strsize>0) && (strsize< fRecvBufSize)) return true;

   if (fRecvBufSize > 0) {
      delete [] fRecvBuf;
      fRecvBuf = 0;
      fRecvBufSize = 0;
   }

   if (strsize == 0) return true;

   fRecvBufSize = 2048;
   while (fRecvBufSize <= strsize) fRecvBufSize *=2;

   fRecvBuf = new char[fRecvBufSize];

   if (fRecvBuf==0) {
      EOUT("Cannot allocate buffer %u", fRecvBufSize);
      fRecvBufSize = 0;
      return false;
   }

   DOUT0("ALLOCATE %u", fRecvBufSize);

   return true;
}

void dabc::SocketCommandClient::CloseClient(bool iserr, const char* msg)
{
   if (msg!=0) {
      if (iserr)
         EOUT("Worker %s closing connection due to error %s", ItemName().c_str(), msg);
      else
         DOUT0("Worker %s closing connection due to %s", ItemName().c_str(), msg);
   }
   fState = iserr ? stFailure : stClosing;
   DeleteThis();
}


int dabc::SocketCommandClient::ExecuteCommand(Command cmd)
{
   if (cmd.IsName("SocketConnect")) {

      int fd = cmd.GetInt("fd", -1);

      if (fd<=0) return dabc::cmd_false;

      if (fState != stConnecting) {
         EOUT("Fatal error - connection again when it was established???");
         return dabc::cmd_false;
      }

      fState = stWorking;

      SocketIOAddon* addon = new SocketIOAddon(fd);
      addon->SetDeliverEventsToWorker(true);

      DOUT0("SocketCommand - create client side fd:%d worker:%s", fd, GetName());

      AssignAddon(addon);

      // DOUT0("Did addon assign numrefs:%u", NumReferences());

      fRecvState = recvHeader;
      // start receiving of header immediately
      addon->StartRecv(&fRecvHdr, sizeof(fRecvHdr));

      // DOUT0("Start header recv fd:%d addon:%p numrefs:%u", addon->Socket(), addon, NumReferences());

      FireEvent(evntActivate);

      return dabc::cmd_true;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}

bool dabc::SocketCommandClient::ExecuteCommandByItself(Command cmd)
{
   if (cmd.IsName("GetHierarchy")) {

      unsigned lastversion = cmd.GetUInt("version");
      cmd.RemoveField("version");

      SocketCommandChannel* channel = dynamic_cast<SocketCommandChannel*> (GetParent());

      if (channel==0) {
         EOUT("Channel not found!!!");
         return false;
      }

      channel->fHierarchy.UpdateHierarchy(dabc::mgr);

      std::string diff = channel->fHierarchy.SaveToXml(true, lastversion + 1);

      //DOUT0("Produce hierarchy %u  diff = %s", channel->fHierarchy.GetVersion(), diff.c_str());

      //DOUT0("Now hierarchy is \n %s", channel->fHierarchy.SaveToXml(false, (uint64_t) -1).c_str());

      cmd.SetRawData(diff.c_str(), diff.length(), false, true);

      cmd.SetResult(cmd_true);

      return true;
   }


   if (cmd.IsName("AcceptClient")) {
      DOUT0("We allow to transform connection to the monitoring channel");
      cmd.SetResult(cmd_true);

      fRemoteObserver = true;
      fRemReqTime.Reset();
      fRemoteHierarchy.Create("Remote");

      Command cmd("GetHierarchy");
      Assign(cmd);
      fCmds.Push(cmd);
      FireEvent(evntActivate);

      return true;
   }

   return false;
}


void dabc::SocketCommandClient::ProcessRecvPacket()
{
   if (fRecvHdr.data_kind == kindDisconnect) {
      CloseClient(false, "disconnect packet");
      return;
   }

   dabc::Command cmd;

   if (fRecvHdr.data_cmdsize > 0) {

      std::string sbuf;

      sbuf.append(fRecvBuf, fRecvHdr.data_cmdsize);

      if (!cmd.ReadFromXml(std::string(fRecvBuf))) {
         CloseClient(true, "command format error");
         return;
      }
   }

   DOUT0("Worker: %s get command %s from remote kind %u ", ItemName().c_str(), cmd.GetName(), (unsigned) fRecvHdr.data_kind);

   switch (fRecvHdr.data_kind) {

      case kindCommand:
         if (!fExeCmd.null()) {
            CloseClient(true, "Command send without previous is completed");
            break;
         }

         fExeCmd = cmd;

         if (!fExeCmd.null() && (fRecvHdr.data_rawsize>0))
            fExeCmd.SetRawData(fRecvBuf + fRecvHdr.data_cmdsize, fRecvHdr.data_rawsize, false, true);

         if (ExecuteCommandByItself(fExeCmd)) {

            fExeCmd.RemoveReceiver();

            SubmitCommandSend(fExeCmd, true);
            fExeCmd.Release();

         } else {

            double tmout = fRecvHdr.data_timeout * 0.001;

            if (tmout>0) fExeCmd.SetTimeout(tmout);

            DOUT0("Submit command %s for execution", fExeCmd.GetName());

            dabc::mgr.Submit(Assign(cmd));
         }

         break;

      case kindReply:
         if (fMainCmd.null()) {
            CloseClient(true, "Command reply at wrong time");
            return;
         }

         fMainCmd.AddValuesFrom(cmd);

         if (!fMainCmd.null() && (fRecvHdr.data_rawsize>0))
            fMainCmd.SetRawData(fRecvBuf + fRecvHdr.data_cmdsize, fRecvHdr.data_rawsize, false, true);

         fMainCmd.Reply(cmd.GetResult());
         break;

      case kindCancel:
         // this is reply from server that command is canceled
         if (fMainCmd.null())
            CloseClient(true, "Command cancel at wrong time");
         else
            fMainCmd.Reply(cmd_timedout);

         break;

   }

   fRecvState = recvHeader;
   // first of all, sumbit next recv header operation
   SocketIOAddon* io = dynamic_cast<SocketIOAddon*> (fAddon());
   io->StartRecv(&fRecvHdr, sizeof(fRecvHdr));
}

bool dabc::SocketCommandClient::ReplyCommand(Command cmd)
{
   if (cmd == fExeCmd) {

      SubmitCommandSend(fExeCmd, true);
      fExeCmd.Release();

      return true;
   }

   if (cmd.IsName("GetHierarchy")) {

      if (!fRemoteObserver) {
         EOUT("Get reply with hierarchy - why???");
         return true;
      }

      fRemReqTime.GetNow();

      if (cmd.GetRawDataSize() > 0) {
         // DOUT0("Get raw data %p %u", cmd2.GetRawData(), cmd2.GetRawDataSize());

         std::string diff;
         diff.append((const char*)cmd.GetRawData(), cmd.GetRawDataSize());
         //DOUT0("length %d diff = %s", diff.length(), diff.c_str());
         if (fRemoteHierarchy.UpdateFromXml(diff)) {
            DOUT0("Update of hierarchy to version %u done", fRemoteHierarchy.GetVersion());
            // DOUT0("Now \n%s", fRemoteHierarchy.SaveToXml().c_str());
         }
      }

      return true;
   }

   return dabc::Worker::ReplyCommand(cmd);
}



void dabc::SocketCommandClient::ProcessEvent(const EventId& evnt)
{
   switch (evnt.GetCode()) {
      case SocketAddon::evntSocketSendInfo:

         // DOUT0("Worker:%s complete sending!!!", GetName());

         if (fSendQueue.Size() == 0) {
            CloseClient(true, "get send complete at wrong state");
            return;
         }

         fSendQueue.Pop().Release();

         PerformCommandSend();

         break;

      case SocketAddon::evntSocketRecvInfo: {

         // DOUT0("Recv something via socket!");

         if (fRecvState == recvInit) {
            CloseClient(true, "Receive data in init state");
            return;
         }

         if (fRecvState == recvData) {
            fRecvState = recvInit;
            // here raw buffer is received and must be processed
            ProcessRecvPacket();
            return;
         }

         if (fRecvState == recvHeader) {
            // received header

            if (fRecvHdr.dabc_header != headerDabc) {
               CloseClient(true, "Wrong packet from network");
               return;
             }

            if (fRecvHdr.data_size == 0) {
               fRecvState = recvInit;
               // when no additional data, process packet
               ProcessRecvPacket();
               return;
            }

            if (!EnsureRecvBuffer(fRecvHdr.data_size)) {
               CloseClient(true, "memory allocation");
               return;
            }

            fRecvState = recvData;
            SocketIOAddon* io = dynamic_cast<SocketIOAddon*> (fAddon());
            io->StartRecv(fRecvBuf, fRecvHdr.data_size);

            DOUT0("Start command recv data_size %u", fRecvHdr.data_size);
         }

         break;
      }

      case SocketAddon::evntSocketErrorInfo:
         CloseClient(true, "Socket error");
         break;


      case SocketAddon::evntSocketCloseInfo:
         CloseClient(false, "Socket closed");
         break;

      case evntActivate:
         if (fMainCmd.null() && (fCmds.Size()>0)) {

            fMainCmd = fCmds.Pop();
            double tmout = fMainCmd.TimeTillTimeout();
            if (tmout==0.) {
               fMainCmd.Reply(cmd_timedout);
               FireEvent(evntActivate);
               break;
            }

            DOUT4("Client sends current cmd %s tmout:%5.3f", fMainCmd.GetName(), tmout);

            SubmitCommandSend(fMainCmd, false, tmout);
         }

         break;

      default:
         dabc::Worker::ProcessEvent(evnt);
   }

}

void dabc::SocketCommandClient::AppendCmd(Command cmd)
{
   if (!IsClient()) {
      EOUT("internal error - cannot send command via server-side of connection");
      cmd.ReplyFalse();
      return;
   }

   fCmds.Push(cmd);

   if (fMainCmd.null()) FireEvent(evntActivate);
}

void dabc::SocketCommandClient::SubmitCommandSend(dabc::Command cmd, bool asreply, double send_tmout)
{
   cmd.SetBool("##dabc_send_kind_reply##", asreply);
   cmd.SetDouble("##dabc_send_tmout##", send_tmout);
   fSendQueue.Push(cmd);

   if (fSendQueue.Size()==1) PerformCommandSend();
}

void dabc::SocketCommandClient::PerformCommandSend()
{
   if (fSendQueue.Size()==0) return;
   bool asreply = fSendQueue.Front().GetBool("##dabc_send_kind_reply##");
   double send_tmout = fSendQueue.Front().GetDouble("##dabc_send_tmout##");

   fSendQueue.Front().RemoveField("##dabc_send_kind_reply##");
   fSendQueue.Front().RemoveField("##dabc_send_tmout##");

   fSendHdr.dabc_header = headerDabc;
   fSendHdr.data_kind = asreply ? kindReply : kindCommand;
   fSendHdr.data_timeout = send_tmout > 0 ? (unsigned) send_tmout*1000. : 0;
   fSendHdr.data_size = 0;
   fSendHdr.data_cmdsize = 0;
   fSendHdr.data_rawsize = 0;

   fSendBuf.clear();

   if (fSendQueue.Front().IsCanceled()) {
      fSendHdr.data_kind = kindCancel;
   } else {
      fSendBuf = fSendQueue.Front().SaveToXml(true);
      fSendHdr.data_cmdsize = fSendBuf.length();
      fSendHdr.data_rawsize = fSendQueue.Front().GetRawDataSize();
      fSendHdr.data_size = fSendHdr.data_cmdsize + fSendHdr.data_rawsize;
   }

   SocketIOAddon* addon = dynamic_cast<SocketIOAddon*> (fAddon());

   if (addon==0) {
      CloseClient(true, "I/O object missing");
      return;
   }

   DOUT0("Start command send fullsize:%u cmd:%u raw:%u", fSendHdr.data_size, fSendHdr.data_cmdsize, fSendHdr.data_rawsize);

   if (!addon->StartSend(&fSendHdr, sizeof(fSendHdr),
                         fSendBuf.c_str(), fSendHdr.data_cmdsize,
                         fSendQueue.Front().GetRawData(), fSendHdr.data_rawsize)) {
      CloseClient(true, "Fail to send data");
      return;
   }

   // DOUT0("Worker:%s numrefs:%d Send command buffer len %u via addon:%p \n %s", ItemName().c_str(), NumReferences(), fSendHdr.data_size, addon, fSendBuf.c_str());
}


double dabc::SocketCommandClient::ProcessTimeout(double last_diff)
{
   if (!fMainCmd.null()) {
      double tmout = fMainCmd.TimeTillTimeout();
      if (tmout==0.) {
         fMainCmd.Reply(cmd_timedout);
         CloseClient(true, "timeout and server is hanging");
      }
   }

   if (!fExeCmd.null()) {
      double tmout = fExeCmd.TimeTillTimeout();

      if (tmout==0.) {
         fExeCmd.Cancel();
         SubmitCommandSend(fExeCmd, true);
         fExeCmd.Release();
      }
   }

   if (!fRemReqTime.null() && fRemReqTime.Expired(2.)) {
      fRemReqTime.Reset();

      Command cmd("GetHierarchy");
      cmd.SetInt("version", fRemoteHierarchy.GetVersion());
      Assign(cmd);
      fCmds.Push(cmd);
      FireEvent(evntActivate);
   }

   return 1.;
}

// =======================================================================================



dabc::SocketCommandChannel::SocketCommandChannel(const std::string& name, SocketServerAddon* connaddon) :
   dabc::Worker(MakePair(name.empty() ? "/CommandChannel" : name)),
   fNodeId(0),
   fClientCnt(0),
   fHierarchy()
{
   fNodeId = dabc::mgr()->cfg()->MgrNodeId();

   SetOwner(true);

   AssignAddon(connaddon);

   fHierarchy.Create("Full");
}

dabc::SocketCommandChannel::~SocketCommandChannel()
{
}

std::string dabc::SocketCommandChannel::GetRemoteNode(const std::string& url_str, int* nodeid)
{
   if (url_str.empty()) return std::string();

   int remnode = Url::ExtractNodeId(url_str);
   if (nodeid) *nodeid = remnode;

   if ((remnode >=0) && (remnode == fNodeId)) return std::string();

   std::string remnodename;
   int remport(0);

   if (remnode>=0) {
      remnodename = dabc::mgr()->cfg()->NodeName(remnode);
      remport = dabc::mgr()->cfg()->NodePort(remnode);
   } else {
      dabc::Url url(url_str);

      if (url.IsValid() && (url.GetProtocol()=="dabc")) {
         remnodename = url.GetHostName();
         remport = url.GetPort();
      }
   }

   if (remnodename.empty()) return std::string();

   if (remport <= 0) remport = 1237;

   return remnodename + dabc::format(":%d", remport);
}


std::string dabc::SocketCommandChannel::ClientWorkerName(const std::string& remnodename)
{
   std::string res = remnodename;

   std::size_t pos = 0;

   while ((pos = res.find_first_of(":/. ")) != std::string::npos) res[pos] = '_';

   return res;
}


int dabc::SocketCommandChannel::PreviewCommand(Command cmd)
{
   int nodeid(-1);

   std::string remnodename = GetRemoteNode(cmd.GetReceiver(), &nodeid);

   if (remnodename.empty())
      return dabc::Worker::PreviewCommand(cmd);

   std::string worker_name = ClientWorkerName(remnodename);

   SocketCommandClientRef worker = FindChildRef(worker_name.c_str());

   if (worker.null()) {
      SocketClientAddon* client = dabc::SocketThread::CreateClientAddon(remnodename);
      if (client==0) {
         EOUT("Cannot start client for the node %s", remnodename.c_str());
         return cmd_false;
      }

      // retry endless number of times
      if (nodeid>=0)
         client->SetRetryOpt(2000000000, 0.1);
      else {
         double tmout = cmd.TimeTillTimeout();
         client->SetRetryOpt(5, tmout > 0. ? tmout/5 : 0.5);
      }

      client->SetDeliverEventsToWorker(true);

      worker = new SocketCommandClient(this, worker_name, client, remnodename);

      worker()->AssignToThread(thread());
   }

   DOUT3("Append command %s to client %s", cmd.GetName(), worker.ItemName().c_str());

   worker()->AppendCmd(cmd);

   worker.Release();

   return cmd_postponed;
}

int dabc::SocketCommandChannel::ExecuteCommand(Command cmd)
{
   if (cmd.IsName("SocketConnect")) {

      int fd = cmd.GetInt("fd", -1);

      if (fd<=0) return dabc::cmd_false;

      std::string worker_name = dabc::format("Server%d", fClientCnt++);

      SocketCommandClientRef worker = FindChildRef(worker_name.c_str());

      if (!worker.null()) {
         EOUT("How such possible - client %s already exists", worker_name.c_str());
         close(fd);
         return dabc::cmd_false;
      }

      SocketIOAddon* io = new SocketIOAddon(fd);
      io->SetDeliverEventsToWorker(true);

      DOUT0("SocketCommand - create server side fd:%d worker:%s", fd, worker_name.c_str());

      worker = new SocketCommandClient(this, worker_name, io);

      worker()->AssignToThread(thread());

      worker.Release();

      return dabc::cmd_true;
   } else
   if (cmd.IsName("disconnect") || cmd.IsName("close")) {
      std::string remnodename = GetRemoteNode(cmd.GetStdStr("host"));

      if (remnodename.empty()) return dabc::cmd_false;

      std::string worker_name = ClientWorkerName(remnodename);

      SocketCommandClientRef worker = FindChildRef(worker_name.c_str());

      if (!worker.null()) {
         DOUT0("Close connection to %s", remnodename.c_str());
         worker.Destroy();
         return dabc::cmd_true;
      }

      DOUT0("No connection to %s exists", remnodename.c_str());

      return dabc::cmd_false;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}


void dabc::SocketCommandChannel::BuildHierarchy(HierarchyContainer* cont)
{
   // do it here, while all properties of main node are ignored when hierarchy is build
   dabc::Hierarchy top(cont);
   top.Field(dabc::prop_binary_producer).SetStr(ItemName());

   for (unsigned n=0;n<NumChilds();n++) {
      WorkerRef w = GetChildRef(n);

      SocketCommandClient* cl = dynamic_cast<SocketCommandClient*> (w.GetObject());
      if ((cl==0) || !cl->fRemoteObserver) continue;

      std::string name = dabc::format("Slave%u", n);

      dabc::Hierarchy chld = top.CreateChild(name);

      cl->fRemoteHierarchy()->BuildHierarchy(chld());
   }
}

