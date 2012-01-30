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

#include "dabc/SocketTransport.h"

#include "dabc/Manager.h"
#include "dabc/Port.h"
#include "dabc/Configuration.h"
#include "dabc/ConnectionManager.h"

#define SocketServerTmout 0.2

// this is fixed-size message for exchanging during protocol execution
#define ProtocolMsgSize 100

namespace dabc {

   class SocketProtocolWorker;

   // TODO: Can we use here connection manager name - all information already there

   class NewConnectRec {
      public:
         std::string            fReqItem;    //!< reference in connection request
         SocketClientWorker*    fClient;     //!< client-side processor, to establish connection
         SocketProtocolWorker*  fProtocol;   //!< protocol processor, to verify connection id
         double                 fTmOut;      //!< used by device to process connection timeouts
         std::string            fConnId;     //! connection id
         Command                fLocalCmd;   //!< command from connection manager which should be replied when connection established or failed

         NewConnectRec(const std::string& item,
                       ConnectionRequestFull& req,
                       SocketClientWorker* clnt) :
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

   class SocketProtocolWorker : public SocketIOWorker {

      friend class SocketDevice;

      enum EProtocolEvents { evntSocketProtLast = evntSocketLast };

      protected:

         enum EState { stServerProto, stClientProto, stDone, stError };

         SocketDevice* fDevice;
         NewConnectRec* fRec;
         EState fState;
         char fInBuf[ProtocolMsgSize];
         char fOutBuf[ProtocolMsgSize];
      public:

         SocketProtocolWorker(int connfd, SocketDevice* dev, NewConnectRec* rec) :
            dabc::SocketIOWorker(connfd),
            fDevice(dev),
            fRec(rec),
            fState(rec==0 ? stServerProto : stClientProto)
         {
         }

         virtual ~SocketProtocolWorker()
         {
         }

         void FinishWork(bool res)
         {
            fState = res ? stDone : stError;
            fDevice->DestroyProtocolWorker(this, res);
         }

         void OnConnectionClosed()
         {
            FinishWork(false);
         }

         void OnSocketError(int errnum, const char* info)
         {
            FinishWork(false);
         }

         virtual void OnThreadAssigned()
         {
            switch (fState) {
               case stServerProto:
                  StartRecv(fInBuf, ProtocolMsgSize);
                  break;
               case stClientProto:
                  // we can start both send and recv operations simultaneously,
                  // while buffer will be received only when server answer on request
                  strcpy(fOutBuf, fRec->ConnId());
                  strcpy(fInBuf, "denied");

                  StartSend(fOutBuf, ProtocolMsgSize);
                  StartRecv(fInBuf, ProtocolMsgSize);
                  break;
               default:
                  EOUT(("Wrong state %d", fState));
                  FinishWork(false);
            }
         }

         virtual void OnSendCompleted()
         {
            switch (fState) {
               case stServerProto:
                  DOUT5(("Server job finished"));
                  fDevice->ProtocolCompleted(this, 0);
                  break;
               case stClientProto:
                  DOUT5(("Client send request, wait reply"));
                  break;
               default:
                  EOUT(("Wrong state %d", fState));
                  FinishWork(false);
            }
         }

