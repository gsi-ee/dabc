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

#include "dabc/SocketDevice.h"

#include <unistd.h>

#include "dabc/SocketTransport.h"
#include "dabc/Manager.h"
#include "dabc/Configuration.h"
#include "dabc/ConnectionManager.h"

#define SocketServerTmout 0.2

namespace dabc {

   class SocketProtocolAddon;

   // TODO: Can we use here connection manager name - all information already there

   class NewConnectRec {
      public:
         std::string            fReqItem;    ///< reference in connection request
         SocketClientAddon*     fClient;     ///< client-side processor, to establish connection
         SocketProtocolAddon*   fProtocol;   ///< protocol processor, to verify connection id
         double                 fTmOut;      ///< used by device to process connection timeouts
         std::string            fConnId;     //! connection id
         Command                fLocalCmd;   ///< command from connection manager which should be replied when connection established or failed

         NewConnectRec() :
            fReqItem(),
            fClient(0),
            fProtocol(0),
            fTmOut(1.),
            fConnId(),
            fLocalCmd()
         {
         }

         NewConnectRec(const std::string &item,
                       ConnectionRequestFull& req,
                       SocketClientAddon* clnt) :
            fReqItem(item),
            fClient(clnt),
            fProtocol(0),
            fTmOut(0.),
            fLocalCmd()
         {
            fTmOut = req.GetConnTimeout() + SocketServerTmout;
            fConnId = req.GetConnId();
         }

         const char* ConnId() const { return fConnId.c_str(); }
         bool IsConnId(const char* id) { return fConnId.compare(id) == 0; }
   };

   // this class is used to perform initial protocol
   // when socket connection is established
   // it also used to transport commands on remote side and execute them

   class SocketProtocolAddon : public SocketIOAddon {

      friend class SocketDevice;

      enum EProtocolEvents { evntSocketProtLast = evntSocketLast };

      protected:

         enum EState { stServerProto, stClientProto, stRedirect, stDone, stError };

         SocketDevice* fDevice;
         NewConnectRec* fRec;
         EState fState;
         char fInBuf[SocketDevice::ProtocolMsgSize];
         char fOutBuf[SocketDevice::ProtocolMsgSize];
      public:

         SocketProtocolAddon(int connfd, SocketDevice* dev, NewConnectRec* rec, void* redirect = 0) :
            dabc::SocketIOAddon(connfd),
            fDevice(dev),
            fRec(rec),
            fState(rec==0 ? stServerProto : stClientProto)
         {
            if (redirect!=0) {
               fState = stRedirect;
               memcpy(fInBuf, redirect, SocketDevice::ProtocolMsgSize); // as it was received
            }
         }

         virtual ~SocketProtocolAddon()
         {
         }

         void FinishWork(bool res)
         {
            fState = res ? stDone : stError;
            fDevice->RemoveProtocolAddon(this, res);
            DeleteWorker();
         }

         virtual void OnSocketError(int, const std::string&)
         {
            FinishWork(false);
         }

         virtual void OnThreadAssigned()
         {
            dabc::SocketIOAddon::OnThreadAssigned();

            uint32_t header = SocketDevice::headerConnect;

            switch (fState) {
               case stServerProto:
                  StartRecv(fInBuf, SocketDevice::ProtocolMsgSize);
                  break;
               case stClientProto:
                  // we can start both send and recv operations simultaneously,
                  // while buffer will be received only when server answer on request

                  memcpy(fOutBuf, &header, sizeof(header));
                  strcpy(fOutBuf + sizeof(header), fRec->ConnId());
                  strcpy(fInBuf, "denied");

                  StartSend(fOutBuf, SocketDevice::ProtocolMsgSize);
                  StartRecv(fInBuf, SocketDevice::ProtocolMsgSize);
                  break;
               case stRedirect:
                  // do like we receive input buffer ourself
                  fDevice->ServerProtocolRequest(this, fInBuf, fOutBuf);
                  StartSend(fOutBuf, SocketDevice::ProtocolMsgSize);
                  break;
               default:
                  EOUT("Wrong state %d", fState);
                  FinishWork(false);
            }
         }

