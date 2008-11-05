#include "dabc/SocketDevice.h"

#include "dabc/SocketTransport.h"

#include "dabc/Manager.h"
#include "dabc/Port.h"
#include "dabc/CommandClient.h"


#define SocketServerTmout 0.2

// this is fixed-size message for exhanging during protocol execution
#define ProtocolMsgSize 100
#define ProtocolCmdHeader "Command"

namespace dabc {

   class SocketProtocolProcessor;
   
   class NewConnectRec {
      public:
         NewConnectRec(Command* cmd,
                       const char* portname, 
                       SocketClientProcessor* clnt,
                       const char* connid,
                       double tmout) :
            fCmd(cmd),
            fPortName(portname),
            fClient(clnt),
            fProtocol(0),
            fConnId(connid),
            fTmOut(tmout),
            fCmdStrBuf(),
            fThreadName()
         {
         }
         
         Command* fCmd;
         String   fPortName; // this full port name must be used later to assign transport
         SocketClientProcessor* fClient; 
         SocketProtocolProcessor*  fProtocol; 
         String   fConnId;
         double   fTmOut; // used by device to process connection timeouts
         String   fCmdStrBuf; // buffer, which contains converted to string command
         String   fThreadName; // name of thread for transport
         
         const char* ConnId() const { return fConnId.c_str(); }
         
         bool IsRemoteCommand() { return fPortName.length()==0; }
   };

   // this class is used to perform initial protocol 
   // when socket connection is established
   // it also used to transport commands on remote side and execute them
   