         virtual void OnRecvCompleted()
         {
            switch (fState) {
               case stServerProto:
                  fDevice->ServerProtocolRequest(this, fInBuf, fOutBuf);
                  StartSend(fOutBuf, ProtocolMsgSize);
                  break;
               case stClientProto:
                  DOUT5(("Client job finished"));
                  fDevice->ProtocolCompleted(this, fInBuf);
                  break;
               default:
                  EOUT(("Wrong state %d", fState));
                  FinishWork(false);
            }
         }
   };

}

// ______________________________________________________________________

dabc::SocketDevice::SocketDevice(const char* name) :
   dabc::Device(name),
   fServer(0),
   fConnRecs(),
   fConnCounter(0)
{
   DOUT5(("Start SocketDevice constructor"));

   dabc::mgr()->MakeThreadFor(this, GetName());

   DOUT5(("Did SocketDevice constructor"));
}

dabc::SocketDevice::~SocketDevice()
{
   // FIXME: cleanup should be done much earlier

   CleanupRecs(-1);

   while (fProtocols.size()>0) {
      SocketProtocolWorker* pr = (SocketProtocolWorker*) fProtocols[0];
      fProtocols.remove_at(0);
      dabc::Object::Destroy(pr);
   }

   SocketServerWorker* serv = 0;
   {
      LockGuard guard(DeviceMutex());
      serv = fServer;
      fServer = 0;
   }
   dabc::Object::Destroy(serv);
}

bool dabc::SocketDevice::StartServerWorker(Command cmd, std::string& servid)
{
   if (fServer==0) {

      int port0(-1), portmin(7000), portmax(9000);

      if (!cmd.null()) {
         port0 = cmd.GetInt("SocketPort", port0);
         portmin = cmd.GetInt("SocketRangeMin", portmin);
         portmax = cmd.GetInt("SocketRangeMax", portmax);
      }

      fServer = dabc::SocketThread::CreateServerWorker(port0, portmin, portmax);

      if (fServer==0) return false;

      fServer->SetConnHandler(this, "---"); // we have no id for the connection
      fServer->AssignToThread(thread());
   }

   servid = fServer->ServerId();

   return true;
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
   dabc::Object::Destroy(rec->fClient);
   dabc::Object::Destroy(rec->fProtocol);

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
            if (tmout>0) EOUT(("Record %p timedout", n));
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
   std::string reqitem = cmd.GetStdStr(ConnectionManagerHandleCmd::ReqArg());

   dabc::ConnectionRequestFull req = dabc::mgr.FindPar(reqitem);

   if (req.null()) return cmd_false;

   switch (req.progress()) {

      // here on initializes connection
      case ConnectionManager::progrDoingInit: {
         if (req.IsServerSide()) {
            std::string serverid;
            if (!StartServerWorker(0, serverid)) return cmd_false;
            req.SetServerId(serverid);
         } else
            req.SetClientId("");

         break;
      }

      case ConnectionManager::progrDoingConnect: {
         // one should register request and start connection here

         DOUT2(("****** SOCKET START: %s %s CONN: %s *******", (req.IsServerSide() ? "SERVER" : "CLIENT"), req.GetConnId().c_str(), req.GetConnInfo().c_str()));

         NewConnectRec* rec = 0;

         if (req.IsServerSide()) {

            rec = new NewConnectRec(reqitem, req, 0);

            AddRec(rec);
         } else {

            SocketClientWorker* client = dabc::SocketThread::CreateClientWorker(req.GetServerId().c_str());
            if (client!=0) {

               // try to make little bit faster than timeout expire why we need
               // some time for the connection protocol
               client->SetRetryOpt(req.GetConnTimeout(), 0.9);
               client->SetConnHandler(this, req.GetConnId().c_str());

               rec = new NewConnectRec(reqitem, req, client);
               AddRec(rec);

               client->AssignToThread(thread());
            }
         }

         // reply remote command that one other side can start connection

         req.ReplyRemoteCommand(rec!=0);

         if (rec==0) return cmd_false;

         rec->fLocalCmd = cmd;

         return cmd_postponed;

         break;
      }

      default:
         EOUT(("Request from connection manager in undefined situation progress =  %d ???", req.progress()));
         break;
   }

   return cmd_true;
}


int dabc::SocketDevice::ExecuteCommand(Command cmd)
{
   int cmd_res = cmd_true;

   if (cmd.IsName(ConnectionManagerHandleCmd::CmdName())) {

      cmd_res = HandleManagerConnectionRequest(cmd);

   } else

   if (cmd.IsName("SocketConnect")) {
      const char* typ = cmd.GetStr("Type");
      const char* connid = cmd.GetStr("ConnId");
      int fd = cmd.GetInt("fd", -1);

      if (strcmp(typ, "Server")==0) {
         DOUT2(("SocketDevice:: create server protocol for socket %d connid %s", fd, connid));

         SocketProtocolWorker* proto = new SocketProtocolWorker(fd, this, 0);
         proto->AssignToThread(thread());

         LockGuard guard(DeviceMutex());
         fProtocols.push_back(proto);
      } else
      if (strcmp(typ, "Client")==0) {
         SocketProtocolWorker* proto = 0;

         {
            LockGuard guard(DeviceMutex());
            NewConnectRec* rec = _FindRec(connid);
            if (rec==0) {
               EOUT(("Client connected for not exiting rec %s", connid));
               close(fd);
               cmd_res = cmd_false;
            } else {
               DOUT2(("SocketDevice:: create client protocol for socket %d connid:%s", fd, connid));

               proto = new SocketProtocolWorker(fd, this, rec);
               rec->fClient = 0; // if we get command, client is destroyed
               rec->fProtocol = proto;
            }
         }
         if (proto) proto->AssignToThread(thread());
      } else
      if (strcmp(typ, "Error")==0) {
         NewConnectRec* rec = 0;
         {
            LockGuard guard(DeviceMutex());
            rec = _FindRec(connid);
            if (rec==0) {
               EOUT(("Client error for not existing rec %s", connid));
               cmd_res = cmd_false;
            } else {
               EOUT(("Client error for connid %s", connid));
               rec->fClient = 0;
               fConnRecs.remove(rec);
            }
         }

         if (rec) DestroyRec(rec, false);
      } else
         cmd_res = cmd_false;
   } else
      cmd_res = dabc::Device::ExecuteCommand(cmd);

   return cmd_res;
}

void dabc::SocketDevice::ServerProtocolRequest(SocketProtocolWorker* proc, const char* inmsg, char* outmsg)
{
   strcpy(outmsg, "denied");

   NewConnectRec* rec = 0;

   {
      LockGuard guard(DeviceMutex());
      rec = _FindRec(inmsg);
      if (rec==0) return;
   }

   strcpy(outmsg, "accepted");

   LockGuard guard(DeviceMutex());
   fProtocols.remove(proc);
   rec->fProtocol = proc;
   proc->fRec = rec;

}

void dabc::SocketDevice::ProtocolCompleted(SocketProtocolWorker* proc, const char* inmsg)
{
   NewConnectRec* rec = proc->fRec;

   bool destr = false;

   {
      LockGuard guard(DeviceMutex());
      if ((rec==0) || !fConnRecs.has_ptr(rec)) {
         EOUT(("Protocol completed without rec"));
         fProtocols.remove(proc);
         destr = true;
      }
   }

   if (destr) {
      dabc::Object::Destroy(proc);
      return;
   }

   bool res = true;
   if (inmsg) res = (strcmp(inmsg, "accepted")==0);

   if (inmsg) DOUT3(("Reply from server: %s", inmsg));

   if (res) {

      // create transport for the established connection
      int fd = proc->TakeSocket();

      ConnectionRequestFull req = dabc::mgr.FindPar(rec->fReqItem);

      PortRef port = req.GetPort();

      if (req.null() || port.null()) {
         EOUT(("Port or request disappear while connection is ready"));
         close(fd);
         res = false;
      } else {

         // FIXME: ConnectionRequest should be used
         std::string newthrdname = req.GetConnThread();
         if (newthrdname.empty()) newthrdname = ThreadName();

         DOUT2(("SocketDevice::ProtocolCompleted conn %s thrd %s useakn %s", rec->ConnId(), newthrdname.c_str(), DBOOL(req.GetUseAckn())));

         SocketTransport* tr = new SocketTransport(port(), req.GetUseAckn(), fd);
         Reference handle(tr);

         if (dabc::mgr()->MakeThreadFor(tr, newthrdname.c_str()))
            port()->AssignTransport(handle, tr);
         else {
            EOUT(("No thread for transport"));
            handle.Destroy();
            res = false;
         }
      }
   }

   DestroyProtocolWorker(proc, res);
}

void dabc::SocketDevice::DestroyProtocolWorker(SocketProtocolWorker* proc, bool res)
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