         virtual void OnSendCompleted()
         {
            switch (fState) {
               case stServerProto:
               case stRedirect:
                  // DOUT5("Server job finished");
                  if (fDevice->ProtocolCompleted(this, 0))
                     DeleteWorker();
                  break;
               case stClientProto:
                  // DOUT5("Client send request, wait reply");
                  break;
               default:
                  EOUT("Wrong state %d", fState);
                  FinishWork(false);
            }
         }

         virtual void OnRecvCompleted()
         {
            switch (fState) {
               case stServerProto:
                  fDevice->ServerProtocolRequest(this, fInBuf, fOutBuf);
                  StartSend(fOutBuf, SocketDevice::ProtocolMsgSize);
                  break;
               case stClientProto:
                  DOUT5("Client job finished");
                  if (fDevice->ProtocolCompleted(this, fInBuf))
                     DeleteWorker();
                  break;
               default:
                  EOUT("Wrong state %d", fState);
                  FinishWork(false);
            }
         }
   };

}

// ______________________________________________________________________

dabc::SocketDevice::SocketDevice(const std::string &name, Command cmd) :
   dabc::Device(name),
   fConnRecs(),
   fProtocols(),
   fConnCounter(0),
   fCmdChannelId()
{
   fBindHost = Cfg("host", cmd).AsStr();
   fBindPort = Cfg("port", cmd).AsInt(-1);

   if (fBindHost.empty() && (fBindPort<0)) {
      dabc::WorkerRef chl = dabc::mgr.GetCommandChannel();
      if (!chl.null()) {
         dabc::Command cmd("RedirectSocketConnect");
         cmd.SetStr("Device", ItemName());
         if (chl.Execute(cmd)) fCmdChannelId = cmd.GetStr("ServerId");
      }

      if (!fCmdChannelId.empty()) DOUT0("Socket device %s reuses %s for connections", GetName(), fCmdChannelId.c_str());
   }

   if (fCmdChannelId.empty() && fBindHost.empty())
      fBindHost = dabc::Configuration::GetLocalHost();
}

dabc::SocketDevice::~SocketDevice()
{
   // FIXME: cleanup should be done much earlier

   CleanupRecs(-1);

   while (fProtocols.size()>0) {
      SocketProtocolAddon* pr = (SocketProtocolAddon*) fProtocols[0];
      fProtocols.remove_at(0);
      dabc::Object::Destroy(pr);
   }

}

std::string dabc::SocketDevice::StartServerAddon()
{
   // in standard case use server socket of command channel
   if (!fCmdChannelId.empty()) return fCmdChannelId;

   SocketServerAddon* serv = dynamic_cast<SocketServerAddon*> (fAddon());

   if (serv == 0) {

      int port0 = fBindPort, portmin(7000), portmax(9000);
      if (port0 > 0) portmin = portmax = 0;

      serv = dabc::SocketThread::CreateServerAddon(fBindHost, port0, portmin, portmax);

      DOUT0("SocketDevice creates server with ID %s", serv->ServerId().c_str());

      AssignAddon(serv);
   }

   if (!serv) return std::string();

   return serv->ServerId();
}


void dabc::SocketDevice::AddRec(NewConnectRec* rec)
{
   if (rec==0) return;

   bool firetmout = false;
   {
      LockGuard guard(DeviceMutex());
      fConnRecs.push_back(rec);
      firetmout = (fConnRecs.size() == 1);
   }
   if (firetmout) ActivateTimeout(0.);
}

void dabc::SocketDevice::DestroyRec(NewConnectRec* rec, bool res)
{
   if (rec==0) return;
   if (rec->fClient) EOUT("Is client %p is destroyed?", rec->fClient);
   if (rec->fProtocol) EOUT("Is protocol %p is destroyed?", rec->fProtocol);

   rec->fLocalCmd.ReplyBool(res);

   delete rec;
}

dabc::NewConnectRec* dabc::SocketDevice::_FindRec(const char* connid)
{
   for (unsigned n=0; n<fConnRecs.size();n++) {
      NewConnectRec* rec = (NewConnectRec*) fConnRecs.at(n);

      if (rec->IsConnId(connid)) return rec;
   }

   return 0;
}

