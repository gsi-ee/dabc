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
#include "dabc/Publisher.h"
#include "dabc/Url.h"

// TODO: All classes here should be factorized so that socket processor can be replaced by
//       the verbs processor and socket addresses replaced by verbs addresses
//       Thus, it should be possible to replace socket by IB in command channel


dabc::SocketCommandClient::SocketCommandClient(Reference parent, const std::string& name,
                                               SocketAddon* addon,
                                               const std::string& hostname,
                                               double reconnect) :
   dabc::Worker(MakePair(parent, name)),
   fRemoteHostName(hostname),
   fReconnectPeriod(reconnect),
   fCmds(),
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
   fRemoteName(),
   fMasterConn(false),
   fClientNameSufix()
{
   AssignAddon(addon);

   SetAutoDestroy(true);

   SocketIOAddon* io = dynamic_cast<SocketIOAddon*> (addon);

   // we are getting I/O in the constructor, means connection is established
   if (io!=0) {
      fState = stWorking;
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
   if (!fRemoteHostName.empty() && (fReconnectPeriod>0) && fAddon.null())
      ActivateTimeout(0.);
   else
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

   if (!fRemoteHostName.empty() && (fReconnectPeriod>0)) {
      AssignAddon(0); // we destroy current addon
      DOUT0("Try to reconnect worker %s to remote node %s", ItemName().c_str(), fRemoteHostName.c_str());
      fState = stConnecting;
      ActivateTimeout(fReconnectPeriod);
   } else {
      fState = iserr ? stFailure : stClosing;
      DeleteThis();
   }
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

      if (fMasterConn) {
         dabc::Command cmd("AcceptClient");
         std::string myname = dabc::SocketThread::DefineHostName();
         if (myname.empty()) myname = "Client";
         if (!fClientNameSufix.empty())
            myname+="_"+fClientNameSufix;

         // this name is used somewhere in hierarchy
         cmd.SetStr("name", myname);

         // this name will be used to identify our node and deliver commands
         cmd.SetStr("globalname", dabc::mgr.GetLocalAddress());

         AppendCmd(cmd);
      }

      // DOUT0("Start header recv fd:%d addon:%p numrefs:%u", addon->Socket(), addon, NumReferences());

      FireEvent(evntActivate);

      return dabc::cmd_true;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}

bool dabc::SocketCommandClient::ExecuteCommandByItself(Command cmd)
{
   if (cmd.IsName("AcceptClient")) {
      DOUT0("We allow to transform connection to the monitoring channel");
      cmd.SetResult(cmd_true);

      fRemoteObserver = true;

      std::string name = cmd.GetStr("name");
      std::string globalname = cmd.GetStr("globalname");
      if (!name.empty()) fRemoteName = name;
      if (!globalname.empty() && fRemoteHostName.empty()) {
         fRemoteHostName = globalname;
         DOUT0("ACCEPT remote %s", fRemoteHostName.c_str());
      }

      // here we inform publisher that new node want to add its hierarchy to the master
      // publisher will request hierarchy itself
      PublisherRef(GetPublisher()).AddRemote(fRemoteHostName, ItemName());

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
      if (!cmd.ReadFromXml(fRecvBuf)) {
         CloseClient(true, "command format error");
         return;
      }
   }

   // DOUT0("Worker: %s get command %s from remote kind %u ", ItemName().c_str(), cmd.GetName(), (unsigned) fRecvHdr.data_kind);

   switch (fRecvHdr.data_kind) {

      case kindCommand:
         if (!fExeCmd.null()) {
            CloseClient(true, "Command send without previous is completed");
            break;
         }

         fExeCmd = cmd;

         if (!fExeCmd.null() && (fRecvHdr.data_rawsize>0)) {
            dabc::Buffer rawdata = dabc::Buffer::CreateBuffer(fRecvBuf + fRecvHdr.data_cmdsize, fRecvHdr.data_rawsize, false, true);
            fExeCmd.SetRawData(rawdata);
         }

         if (ExecuteCommandByItself(fExeCmd)) {

            fExeCmd.RemoveReceiver();
            SubmitCommandSend(fExeCmd, true);
            fExeCmd.Release();

         } else {

            double tmout = fRecvHdr.data_timeout * 0.001;

            if (tmout>0) fExeCmd.SetTimeout(tmout);

            DOUT2("Submit command %s rcv:%s for execution", fExeCmd.GetName(), fExeCmd.GetReceiver().c_str());

            // indicate that command must be executed locally
            // done while receiver may contain different address format and may not recognized by Manager
            cmd.SetBool("#local_cmd", true);

            dabc::mgr.Submit(Assign(cmd));
         }

         break;

      case kindReply:
         if (fMainCmd.null()) {
            CloseClient(true, "Command reply at wrong time");
            return;
         }

         fMainCmd.AddValuesFrom(cmd);

         if (!fMainCmd.null() && (fRecvHdr.data_rawsize>0)) {
            dabc::Buffer rawdata = dabc::Buffer::CreateBuffer(fRecvBuf + fRecvHdr.data_cmdsize, fRecvHdr.data_rawsize, false, true);
            fMainCmd.SetRawData(rawdata);
         }

         fMainCmd.Reply(cmd.GetResult());

         // if any new commands are in the queue, submit them
         if (fCmds.Size()>0) FireEvent(evntActivate);

         break;

      case kindCancel:
         // this is reply from server that command is canceled
         if (fMainCmd.null())
            CloseClient(true, "Command cancel at wrong time");
         else
            fMainCmd.Reply(cmd_timedout);

         // if any new commands are in the queue, submit them
         if (fCmds.Size()>0) FireEvent(evntActivate);

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

            // DOUT0("Start command recv data_size %u", fRecvHdr.data_size);
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
         if (fMainCmd.null() && (fCmds.Size()>0) && (fState == stWorking)) {

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

//   DOUT0("RAWSEND cmd %s debugid %d", fSendQueue.Front().GetName(), fSendQueue.Front().GetInt("debugid"));

   fSendQueue.Front().RemoveField("##dabc_send_kind_reply##");
   fSendQueue.Front().RemoveField("##dabc_send_tmout##");

   fSendHdr.dabc_header = headerDabc;
   fSendHdr.data_kind = asreply ? kindReply : kindCommand;
   fSendHdr.data_timeout = send_tmout > 0 ? (unsigned) send_tmout*1000. : 0;
   fSendHdr.data_size = 0;
   fSendHdr.data_cmdsize = 0;
   fSendHdr.data_rawsize = 0;

   fSendBuf.clear();
   fSendRawData.Release();

   void* rawdata = 0;

   if (fSendQueue.Front().IsCanceled()) {
      fSendHdr.data_kind = kindCancel;
   } else {
      fSendBuf = fSendQueue.Front().SaveToXml(true);
      fSendRawData = fSendQueue.Front().GetRawData();

      fSendHdr.data_cmdsize = fSendBuf.length() + 1; // transport 0-terminated string as is
      fSendHdr.data_rawsize = fSendRawData.GetTotalSize();
      if (fSendHdr.data_rawsize > 0) rawdata = fSendRawData.SegmentPtr();
      fSendHdr.data_size = fSendHdr.data_cmdsize + fSendHdr.data_rawsize;
   }

   SocketIOAddon* addon = dynamic_cast<SocketIOAddon*> (fAddon());

   if (addon==0) {
      CloseClient(true, "I/O object missing");
      return;
   }

   // DOUT0("Start command send fullsize:%u cmd:%u raw:%u", fSendHdr.data_size, fSendHdr.data_cmdsize, fSendHdr.data_rawsize);

   if (!addon->StartSend(&fSendHdr, sizeof(fSendHdr),
                         fSendBuf.c_str(), fSendHdr.data_cmdsize,
                         rawdata, fSendHdr.data_rawsize)) {
      CloseClient(true, "Fail to send data");
      return;
   }

   // DOUT0("Worker:%s numrefs:%d Send command buffer len %u via addon:%p \n %s", ItemName().c_str(), NumReferences(), fSendHdr.data_size, addon, fSendBuf.c_str());
}


double dabc::SocketCommandClient::ProcessTimeout(double last_diff)
{
//   DOUT0("dabc::SocketCommandClient::ProcessTimeout");

   double next_tmout = 1.;

   if (!fRemoteHostName.empty() && (fReconnectPeriod>0) && fAddon.null()) {

      SocketClientAddon* client = dabc::SocketThread::CreateClientAddon(fRemoteHostName, defaultDabcPort);

      // DOUT0("Create client %p to host %s", client, fRemoteHostName.c_str());

      if (client!=0) {
         client->SetRetryOpt(2000000000, fReconnectPeriod);

         client->SetDeliverEventsToWorker(true);

         AssignAddon(client);
      } else {
         next_tmout = fReconnectPeriod;
      }
   }

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
         DOUT0("Command timedout %s", fExeCmd.GetName());

         Command repl = fExeCmd;
         fExeCmd.Cancel(); // this will release reference
         fExeCmd.Release();
         SubmitCommandSend(repl, true);
      }
   }

   return next_tmout;
}

// =======================================================================================



dabc::SocketCommandChannel::SocketCommandChannel(const std::string& name, SocketServerAddon* connaddon, Command cmd) :
   dabc::Worker(MakePair(name.empty() ? "/CommandChannel" : name)),
   fNodeId(0),
   fClientCnt(0)
{
   fNodeId = dabc::mgr()->cfg()->MgrNodeId();

   AssignAddon(connaddon);

   // object is owner its childs - autodestroy flag will be automatically set to the new add object
   SetOwner(true);
}

dabc::SocketCommandChannel::~SocketCommandChannel()
{
   // fHierarchy.Destroy();
}

void dabc::SocketCommandChannel::OnThreadAssigned()
{
}


std::string dabc::SocketCommandChannel::GetRemoteNode(const std::string& url_str)
{
   std::string server, itemname;
   bool islocal(true);

   if (dabc::mgr.DecomposeAddress(url_str, islocal, server, itemname))
      if (!islocal) return server;

   return std::string();
}


dabc::SocketCommandClientRef dabc::SocketCommandChannel::ProvideWorker(const std::string& remnodename, double conn_tmout)
{
   SocketCommandClientRef worker;

   if (remnodename.empty()) return worker;

   for (unsigned n=0;n<NumChilds();n++) {
      worker = GetChildRef(n);
      if (!worker.null() && (worker()->fRemoteHostName == remnodename)) break;
      worker.Release();
   }

   if (!worker.null() || (conn_tmout<0)) return worker;

   // we create worker only if address with port is specified
   // dabc::Url url(remnodename);
   // if (url.GetPort()<=0) return worker;

   std::string worker_name = dabc::format("Client%d", fClientCnt++);

   worker = new SocketCommandClient(this, worker_name, 0, remnodename, conn_tmout);

   worker()->AssignToThread(thread());

   return worker;
}



int dabc::SocketCommandChannel::PreviewCommand(Command cmd)
{
   std::string receiver = cmd.GetReceiver();
   if (receiver.empty()) return dabc::Worker::PreviewCommand(cmd);

   std::string remnodename = GetRemoteNode(receiver);

   dabc::SocketCommandClientRef worker = ProvideWorker(remnodename, 1);

   DOUT2("SEARCH node %s receiver %s worker %p", remnodename.c_str(), cmd.GetReceiver().c_str(), worker());

   if (worker.null()) return dabc::Worker::PreviewCommand(cmd);

   DOUT3("Append command %s to client %s", cmd.GetName(), worker.ItemName().c_str());

   worker()->AppendCmd(cmd);

   // worker.Release();

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

      worker = new SocketCommandClient(this, worker_name, io);

      worker()->AssignToThread(thread());

      DOUT0("SocketCommand - create server side fd:%d worker:%s thread:%s", fd, worker_name.c_str(), thread().GetName());

      worker.Release();

      return dabc::cmd_true;
   } else
   if (cmd.IsName("disconnect") || cmd.IsName("close")) {
      std::string remnodename = GetRemoteNode(cmd.GetStr("host"));

      if (remnodename.empty()) return dabc::cmd_false;

      dabc::Url url(remnodename);
      if (url.IsValid() && (url.GetProtocol()=="dabc")) {

         SocketCommandClientRef worker = ProvideWorker(url.GetHostNameWithPort(defaultDabcPort));

         if (!worker.null()) {
            DOUT0("Close connection to %s", remnodename.c_str());
            worker.Destroy();
            return dabc::cmd_true;
         }
      }

      DOUT0("No connection to %s exists", remnodename.c_str());

      return dabc::cmd_false;
   } else
   if (cmd.IsName("ConfigureMaster")) {

      Url url(cmd.GetStr("Master"));

      // std::string dabc::Manager::ComposeAddress(const std::string& server, const std::string& itemname)

      std::string remnode = url.GetHostNameWithPort(defaultDabcPort);

      dabc::SocketCommandClientRef worker = ProvideWorker(remnode, 3.);

      // DOUT0("Start worker %s to connect with master %s", worker.GetName(), remnode.c_str());

      if (worker.null()) return dabc::cmd_true;

      // indicate that this is special connection to master node
      // each time it is reconnected, it will append special command to register itself in remote master
      worker()->fMasterConn = true;
      worker()->fClientNameSufix = cmd.GetStr("NameSufix");

      return dabc::cmd_true;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}

double dabc::SocketCommandChannel::ProcessTimeout(double last_diff)
{
   return -1;
}