   class SocketProtocolProcessor : public SocketIOProcessor,
                                public CommandClientBase {
      
      friend class SocketDevice;
      
      enum EProtocolEvents { evntProtocolReplyCmd = evntSocketLast };
      
      protected:
      
         enum EStatus { stServerProto, stClientProto, stSendCmd, stRecvCmd, stWaitCmdReply, stSendReplySize, stRecvReplySize, stSendReply, stRecvReply, stDone, stError };
      
         SocketDevice* fDevice;
         NewConnectRec* fRec;
         EStatus fStatus; 
         char fInBuf[ProtocolMsgSize];
         char fOutBuf[ProtocolMsgSize];
         char* fCmdBuf; // buffer for input/output of cmd buffer
      public:
      
         SocketProtocolProcessor(int connfd, SocketDevice* dev, NewConnectRec* rec) :
            dabc::SocketIOProcessor(connfd),
            CommandClientBase(),
            fDevice(dev),
            fRec(rec),
            fStatus(rec==0 ? stServerProto : stClientProto),
            fCmdBuf(0)
         {
         }
         
         virtual ~SocketProtocolProcessor() 
         {
            if (fCmdBuf) delete [] fCmdBuf; 
            fCmdBuf = 0;
         }
         
         void FinishWork(bool res)
         {
            fStatus = res ? stDone : stError;
            fDevice->DestroyProcessor(this, res);
         }
         
         bool _ProcessReply(Command* cmd)
         {
            if ((fRec==0) || (fRec->fCmd!=cmd)) return false;
            
            FireEvent(evntProtocolReplyCmd);
            
            return true;
         }
         
         void StartCmdBufRecv(int sz)
         {
             delete[] fCmdBuf;
      
             fCmdBuf = new char[sz];
      
             StartRecv(fCmdBuf, sz);
         }

         void StartCmdBufSend(String& sbuf)
         {
             int sz = sbuf.length() + 1;
             delete[] fCmdBuf;
             fCmdBuf = new char[sz];
             strcpy(fCmdBuf, sbuf.c_str());
             StartSend(fCmdBuf, sz);
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
            switch (fStatus) {
               case stServerProto:
                  StartRecv(fInBuf, ProtocolMsgSize);
                  break;
               case stClientProto:
                  // we can start both send and recv operations simultaniousely,
                  // while buffer will be received only when server answer on request
                  strcpy(fOutBuf, fRec->ConnId());
                  strcpy(fInBuf, "denied");

                  StartSend(fOutBuf, ProtocolMsgSize); 
                  StartRecv(fInBuf, ProtocolMsgSize);
                  break;
               default:
                  EOUT(("Wrong status %d", fStatus));
                  FinishWork(false);
            }
         }
         
         virtual void OnSendCompleted()
         {
            switch (fStatus) {
               case stServerProto:
                  DOUT5(("Server job finished"));
                  fDevice->ProtocolCompleted(this, 0);
                  break;
               case stClientProto:
                  DOUT5(("Client send request, wait reply"));
                  break;   
               case stSendCmd:
                  DOUT5(("Command send, recv reply size"));
                  StartRecv(fInBuf, ProtocolMsgSize);
                  fStatus = stRecvReplySize;
                  break;
               case stSendReplySize: 
                  StartCmdBufSend(fRec->fCmdStrBuf);
                  fStatus = stSendReply;
                  break;
               case stSendReply: 
                  FinishWork(true);
                  break;  
               default:
                  EOUT(("Wrong status %d", fStatus));
                  FinishWork(false);
            }
         }
      
         virtual void OnRecvCompleted()
         {
            switch (fStatus) {
               case stServerProto:
                  fDevice->ServerProtocolRequest(this, fInBuf, fOutBuf);
                  StartSend(fOutBuf, ProtocolMsgSize);
                  break;
               case stClientProto:   
                  DOUT5(("Client job finished"));
                  fDevice->ProtocolCompleted(this, fInBuf);
                  break;
               case stRecvCmd:
                  if (fDevice->SubmitCommandFromRemote(this, fCmdBuf))
                     fStatus = stWaitCmdReply;
                  else
                     FinishWork(false); 
                  break;
               case stRecvReplySize: {
               
                  DOUT5(("Get cmd reply size %s", fInBuf));
                  
                  long reply_sz = -1;
                  sscanf(fInBuf,"%ld", &reply_sz);
                  
                  if (reply_sz<=0) {
                     EOUT(("when receiving cmd size"));
                     FinishWork(false);
                     return;  
                  }
                  
                  StartCmdBufRecv(reply_sz);
                  
                  fStatus = stRecvReply;
                  
                  break;
               }
               case stRecvReply: {
                  DOUT5(("Get cmd reply %s", fCmdBuf));

                  bool res = fDevice->RemoteCommandReplyed(this, fCmdBuf);

                  FinishWork(res);
                  
                  break;  
               }
                  
               default:
                  EOUT(("Wrong status %d", fStatus));
                  FinishWork(false);
            }
         }
         
         bool StartRemoteCommandJob()
         {
            if (fRec==0) {
               EOUT(("No record for command"));
               return false;
            }

            switch (fStatus) {
               case stServerProto: 
                  // we already submit recv, just wait for completions
                  fStatus = stRecvCmd;
                  break;
               
               case stClientProto: 
                  StartCmdBufSend(fRec->fCmdStrBuf);
                  fStatus = stSendCmd;
                  break;
               
               default:
                  EOUT(("Wrong status %d", fStatus));
                  return false;
            }
            
            return true;
         }
         
         virtual void ProcessEvent(dabc::EventId evnt)
         {
            switch (GetEventCode(evnt)) {
               case evntProtocolReplyCmd: {
                  if (fStatus!=stWaitCmdReply) {
                     EOUT(("Internal problem"));
                     FinishWork(false);
                     return;
                  }
                  
                  fRec->fCmd->SaveToString(fRec->fCmdStrBuf);
                  
                  dabc::Command::Finalise(fRec->fCmd);
                  
                  fRec->fCmd = 0;
                  
                  unsigned cmdlen = fRec->fCmdStrBuf.length() + 1;
                  
                  snprintf(fOutBuf, sizeof(fOutBuf), "%u", cmdlen);
                  
                  fStatus = stSendReplySize;
                  
                  StartSend(fOutBuf, ProtocolMsgSize);
                  
                  break;
               }
       
               default:
                 SocketIOProcessor::ProcessEvent(evnt);
            }
         }
   };
}

// ______________________________________________________________________

dabc::SocketDevice::SocketDevice(Basic* parent, const char* name) : 
   dabc::Device(parent, name),
   fServer(0),
   fServerCmdChannel("channel"),
   fConnRecs(),
   fConnCounter(0)
{
   DOUT5(("Start SocketDevice constructor")); 
    
   Manager::Instance()->MakeThreadFor(this, GetName()); 
    
   DOUT5(("Did SocketDevice constructor")); 
}

dabc::SocketDevice::~SocketDevice()
{
   CleanupRecs(-1);

   while (fProtocols.size()>0) {
      SocketProtocolProcessor* pr = (SocketProtocolProcessor*) fProtocols[0];
      fProtocols.remove_at(0);
      pr->DestroyProcessor();
   }   
   
   LockGuard guard(DeviceMutex());
   delete fServer; 
   fServer = 0;
}