bool dabc::SocketDevice::CleanupRecs(double tmout)
{
   PointersVector del_recs;

   bool more_timeout = false;

   {
      LockGuard guard(DeviceMutex());

      unsigned n = 0;

      while (n<fConnRecs.size()) {
         NewConnectRec* rec = (NewConnectRec*) fConnRecs.at(n);
         rec->fTmOut -= tmout;

         if ((rec->fTmOut<0) || (tmout<0)) {
            if (tmout>0) EOUT("Record %u timedout", n);
            fConnRecs.remove_at(n);
            del_recs.push_back(rec);
         } else
            n++;
      }

      more_timeout = fConnRecs.size() > 0;
   }

   for (unsigned n=0;n<del_recs.size();n++)
      DestroyRec((NewConnectRec*) del_recs.at(n), false);

   return more_timeout;
}

double dabc::SocketDevice::ProcessTimeout(double last_diff)
{
   return CleanupRecs(last_diff) ? SocketServerTmout : -1;
}

int dabc::SocketDevice::HandleManagerConnectionRequest(Command cmd)
{
   std::string reqitem = cmd.GetStr(CmdConnectionManagerHandle::ReqArg());

   dabc::ConnectionRequestFull req = dabc::mgr.FindPar(reqitem);

   if (req.null()) return cmd_false;

   switch (req.progress()) {

      // here on initializes connection
      case ConnectionManager::progrDoingInit: {
         if (req.IsServerSide()) {
            std::string serverid = StartServerAddon();
            if (serverid.empty()) return cmd_false;
            req.SetServerId(serverid);
         } else
            req.SetClientId("");

         break;
      }

      case ConnectionManager::progrDoingConnect: {
         // one should register request and start connection here

         DOUT2("****** SOCKET START: %s %s CONN: %s *******", (req.IsServerSide() ? "SERVER" : "CLIENT"), req.GetConnId().c_str(), req.GetConnInfo().c_str());

         NewConnectRec* rec = 0;

         if (req.IsServerSide()) {

            rec = new NewConnectRec(reqitem, req, 0);

            AddRec(rec);
         } else {

            SocketClientAddon* client = dabc::SocketThread::CreateClientAddon(req.GetServerId());
            if (client!=0) {

               // try to make little bit faster than timeout expire why we need
               // some time for the connection protocol
               client->SetRetryOpt(5, req.GetConnTimeout());
               client->SetConnHandler(this, req.GetConnId());

               rec = new NewConnectRec(reqitem, req, client);
               AddRec(rec);

               thread().MakeWorkerFor(client);
            }
         }

         // reply remote command that one other side can start connection

         req.ReplyRemoteCommand(rec!=0);

         if (rec==0) return cmd_false;

         rec->fLocalCmd = cmd;

         return cmd_postponed;
      }

      default:
         EOUT("Request from connection manager in undefined situation progress =  %d ???", req.progress());
         break;
   }

   return cmd_true;
}


