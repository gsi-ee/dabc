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
   fCurrentCmd()
{
   AssignAddon(addon);

   SocketIOAddon* io = dynamic_cast<SocketIOAddon*> (addon);

   // we are getting I/O in the constructor, means connection is established
   if (io!=0) {
      io->StartRecv(&fRecvHdr, sizeof(fRecvHdr));
      fState = stWaitCmd;
   }

}

dabc::SocketCommandClient::~SocketCommandClient()
{
   EnsureRecvBuffer(0);
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

      fState = stWaitCmd;

      SocketIOAddon* addon = new SocketIOAddon(fd);
      addon->SetDeliverEventsToWorker(true);

      DOUT0("SocketCommand - create client side fd:%d worker:%s", fd, GetName());

      AssignAddon(addon);

      // DOUT0("Did addon assign numrefs:%u", NumReferences());

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

      std::string diff = channel->fHierarchy.SaveToXml(false, lastversion + 1);

      DOUT0("Produce hierarchy %u", channel->fHierarchy.GetVersion());

      cmd.SetRawData(diff.c_str(), diff.length(), false, true);

      cmd.SetResult(cmd_true);

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

   DOUT0("Worker: %s get command %s from remote", ItemName().c_str(), cmd.GetName());

   if (IsClient()) {

      if (fRecvHdr.data_kind == kindCancel) {
         // this is situation when server was not able to process command in time
         fCurrentCmd.Reply(cmd_timedout);

      } else {
         fCurrentCmd.AddValuesFrom(cmd);

         if (!fCurrentCmd.null() && (fRecvHdr.data_rawsize>0)) {
            fCurrentCmd.SetRawData(fRecvBuf + fRecvHdr.data_cmdsize, fRecvHdr.data_rawsize, false, true);
         }

         fCurrentCmd.Reply(cmd.GetResult());
      }

      fState = stWaitCmd;

   } else {
      fState = stExecuting;

      fCurrentCmd = cmd;

      if (!fCurrentCmd.null() && (fRecvHdr.data_rawsize>0)) {
         fCurrentCmd.SetRawData(fRecvBuf + fRecvHdr.data_cmdsize, fRecvHdr.data_rawsize, false, true);
      }

      if (ExecuteCommandByItself(fCurrentCmd)) {

         fCurrentCmd.RemoveReceiver();

         SendCurrentCommand();

      } else {

         double tmout = fRecvHdr.data_timeout * 0.001;

         if (tmout>0) fCurrentCmd.SetTimeout(tmout);

         DOUT0("Submit command %s for execution", fCurrentCmd.GetName());

         ActivateTimeout(tmout > 0. ? tmout + 0.1 : 10.);

         dabc::mgr.Submit(Assign(cmd));
      }
   }

   // first of all, sumbit next recv header operation
   SocketIOAddon* io = dynamic_cast<SocketIOAddon*> (fAddon());
   io->StartRecv(&fRecvHdr, sizeof(fRecvHdr));
}

bool dabc::SocketCommandClient::ReplyCommand(Command cmd)
{

   // DOUT0("Worker: %s get command %s reply curr %s iscurr %s", ItemName().c_str(), cmd.GetName(), fCurrentCmd.GetName(), DBOOL(cmd == fCurrentCmd));

   if (IsServer() && (cmd == fCurrentCmd)) {

      if (fState != stExecuting) {
         CloseClient(true, "Get reply not in execution state");
         return false;
      }

      SendCurrentCommand();

      // we will release command when operation completed while raw buffer may be used during transfer

      return true;
   }

   return dabc::Worker::ReplyCommand(cmd);
}