   dabc::Object::Destroy(proc);

   DestroyRec(rec, res);
}

dabc::Transport* dabc::SocketDevice::CreateTransport(Command cmd, Reference portref)
{
   Port* port = (Port*) portref();
   if (port==0) return 0;

   const char* portname = cmd.GetField("PortName");

   std::string mhost = port->Cfg(xmlMcastAddr, cmd).AsStdStr();

   if (!mhost.empty()) {
      int mport = port->Cfg(xmlMcastPort,cmd).AsInt(7654);
      bool mrecv = port->Cfg(xmlMcastRecv,cmd).AsBool(port->InputQueueCapacity() > 0);

      if (mrecv && (port->InputQueueCapacity()==0)) {
         EOUT(("Wrong Multicast configuration - port %s cannot recv packets", portname));
         return 0;
      } else
      if (!mrecv && (port->OutputQueueCapacity()==0)) {
         EOUT(("Wrong Multicast configuration - port %s cannot send packets", portname));
         return 0;
      }

      int fhandle = dabc::SocketThread::StartMulticast(mhost.c_str(), mport, mrecv);
      if (fhandle<0) return 0;

      if (!dabc::SocketThread::SetNonBlockSocket(fhandle)) {
         dabc::SocketThread::CloseMulticast(fhandle, mhost.c_str(), mrecv);
         return 0;
      }

      return new SocketTransport(port, false, fhandle, true);

   }

   return 0;
}

std::string dabc::SocketDevice::GetLocalHost(bool force)
{
   std::string host = dabc::Configuration::GetLocalHost();
   if (host.empty() && force) {
      char sbuf[500];
      if (gethostname(sbuf, sizeof(sbuf))) {
         EOUT(("Error to get local host name"));
         host = "localhost";
      } else
         host = sbuf;
   }

   return host;
}