bool dabc::SocketDevice::StartServerThread(Command* cmd, String& servid, const char* cmdchannel)
{
   if (fServer==0) {
      SocketServerProcessor* new_serv = 
         dabc::SocketThread::CreateServerProcessor(
            cmd->GetInt("SocketPort", -1), 
            cmd->GetInt("SocketRangeMin",7000), 
            cmd->GetInt("SocketRangeMax", 9000));
      if (new_serv==0) return false;
      new_serv->SetConnHandler(this, "---"); // we have no id for the connection
      new_serv->AssignProcessorToThread(ProcessorThread());
     
      {
         LockGuard guard(DeviceMutex());
         if (fServer==0) {
            if (cmdchannel!=0) {
               fServerCmdChannel = cmdchannel;
               DOUT1(("Set command channel %s", cmdchannel));
            }
            
            fServer = new_serv;
            new_serv = 0;
         }
      }
     
      if (new_serv!=0) {
         EOUT(("Cannot use server - other is running"));
         delete new_serv;  
      }
   }

   LockGuard guard(DeviceMutex());
   
   servid = fServer->ServerId();

   return true;
}
         
bool dabc::SocketDevice::ServerConnect(Command* cmd, Port* port, const char* portname)
{
   port = dabc::mgr()->FindPort(portname);
   DOUT3(("ServerConnect %s %p", portname, port));
   if (port==0) return false;

   String servid;   
   if (!StartServerThread(cmd, servid)) {
      EOUT(("Not started server thread %s", cmd->GetName())); 
      return false;
   }
   
   NewConnectRec* new_rec = 0;
   bool needreply = cmd->GetBool("ImmidiateReply", false);

   {
      LockGuard guard(DeviceMutex());
      
      String connid;
      if (cmd->GetPar("ConnId")==0)
         dabc::formats(connid, "%s-%d-%d", fServer->ServerHostName(), fServer->ServerPortNumber(), fConnCounter++);
      else    
         connid = cmd->GetPar("ConnId");

      cmd->SetPar("ServerId", servid.c_str());
      cmd->SetPar("ConnId", connid.c_str());
        
      cmd->SetBool("ServerUseAckn", port->IsUseAcknoledges());
      cmd->SetUInt("ServerHeaderSize", port->UserHeaderSize());
         
      int timeout = cmd->GetInt("Timeout", 10);
   
      new_rec = new NewConnectRec(needreply? 0 : cmd, portname, 0, connid.c_str(), timeout + SocketServerTmout);

      new_rec->fThreadName = cmd->GetStr("TrThread","");
   }
   
   if (needreply)
     dabc::Command::Reply(cmd, true);
     
   if (new_rec) AddRec(new_rec);
    
   return true;
}

bool dabc::SocketDevice::ClientConnect(Command* cmd, Port* port, const char* portname)
{
   port = dabc::mgr()->FindPort(portname);
   DOUT3(("ClientConnect %s %p", portname, port));
   if (port==0) return false;

   const char* serverid = cmd->GetPar("ServerId");

   SocketClientProcessor* client = dabc::SocketThread::CreateClientProcessor(serverid);
   if (client==0) return false;

   const char* connid = cmd->GetPar("ConnId");
   int timeout = cmd->GetInt("Timeout", 10);

   bool useackn = cmd->GetBool("ServerUseAckn", false);
   if (useackn != port->IsUseAcknoledges()) {
      EOUT(("Missmatch in acknowledges usage in ports server %s ispar %s connid %s cmd %s", 
           DBOOL(useackn), DBOOL(cmd->GetPar("ServerUseAckn")), connid, cmd->GetName())); 
      port->ChangeUseAcknoledges(useackn);
   }
      
   unsigned headersize = cmd->GetUInt("ServerHeaderSize", 0);
   if (headersize != port->UserHeaderSize()) {
      EOUT(("Missmatch in configured header sizes: %d %d", headersize, port->UserHeaderSize()));
      port->ChangeUserHeaderSize(headersize); 
   }
   
   // try to make little bit faster than timeout expire why we need 
   // some time for the connection protocol
   client->SetRetryOpt(timeout, 0.9);
   client->SetConnHandler(this, connid);

   NewConnectRec* rec = new NewConnectRec(cmd, portname, client, connid, timeout + SocketServerTmout);
   rec->fThreadName = cmd->GetStr("TrThread", "");

   AddRec(rec);
   
   client->AssignProcessorToThread(ProcessorThread());
   
   return true;
}