int dabc::SocketDevice::ExecuteCommand(Command cmd)
{
   int cmd_res = cmd_true;

   if (cmd.IsName(CmdConnectionManagerHandle::CmdName())) {

      cmd_res = HandleManagerConnectionRequest(cmd);

   } else

   if (cmd.IsName("SocketConnect")) {
      std::string typ = cmd.GetStr("Type");
      std::string connid = cmd.GetStr("ConnId");
      int fd = cmd.GetInt("fd", -1);

      if (typ == "Server") {
         DOUT2("SocketDevice:: create server protocol for socket %d connid %s", fd, connid.c_str());

         SocketProtocolAddon* proto = new SocketProtocolAddon(fd, this, 0);

         thread().MakeWorkerFor(proto, connid);

         LockGuard guard(DeviceMutex());
         fProtocols.push_back(proto);
      } else
      if (typ == "Client") {
         SocketProtocolAddon* proto = 0;

         {
            LockGuard guard(DeviceMutex());
            NewConnectRec* rec = _FindRec(connid.c_str());
            if (rec==0) {
               EOUT("Client connected for not exiting rec %s", connid.c_str());
               close(fd);
               cmd_res = cmd_false;
            } else {
               DOUT2("SocketDevice:: create client protocol for socket %d connid:%s", fd, connid.c_str());

               proto = new SocketProtocolAddon(fd, this, rec);
               rec->fClient = 0; // if we get command, client is destroyed
               rec->fProtocol = proto;
            }
         }
         if (proto) thread().MakeWorkerFor(proto, connid);
      } else
      if (typ == "Error") {
         NewConnectRec* rec = 0;
         {
            LockGuard guard(DeviceMutex());
            rec = _FindRec(connid.c_str());
            if (rec==0) {
               EOUT("Client error for not existing rec %s", connid.c_str());
               cmd_res = cmd_false;
            } else {
               EOUT("Client error for connid %s", connid.c_str());
               rec->fClient = 0; // if we get command, client is destroyed
               fConnRecs.remove(rec);
            }
         }

         if (rec) DestroyRec(rec, false);
      } else
         cmd_res = cmd_false;
   } else
   if (cmd.IsName("RedirectConnect")) {
      int fd = cmd.GetInt("Socket");
      Buffer buf = cmd.GetRawData();

      SocketProtocolAddon* proto = new SocketProtocolAddon(fd, this, 0, buf.SegmentPtr());

      thread().MakeWorkerFor(proto, fCmdChannelId);

      LockGuard guard(DeviceMutex());
      fProtocols.push_back(proto);

      cmd_res = cmd_true;

   } else {
      cmd_res = dabc::Device::ExecuteCommand(cmd);
   }

   return cmd_res;
}

void dabc::SocketDevice::ServerProtocolRequest(SocketProtocolAddon* proc, const char* inmsg, char* outmsg)
{
   strcpy(outmsg, "denied");

   uint32_t *header = (uint32_t*) inmsg;
   if (*header != SocketDevice::headerConnect) {
      EOUT("Wrong header identifier in the SOCKET connect");
      return;
   }

   NewConnectRec* rec = 0;

   {
      LockGuard guard(DeviceMutex());
      rec = _FindRec(inmsg+sizeof(uint32_t));
      if (rec==0) return;
   }

   strcpy(outmsg, "accepted");

   LockGuard guard(DeviceMutex());
   fProtocols.remove(proc);
   rec->fProtocol = proc;
   proc->fRec = rec;
}

bool dabc::SocketDevice::ProtocolCompleted(SocketProtocolAddon* proc, const char* inmsg)
{
   NewConnectRec* rec = proc->fRec;

   bool destr = false;

   {
      LockGuard guard(DeviceMutex());
      if ((rec==0) || !fConnRecs.has_ptr(rec)) {
         EOUT("Protocol completed without rec %p", rec);
         fProtocols.remove(proc);
         destr = true;
      }
   }

   if (destr) return true;

   bool res = true;
   if (inmsg) res = (strcmp(inmsg, "accepted")==0);

   if (inmsg) DOUT3("Reply from server: %s", inmsg);

   if (res) {
      // create transport for the established connection
      int fd = proc->TakeSocket();

      ConnectionRequestFull req = dabc::mgr.FindPar(rec->fReqItem);

      SocketNetworkInetrface* addon = new SocketNetworkInetrface(fd);

      res = dabc::NetworkTransport::Make(req, addon, ThreadName());

      DOUT0("Create socket transport for fd %d res %s", fd, DBOOL(res));
   }

   RemoveProtocolAddon(proc, res);

   return true;
}

void dabc::SocketDevice::RemoveProtocolAddon(SocketProtocolAddon* proc, bool res)
{
   if (proc==0) return;

   NewConnectRec* rec = proc->fRec;

   {
      LockGuard guard(DeviceMutex());
      if (rec!=0) {
         fConnRecs.remove(rec);
         rec->fProtocol = 0;
      } else
         fProtocols.remove(proc);
   }

   DestroyRec(rec, res);
}