void dabc::SocketCommandClient::ProcessEvent(const EventId& evnt)
{
   switch (evnt.GetCode()) {
      case SocketAddon::evntSocketSendInfo:

         // DOUT0("Worker:%s complete sending!!!", GetName());

         if (fState != stSending) {
            CloseClient(true, "get send complete at wrong state");
            return;
         }

         fState = IsClient() ? stWaitReply : stWaitCmd;

         if (IsServer()) fCurrentCmd.Release();

         break;

      case SocketAddon::evntSocketRecvInfo: {

         // DOUT0("Recv something via socket!");

         if (fState == stReceiving) {
            // here raw buffer is received and must be processed
            ProcessRecvPacket();
            return;
         }

         if ((IsClient() && (fState == stWaitReply)) || (IsServer() && (fState == stWaitCmd))) {
            // received header

            if (fRecvHdr.dabc_header != headerDabc) {
               CloseClient(true, "Wrong packet from network");
               return;
             }

            if (fRecvHdr.data_size == 0) {
               // when no additional data, process packet
               ProcessRecvPacket();
               return;
            }

            if (!EnsureRecvBuffer(fRecvHdr.data_size)) {
               CloseClient(true, "memory allocation");
               return;
            }

            fState = stReceiving;
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
         if ((fState==stWaitCmd) && (fCmds.Size()>0)) {

            fCurrentCmd = fCmds.Pop();
            double tmout = fCurrentCmd.TimeTillTimeout();
            if (tmout==0.) {
               fCurrentCmd.Reply(cmd_timedout);
               FireEvent(evntActivate);
               break;
            }

            DOUT4("Client sends current cmd %s tmout:%5.3f", fCurrentCmd.GetName(), tmout);

            SendCurrentCommand(tmout);

            // activate timeout to await response
            ActivateTimeout(tmout > 0 ? tmout + ExtraClientTimeout() : 10.);
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

   if (fCurrentCmd.null()) FireEvent(evntActivate);
}

void dabc::SocketCommandClient::SendCurrentCommand(double send_tmout)
{
   fSendHdr.dabc_header = headerDabc;
   fSendHdr.data_kind = IsClient() ? kindCommand : kindReply;
   fSendHdr.data_timeout = send_tmout > 0 ? (unsigned) send_tmout*1000. : 0;
   fSendHdr.data_size = 0;
   fSendHdr.data_cmdsize = 0;
   fSendHdr.data_rawsize = 0;

   fSendBuf.clear();
   if (!fCurrentCmd.null()) {
      fSendBuf = fCurrentCmd.SaveToXml(true);
      fSendHdr.data_cmdsize = fSendBuf.length();
      fSendHdr.data_rawsize = fCurrentCmd.GetRawDataSize();
      fSendHdr.data_size = fSendHdr.data_cmdsize + fSendHdr.data_rawsize;
   }

   if (fSendBuf.length() == 0) fSendHdr.data_kind = kindCancel;

   SocketIOAddon* addon = dynamic_cast<SocketIOAddon*> (fAddon());

   if (addon==0) {
      CloseClient(true, "I/O object missing");
      return;
   }

   DOUT0("Start command send fullsize:%u cmd:%u raw:%u", fSendHdr.data_size, fSendHdr.data_cmdsize, fSendHdr.data_rawsize);

   if (!addon->StartSend(&fSendHdr, sizeof(fSendHdr),
                         fSendBuf.c_str(), fSendHdr.data_cmdsize,
                         fCurrentCmd.GetRawData(), fSendHdr.data_rawsize)) {
      CloseClient(true, "Fail to send data");
      return;
   }

   // DOUT0("Worker:%s numrefs:%d Send command buffer len %u via addon:%p \n %s", ItemName().c_str(), NumReferences(), fSendHdr.data_size, addon, fSendBuf.c_str());

   fState = stSending;
}


double dabc::SocketCommandClient::ProcessTimeout(double last_diff)
{
   if (fCurrentCmd.null()) return -1.;

   double tmout = fCurrentCmd.TimeTillTimeout();

   // if no timeout was specified, repeat timeout not very often
   if (tmout<0.) {
      DOUT0("Command %s is executing too long", fCurrentCmd.GetName());
      return 10.;
   }

   if (tmout>0.) return tmout + IsClient() ? ExtraClientTimeout() : ExtraServerTimeout();

   // from this point command was timed out, one need to do something with it

   if (IsServer()) {
      EOUT("Cancel command %s !!!", fCurrentCmd.GetName());
      fCurrentCmd.Cancel();
      SendCurrentCommand();
   } else {
      fCurrentCmd.Reply(cmd_timedout);
      CloseClient(true, "timeout and server is hanging");
   }

   return -1.;
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
         ::close(fd);
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