bool dabc::SocketDevice::SubmitRemoteCommand(const char* serverid, const char* channelid, Command* cmd)
{
   // this id containes channelid, command string length and local uniquie id
   String connid, scmd;
   
   cmd->SaveToString(scmd);
   dabc::formats(connid, "%s %s %d %d", ProtocolCmdHeader, channelid, scmd.length()+1, fConnCounter++);
   
   int timeout = 10;

   SocketClientProcessor* client = dabc::SocketThread::CreateClientProcessor(serverid);
   if (client==0) return false;
   
   client->SetRetryOpt(timeout, 0.9);
   client->SetConnHandler(this, connid.c_str());

   NewConnectRec* rec = new NewConnectRec(cmd, "", client, connid.c_str(), timeout + SocketServerTmout);
   rec->fCmdStrBuf = scmd;
   
   AddRec(rec);
   
   client->AssignProcessorToThread(ProcessorThread());

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
   if (rec->fClient) rec->fClient->DestroyProcessor();
   if (rec->fProtocol) rec->fProtocol->DestroyProcessor();
   dabc::Command::Reply(rec->fCmd, res);
   delete rec;
}

dabc::NewConnectRec* dabc::SocketDevice::_FindRec(const char* connid)
{
   for (unsigned n=0; n<fConnRecs.size();n++) {
      NewConnectRec* rec = (NewConnectRec*) fConnRecs.at(n);
      
      if (rec->fConnId.compare(connid)==0) return rec;
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

void dabc::SocketDevice::NewClientRequest(int connfd)
{
   // this method is called when server port object gets new connected client
   // we should check first if one expects connection or should just close connection
   
   SocketProtocolProcessor* proto = new SocketProtocolProcessor(connfd, this, 0);
   
   proto->AssignProcessorToThread(ProcessorThread());
   
   LockGuard guard(DeviceMutex());
   fProtocols.push_back(proto);   
}


int dabc::SocketDevice::ExecuteCommand(dabc::Command* cmd)
{
   int cmd_res = cmd_true;
   
   DOUT5(("Execute command %s", cmd->GetName()));
   
   if (cmd->IsName("StartServer")) {
      String servid;
      cmd_res = StartServerThread(cmd, servid, cmd->GetPar("CmdChannel"));
      cmd->SetPar("ConnId", servid.c_str());
   } else 
   if (cmd->IsName("SocketConnect")) {
      const char* typ = cmd->GetStr("Type");
      const char* connid = cmd->GetStr("ConnId");
      int fd = cmd->GetInt("fd", -1);
      
      if (strcmp(typ, "Server")==0) {
         DOUT3(("Create server protocol for socket %d", fd));
         
         SocketProtocolProcessor* proto = new SocketProtocolProcessor(fd, this, 0);
         proto->AssignProcessorToThread(ProcessorThread());
   
         LockGuard guard(DeviceMutex());
         fProtocols.push_back(proto);   
      } else
      if (strcmp(typ, "Client")==0) {
         LockGuard guard(DeviceMutex());
         NewConnectRec* rec = _FindRec(connid);
         if (rec==0) { 
            EOUT(("Client connected for not exiting rec %s", connid)); 
            close(fd);
            cmd_res = cmd_false;
         } else {

            DOUT3(("Create client protocol for socket %d connid:%s", fd, connid));
            
            SocketProtocolProcessor* proto = new SocketProtocolProcessor(fd, this, rec);
            rec->fClient = 0; // if we get command, client is detroyed
            rec->fProtocol = proto;
            proto->AssignProcessorToThread(ProcessorThread());
         }
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

void dabc::SocketDevice::ServerProtocolRequest(SocketProtocolProcessor* proc, const char* inmsg, char* outmsg)
{
   strcpy(outmsg, "denied");
   
   NewConnectRec* rec = 0;
   
   if (strncmp(inmsg, ProtocolCmdHeader, strlen(ProtocolCmdHeader))==0) {
      
      char buf1[ProtocolMsgSize], buf2[ProtocolMsgSize];
      long cmd_sz(0), cmd_id(0);
      int res = sscanf(inmsg, "%s %s %ld %ld",
                       buf1, buf2, &cmd_sz, &cmd_id);
                                  
      if ((res!=4) || (cmd_sz==0)) {
         EOUT(("Invalid command connection id %s", inmsg));
         return;
      }
      
      if ((fServerCmdChannel.length()>0) && (fServerCmdChannel.compare(buf2)!=0)) {
         EOUT(("Wrong command channel"));
         return;   
      }
      
      proc->StartCmdBufRecv(cmd_sz);
      
      int timeout = 10;
      rec = new NewConnectRec(0, "", 0, inmsg, timeout + SocketServerTmout);
      rec->fProtocol = proc;
      
      AddRec(rec);
   } else {
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

void dabc::SocketDevice::ProtocolCompleted(SocketProtocolProcessor* proc, const char* inmsg)
{
   NewConnectRec* rec = proc->fRec;
   
   {
      LockGuard guard(DeviceMutex());
      if ((rec==0) || !fConnRecs.has_ptr(rec)){
         EOUT(("Protocol completed without rec"));
         fProtocols.remove(proc);
         proc->DestroyProcessor();
         return;
      }
   }
   
   bool res = true;
   if (inmsg) res = (strcmp(inmsg, "accepted")==0);
   
   if (inmsg) DOUT3(("Reply from server: %s", inmsg));
   
   if (res) {
      
      if (rec->IsRemoteCommand()) {
         // start sending of remote command 
         if (proc->StartRemoteCommandJob()) return; 
         
         res = false;
         
      } else {

         // create transport for the established connection 
         int fd = proc->TakeSocket();
         
         Port* port = dabc::mgr()->FindPort(rec->fPortName.c_str());
         
         if (port==0) {
            EOUT(("Port dissappear while connection is ready"));
            close(fd);
            res = false;
         } else {
            
            const char* newthrdname = ProcessorThreadName();
            
            if (rec->fThreadName.length() > 0) 
               newthrdname = rec->fThreadName.c_str();
            
            DOUT1(("TRANSPORT %s thrd %s", rec->ConnId(), newthrdname));
            
            SocketTransport* tr = new SocketTransport(this, port, fd);
            if (Manager::Instance()->MakeThreadFor(tr, newthrdname))
               port->AssignTransport(tr);
            else {
               EOUT(("No thread for transport"));
               delete tr; 
               res = false;
            }
         }
      }
   }
   
   DestroyProcessor(proc, res);
}

bool dabc::SocketDevice::SubmitCommandFromRemote(SocketProtocolProcessor* proc, const char* scmd)
{
   NewConnectRec* rec = proc->fRec;
   
   if ((rec==0) || (rec->fCmd!=0)) {
      EOUT(("Internal error"));
      return false;  
   }
   
   Command* cmd = new Command;
   if (!cmd->ReadFromString(scmd)) {
      EOUT(("Cannot transform command from string"));
      dabc::Command::Finalise(cmd); 
      return false;
   }
   
   proc->Assign(cmd);
   
   rec->fCmd = cmd;
   
   dabc::mgr()->Submit(cmd);

   return true;
}

bool dabc::SocketDevice::RemoteCommandReplyed(SocketProtocolProcessor* proc, const char* scmd)
{
   NewConnectRec* rec = proc->fRec;
   if ((rec==0) || (rec->fCmd==0)) {
      EOUT(("Internal error"));
      return false;  
   }
   
   bool res = true;
   
   Command* rescmd = new Command;
   if (rescmd->ReadFromString(scmd)) {
      rec->fCmd->AddValuesFrom(rescmd);
      dabc::Command::Reply(rec->fCmd, rescmd->GetResult());
   } else {
      EOUT(("Cannot decode extren cmd: %s", scmd));
      dabc::Command::Reply(rec->fCmd, false);
      res = false;
   }
   dabc::Command::Finalise(rescmd);
   rec->fCmd = 0;
   
   return res;
}

void dabc::SocketDevice::DestroyProcessor(SocketProtocolProcessor* proc, bool res)
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
   
   proc->DestroyProcessor();
   
   DestroyRec(rec, res);   
}

int dabc::SocketDevice::CreateTransport(Command* cmd, Port* port)
{
   bool isserver = cmd->GetBool("IsServer", true);
   
   const char* portname = cmd->GetPar("PortName");
   
//   DOUT1(("dabc::SocketDevice::CreateTransport %s",portname));
   
   if (isserver ? ServerConnect(cmd, port, portname) : ClientConnect(cmd, port, portname))
      return cmd_postponed;
      
   return cmd_false;
}

char dabc::SocketDevice::fLocalHostIP[100] = "";

void dabc::SocketDevice::SetLocalHostIP(const char* ip)
{
   strncpy(fLocalHostIP, ip, sizeof(fLocalHostIP));
}

const char* dabc::SocketDevice::GetLocalHost()
{
   if (strlen(fLocalHostIP)==0)
      if (gethostname(fLocalHostIP, sizeof(fLocalHostIP))) {
         EOUT(("Error to get local host name"));
         strcpy(fLocalHostIP,"localhost");
      }
   
   return fLocalHostIP;   
}
