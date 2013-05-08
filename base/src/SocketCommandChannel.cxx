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
#include <list>

#include "dabc/timing.h"
#include "dabc/Manager.h"
#include "dabc/Application.h"
#include "dabc/Configuration.h"
#include "dabc/Url.h"

// TODO: All classes here should be factorized so that socket processor can be replaced by
//       the verbs processor and socket addresses replaced by verbs addresses
//       Thus, it should be possible to replace socket by IB in command channel

#define DABC_CMD_HEADER 0x1234567887654321ULL
#define DABC_APP_HEADER 0x9753186446813579ULL


#define DABC_SEND_RETRY_TM 0.1

// these constants defines how many times and during which time period
// addr resolution requests will be send
// intervals between retries will increase linearly
#define DABC_RESOLVE_RETRY_LIMIT 12
#define DABC_RESOLVE_RETRY_TIME  5.

// these constants defines how many times and during which time period
// command submission will be retried
// this time does not include command execution
#define DABC_SUBMIT_RETRY_LIMIT 12
#define DABC_SUBMIT_RETRY_TIME  5.


// this is retry counts for command execution
// if possible, it should be defined individually for each submitted command
#define DABC_EXECUTE_RETRY_LIMIT  100
#define DABC_EXECUTE_FIRST_TIME   3.
#define DABC_EXECUTE_RETRY_TIME   1000.


// this timeout for all actions when connection is broken,
// all running records will be preserved for such long time
#define DABC_WAIT_RECONNECT_TIME 30.

// enable this define to simulate packet lost on receiving
// #define LOST_RATE 0.3


#ifdef LOST_RATE

class LostStatistic {
   public:

      long numPkts;
      long numLost;
      int cnt;

      LostStatistic()
      {
         srandom(int(dabc::Now()));

         numPkts = 0;
         numLost = 0;

         cnt = int(2. / LOST_RATE * random() / RAND_MAX);
      }

      ~LostStatistic()
      {
         show();
      }

      bool lost()
      {
         numPkts++;
         if (--cnt > 0) return false;

         cnt = int(2. / LOST_RATE * random() / RAND_MAX);
         numLost++;
         return true;
      }

      void show()
      {
         DOUT0("LOST  STATISTIC: Recv %5ld Lost  %5ld %5.3f", numPkts, numLost, (numPkts > 0 ? 1.*numLost/numPkts : 0.);
      }
};

LostStatistic lost;

#endif



namespace dabc {

   class SocketCmdRec {
      public:

         enum EState {

            /** These are states of address resolution, client side */
            stRequestResolve,     //!< state with resolve request (on client)

            /** These are states of address resolution, server (respond) side */
            stReplyResolve,    //!< state just sends back packet that we are existing

            /** This is state to disconnect from the node */
            stDisconnect,      //!< just send information that connections will be deactivated, no reply is required

            /** These are states on the command sender side */
            stSendCommand,   //!< command send to remote and waiting for replay
            stGetConfirm,    //!< this indicates that command is confirmed, but not executed by receiver

            /** These are states on the command receiver side */
            stReceived,      //!< command received from remote
            stConfirmed,     //!< command confirmed to the remote side
            stExecuted       //!< command executed
         };

         enum EDelayKind {
            kindLinear = 1,        //!< all delay equidistant over whole range
            kindProgressive = 2,   //!< arithmetic progression
            kindBurst = 3          //!< first half short, than linear
         };

         Command      fCmd;
         uint32_t     fRecordId;     //!< unique id of the record inside correspondent node
         int          fRecordNode;   //!< node where record was created, node+id are unique inside complete system
         int          fRemoteNode;   //!< remote node there command or reply should be send
         EState       fState;
         TimeStamp    fNextTm;

         std::string  fStrBuf;

         int          fRetryLimit;
         int          fRetryCnt;
         double       fRetryTime;
         EDelayKind   fRetryKind;


         SocketCmdRec(Command cmd, int nodeid, EState state) :
            fCmd(cmd),
            fRecordId(0),
            fRecordNode(0),
            fRemoteNode(nodeid),
            fState(state)
         {
            fNextTm = dabc::Now();

            SetRetryLimit(1);
         }

         virtual ~SocketCmdRec()
         {
            // DOUT0("Delete rec %p", this);
         }


         EState State() const { return fState; }
         void SetState(EState st) { fState = st; }

         bool IsSenderOfCommand() { return (State()==stSendCommand) || (State()==stGetConfirm); }

         bool IsResolveRequest(int nodeid)
         {
            return (State()==stRequestResolve) && ((nodeid<0) || (nodeid==fRemoteNode));
         }

         /** \brief Configure number and duration of retries */
         void SetRetryLimit(int cnt, EDelayKind kind = kindLinear, double fulltm = 5.)
         {
            fRetryCnt = 0;
            fRetryLimit = cnt;
            fRetryKind = kind;
            fRetryTime = fulltm;
         }

         bool IncRetry() { return ++fRetryCnt <= fRetryLimit; }

         int RetryCnt() const { return fRetryCnt; }

         double NextLinearDelay()
         {
            if (fRetryLimit<=1) return fRetryTime;
            return fRetryTime / fRetryLimit;
         }

         /** This generate progressively increasing time intervals
          * If first interval 0.1 s, than 0.2, 0.3 and so on
          * Intervals calculated so that total sum should be fulltm */
         double NextProgressiveDelay()
         {
            if (fRetryLimit<=1) return fRetryTime;

            if ((fRetryCnt<1) || (fRetryCnt>=fRetryLimit)) return DABC_SEND_RETRY_TM;

            return fRetryTime / fRetryLimit / (fRetryLimit-1) * fRetryCnt;
         }

         /** This is another scheme of intervals
          * First half are always short and equal to DABC_SEND_RETRY_TM
          * Second half is equally distributed over \par fulltm period
          */
         double NextBoorstDelay()
         {
            if (fRetryLimit<=1) return fRetryTime;

            if ((fRetryCnt<=fRetryLimit/2) || (fRetryCnt>=fRetryLimit)) return DABC_SEND_RETRY_TM;

            double res = (fRetryTime - (fRetryLimit/2+1) * DABC_SEND_RETRY_TM) / (fRetryLimit / 2.);

            return (res < DABC_SEND_RETRY_TM) ? DABC_SEND_RETRY_TM : res;
         }

         double NextDelay()
         {
            double zn = 1.;
            switch (fRetryKind) {
               case kindLinear:      zn = NextLinearDelay(); break;
               case kindProgressive: zn = NextProgressiveDelay(); break;
               case kindBurst:       zn = NextBoorstDelay(); break;
               default:              zn = 1.;
            }

            if (zn<DABC_SEND_RETRY_TM) zn = DABC_SEND_RETRY_TM;
            return zn;
         }

         void Close(bool res)
         {
            switch (State()) {
               case stRequestResolve:
                  break;

               case stReplyResolve:
                  break;

               case stDisconnect:
                  break;

               case stSendCommand:
               case stGetConfirm:
                  fCmd.ReplyBool(res);
                  break;

               case stReceived:
               case stConfirmed:
               case stExecuted:
                  break;

               default:
                  EOUT("Something else ?? %d", State());
            }
         }

   };

   class SocketCmdRecList : public std::list<dabc::SocketCmdRec*> {
      public:

         virtual ~SocketCmdRecList()
         {
            destroy_recs();
         }

         void destroy_recs()
         {
            while (size()>0) {
               SocketCmdRec* rec = front();
               pop_front();
               rec->Close(false);
               delete rec;
            }
         }

         dabc::SocketCmdRec* find_rec(uint32_t rec_id, int nodeid = -1)
         {
            iterator iter = begin();
            while (iter!=end()) {
               if ((*iter)->fRecordId == rec_id)
                  if ((nodeid<0) || ((*iter)->fRecordNode == nodeid)) return *iter;
               iter++;
            }
            return 0;
         }

         dabc::SocketCmdRec* find_rec_by_cmd(dabc::Command cmd)
         {
            if (cmd.null()) return 0;
            iterator iter = begin();
            while (iter!=end()) {
               if ((*iter)->fCmd == cmd) return *iter;
               iter++;
            }
            return 0;
         }

         dabc::SocketCmdRec* find_rec_by_tm(const dabc::TimeStamp& tm, double* mindiffret = 0)
         {
            dabc::SocketCmdRec* find = 0;
            double mindiff = 100.;
            iterator iter = begin();
            while (iter!=end()) {
               dabc::SocketCmdRec* rec = *iter++;
               if (rec==0) continue;
               double diff = rec->fNextTm - tm;
               if ((find==0) || (diff < mindiff)) {
                  find = rec;
                  mindiff = diff;
               }
            }

            if (mindiffret) *mindiffret = mindiff;
            return mindiff <= 0. ? find : 0;
         }

         /*! \brief Cleanup all records which expire their lifetime */
         void CleanupByTime(const TimeStamp& tm)
         {
            iterator iter = begin();
            while (iter!= end()) {
               SocketCmdRec* rec = *iter;
               if (tm > rec->fNextTm) {
                  iter = erase(iter);
                  rec->Close(false);
                  delete rec;
               } else
                  iter++;
            }
         }

         /*! \brief Checks that address resolution requests exists */
         bool HasRequestResolve(int nodeid)
         {
            iterator iter = begin();
            while (iter!= end()) {
               SocketCmdRec* rec = *iter;
               if ((rec->fRemoteNode == nodeid) && (rec->State()==SocketCmdRec::stRequestResolve)) return true;
               iter++;
            }
            return false;
         }
   };

   class SocketCmdAddr {
      public:

         int                fRemoteNode;     //!< remote node id
         bool               resolved;    //!< is address resolved
         struct sockaddr_in address;     //!< ip address
         TimeStamp       fLastRecvTm; //!< indicates when last packet was received from the remote node

         SocketCmdAddr(int node) :
            fRemoteNode(node),
            resolved(false),
            fLastRecvTm()
         {
            memset (&address, 0, sizeof (address));
         }

         virtual ~SocketCmdAddr()
         {
         }

   };

   enum ECmdDataKind {
      kindResolveReq    = 1,   //!< address resolution request              (client -> server)
      kindResolveRepl   = 2,   //!< address resolution response             (server -> client)
      kindCommandReq    = 3,   //!< command request                         (client -> server)
      kindCommandConf   = 4,   //!< confirmation that command received      (server -> client)
      kindCommandRepl   = 5,   //!< command reply                           (server -> client)
      kindCommandCancel = 6,   //!< cancellation of the command             (server -> client)
      kindDisconnect    = 7,   //!< close of connection                     (client -> server)
   };

}




dabc::SocketCommandAddon::SocketCommandAddon(int fd, int port) :
   dabc::SocketAddon(fd),
   fPort(port)
{
   SetDoingInput(true);
   SetDoingOutput(true);
}

dabc::SocketCommandAddon::~SocketCommandAddon()
{
}

void dabc::SocketCommandAddon::ProcessEvent(const EventId& evnt)
{
   switch (evnt.GetCode()) {
      case evntSocketRead: {

         SetDoingInput(true);

         SocketCmdPacket recvhdr;
         uint8_t         recvbuf[MAX_CMD_PAYLOAD];
         struct sockaddr_in recvaddr;

         ssize_t len = DoRecvBufferHdr(&recvhdr, sizeof(SocketCmdPacket),
                                        recvbuf, MAX_CMD_PAYLOAD,
                                       &recvaddr, sizeof(struct sockaddr_in));

         SocketCommandChannel* chl = (SocketCommandChannel*) fWorker();

//         DOUT0("Recv data len = %d", len);

         if (len>0) chl->ProcessRecvData(&recvhdr, recvbuf, len, &recvaddr, sizeof(struct sockaddr_in));

         return;
      }

      case evntSocketWrite: {

//         DOUT0("Can write to the socket");

         // do not automatically enable event generation - only if we want to send something
         // SetDoingOutput(true);

         SocketCommandChannel* chl = (SocketCommandChannel*) fWorker();

         chl->fCanSendDirect = true;

         chl->CheckSocketCanSend();

         return;
      }
   }

   dabc::SocketAddon::ProcessEvent(evnt);

}

bool dabc::SocketCommandAddon::SendData(void* hdr, unsigned hdrsize, void* data, unsigned datasize, void* addr, unsigned addrsize)
{
   ssize_t res = DoSendBufferHdr(hdr, hdrsize, data, datasize, addr, addrsize);
   bool isok = (res == hdrsize + datasize);

   if (isok) SetDoingOutput(true);

   return isok;
}



// ===============================================================================


dabc::SocketCommandChannel::SocketCommandChannel(const std::string& name, SocketCommandAddon* addon) :
   dabc::Worker(MakePair(name.empty() ? "/CommandChannel" : name)),
   fAppId(DABC_APP_HEADER),
   fAcceptRequests(true),
   fRecs(0),
   fPending(0),
   fProcessed(0),
   fConnCmds(CommandsQueue::kindSubmit),
   fAddrs()
{
   AssignAddon(addon);

   fCanSendDirect = true;
   fRecIdCounter = 0;

   fCleanupTm = 0.;

   fNodeId = dabc::mgr()->cfg()->MgrNodeId();

   fRecs = new SocketCmdRecList;
   fPending = new SocketCmdRecList;
   fProcessed = new SocketCmdRecList;

   fSendPackets = 0;
   fRetryPackets = 0;

   DOUT0("Create dabc::SocketCommandChannel parent %p", GetParent());
}

dabc::SocketCommandChannel::~SocketCommandChannel()
{
   #ifdef LOST_RATE
   lost.show();
   #endif

   DOUT0("RETRY STATISTIC: Send %5ld Retry %5ld %5.3f", fSendPackets, fRetryPackets, (fSendPackets>0 ? 1.*fRetryPackets/fSendPackets : 0.));

   delete fRecs;
   delete fPending;
   delete fProcessed;

   fConnCmds.Cleanup();

   while (fAddrs.size()>0) {
      SocketCmdAddr* rec = (SocketCmdAddr*) fAddrs.pop();
      if (rec!=0) delete rec;
   }
}

int dabc::SocketCommandChannel::PreviewCommand(Command cmd)
{
   std::string url = cmd.GetReceiver();

   if (url.empty())
      return dabc::Worker::PreviewCommand(cmd);

   int remnode = Url::ExtractNodeId(url);

   if ((remnode<0) || (remnode==fNodeId))
      return dabc::Worker::PreviewCommand(cmd);

   int resolve = TryAddressResolve(remnode);

   // remote node address cannot be resolved
   if (resolve<0) return cmd_false;

   std::string sbuf;
   cmd.SaveToString(sbuf);

   if (sbuf.length() > MAX_CMD_PAYLOAD) {
      // FIXME: one should implement treatment of very long commands
      EOUT("TOO long command %u - supported only %u", sbuf.length(), MAX_CMD_PAYLOAD);
      return cmd_false;
   }

   DOUT2("Create command record for remnode %d", remnode);

   SocketCmdRec* rec = new SocketCmdRec(cmd, remnode, SocketCmdRec::stSendCommand);
   rec->fRecordId = fRecIdCounter++;
   rec->fRecordNode = fNodeId;
   rec->fStrBuf = sbuf;
   rec->SetRetryLimit(DABC_SUBMIT_RETRY_LIMIT, SocketCmdRec::kindBurst, DABC_SUBMIT_RETRY_TIME);

   if (resolve>0)
      fRecs->push_back(rec);
   else {
      // command cannot wait forever - it will be canceled sometime
      rec->fNextTm = dabc::Now() + DABC_WAIT_RECONNECT_TIME;
      fPending->push_back(rec);
   }

   // check if we could send data immediately

   CheckSocketCanSend();

   return cmd_postponed;
}

int dabc::SocketCommandChannel::ExecuteCommand(Command cmd)
{
   if (cmd.IsName("ConnectAll")) {

      fAcceptRequests = true;

      if (CheckConnCommandStatus(cmd)) return cmd_true;

      for (int n=0; n< (int) dabc::mgr()->cfg()->NumNodes(); n++) {
         if (n==fNodeId) continue;

         int ret = TryAddressResolve(n);

         if (ret < 0) return cmd_false;
      }

      CheckAllConnCommands(cmd);

      CheckSocketCanSend();

      return cmd_postponed;
   } else
   if (cmd.IsName(CmdShutdownControl::CmdName())) {

      fAcceptRequests = false;

      if (CheckConnCommandStatus(cmd)) return cmd_true;

      CheckAllConnCommands(cmd);

      for (unsigned n=0; n<fAddrs.size(); n++) {
         dabc::SocketCmdAddr* addr = (dabc::SocketCmdAddr*) fAddrs[n];
         if (addr==0) continue;

         SocketCmdRec* rec = new SocketCmdRec(0, n, SocketCmdRec::stDisconnect);
         rec->fRecordId = fRecIdCounter++;
         rec->fRecordNode = fNodeId;
         fRecs->push_back(rec);
      }

      CheckSocketCanSend();

      return cmd_postponed;
   } else
   if (cmd.IsName(CmdGetNodesState::CmdName())) {

      std::string sbuf;
      for (unsigned n=0; n<fAddrs.size(); n++) {
         SocketCmdAddr* addr = FindAddr(n);
         CmdGetNodesState::SetState(sbuf, n, addr && addr->resolved);
      }
      cmd.SetStr(CmdGetNodesState::States(), sbuf);

      return cmd_true;
   }


   return dabc::Worker::ExecuteCommand(cmd);
}


void dabc::SocketCommandChannel::ProduceNodesStateNotification(int nodeid)
{
   dabc::ApplicationRef appref = dabc::mgr.app();
   if (appref.null()) return;

   CmdGetNodesState cmd;

   std::string sbuf;
   for (unsigned n=0; n<fAddrs.size(); n++) {
      SocketCmdAddr* addr = FindAddr(n);
      CmdGetNodesState::SetState(sbuf, n, addr && addr->resolved);
   }
   cmd.SetStr(CmdGetNodesState::States(), sbuf);

   cmd.SetInt(CmdGetNodesState::NodeId(), nodeid);

   appref.Submit(cmd);
}

bool dabc::SocketCommandChannel::CheckConnCommandStatus(Command cmd)
{
   if (cmd.null()) return false;

   if (cmd.IsName("ConnectAll")) {
      for (int n=0; n< (int) dabc::mgr()->cfg()->NumNodes(); n++) {
         if (n==fNodeId) continue;

         SocketCmdAddr* addr = FindAddr(n);
         if ((addr==0) || !addr->resolved) return false;
      }

      return true;
   } else
   if (cmd.IsName(CmdShutdownControl::CmdName())) {
      for (int n=0; n< (int) dabc::mgr()->cfg()->NumNodes(); n++)
         if (FindAddr(n) != 0) return false;
      return true;
   }

   return false;
}

void dabc::SocketCommandChannel::CheckAllConnCommands(Command newcmd)
{
   while (fConnCmds.Size()>0) {
      if (CheckConnCommandStatus(fConnCmds.Front()))
         fConnCmds.Pop().ReplyTrue();
      else
      if (!newcmd.null())
         fConnCmds.Pop().ReplyFalse();
      else
         break;
   }

   if (!newcmd.null()) fConnCmds.Push(newcmd);
}


bool dabc::SocketCommandChannel::ReplyCommand(Command cmd)
{
   // ReplyCommand return true, when command can be safely deleted

   // first search in active commands
   SocketCmdRec* rec = fRecs->find_rec_by_cmd(cmd);

   // it can happen that meanwhile connection is broken and command is pending
   if (rec==0) rec = fPending->find_rec_by_cmd(cmd);

   if (rec==0) {
      EOUT("Cannot find record for command: %s", cmd.GetName());
      return true;
   }

   cmd.SaveToString(rec->fStrBuf);

   DOUT2("RECID:%d:%u !!!!!! Reply command %s !!!!", rec->fRecordNode, rec->fRecordId, cmd.GetName());

   rec->fCmd = 0;
   rec->SetState(SocketCmdRec::stExecuted);
   rec->fNextTm = dabc::Now();
   rec->SetRetryLimit(1);

   CheckSocketCanSend();

   return true;
}

/** Check if specified node can be resolved.
 * Return  0 - if address not resolved, but will be tried
 *        -1 - no possibility to resolved address at all
 *         1 - node address is resolved
 * */
int dabc::SocketCommandChannel::TryAddressResolve(int nodeid)
{
   SocketCmdAddr* addr = FindAddr(nodeid);
   if ((addr!=0) && addr->resolved) return 1;

   // request is already formulated and no additional actions are required
   if (addr!=0) return 0;

   std::string nodename = dabc::mgr()->cfg()->NodeName(nodeid);
   int remport = dabc::mgr()->cfg()->NodePort(nodeid);
   if (remport <= 0) remport = 1237;

   if (nodename.length()==0) {
      EOUT("Cannot get nodename for node %d", nodeid);
      return -1;
   }

   struct hostent *host = gethostbyname(nodename.c_str());
   if ((host==0) || (host->h_addrtype!=AF_INET)) {
      EOUT("Cannot get host information for %s", nodename.c_str());
      return -1;
   }

   addr = new SocketCmdAddr(nodeid);

   addr->resolved = false;
   addr->address.sin_family = AF_INET;
   memcpy(&(addr->address.sin_addr.s_addr), host->h_addr_list[0], host->h_length);
   addr->address.sin_port = htons (remport);

   while ((int)fAddrs.size() <= nodeid) fAddrs.push_back(0);

   fAddrs[nodeid] = addr;

   SocketCmdRec* rec = new SocketCmdRec(0, nodeid, SocketCmdRec::stRequestResolve);
   rec->fRecordId = fRecIdCounter++;
   rec->fRecordNode = fNodeId;
   rec->SetRetryLimit(DABC_RESOLVE_RETRY_LIMIT, SocketCmdRec::kindProgressive, DABC_RESOLVE_RETRY_TIME);
   fRecs->push_back(rec);

   CheckSocketCanSend();

   return 0;
}

void dabc::SocketCommandChannel::RefreshAddr(int nodeid, void* addr, int addrlen)
{
   if ((addr==0) || (addrlen < (int)sizeof(struct sockaddr_in))) return;

   SocketCmdAddr* rec = FindAddr(nodeid);

   if (rec==0) {
      rec = new SocketCmdAddr(nodeid);

      rec->resolved = true; // we know from the beginning that address is resolved

      memcpy(&(rec->address), addr, sizeof(struct sockaddr_in));

      while ((int)fAddrs.size() <= nodeid) fAddrs.push_back(0);

      DOUT2("Node %d send its address itself", nodeid);

      fAddrs[nodeid] = rec;

      ProduceNodesStateNotification(nodeid);

   } else
   if (rec->resolved) {
      if (memcmp(&(rec->address), addr, sizeof(struct sockaddr_in))==0) return;

      EOUT("Address mismatch for remote node %d - use new", nodeid);

      memcpy(&(rec->address), addr, sizeof(struct sockaddr_in));
   } else {

      if (memcmp(&(rec->address), addr, sizeof(struct sockaddr_in))==0)
         DOUT2("We got reply from node %d during resolution - good", nodeid);
      else {
         EOUT("Address from node %d differ from that we trying to resolve - accept new", nodeid);
         memcpy(&(rec->address), addr, sizeof(struct sockaddr_in));
      }

      rec->resolved = true;

      ProduceNodesStateNotification(nodeid);
   }

   if (rec && rec->resolved) {

      rec->fLastRecvTm = dabc::Now();

      // first we delete all resolution requests to that node
      SocketCmdRecList::iterator iter = fRecs->begin();
      while (iter != fRecs->end()) {
         SocketCmdRec* rec = *iter;
         if (rec->IsResolveRequest(nodeid)) {
            iter = fRecs->erase(iter);
            rec->Close(true);
            delete rec;
         } else
            iter++;
      }

      // second, activate records submitted to this node
      iter = fPending->begin();
      while (iter != fPending->end()) {
         SocketCmdRec* rec = *iter;
         if (rec->fRemoteNode == nodeid) {
            iter = fPending->erase(iter);
            rec->fNextTm = dabc::Now();
            fRecs->push_back(rec);
         } else
            iter++;
      }

      // third check if connection command is finished
      CheckAllConnCommands();
   }
}

bool dabc::SocketCommandChannel::CheckAddr(int nodeid, void* addr, int addrlen)
{
   if ((addr==0) || (addrlen < (int)sizeof(struct sockaddr_in))) return false;

   SocketCmdAddr* rec = FindAddr(nodeid);

   if (rec==0) return false;

   if (memcmp(&(rec->address), addr, sizeof(struct sockaddr_in))!=0) return false;

   rec->fLastRecvTm = dabc::Now();

   return true;
}


void dabc::SocketCommandChannel::CloseRec(SocketCmdRec* rec, double pospone_tm)
{
   if (rec==0) return;
   fRecs->remove(rec);
   if (pospone_tm>0) {
      rec->fNextTm = dabc::Now() + pospone_tm;
      fProcessed->push_back(rec);
   } else {
      rec->Close(true);
      delete rec;
   }
}

void dabc::SocketCommandChannel::ErrorCloseRec(SocketCmdRec* rec)
{
   // TODO: probably record can be stored in processed records?

   int close_addr = -1;

   if (rec->State() == SocketCmdRec::stRequestResolve) {
      // FIXME: if address resolution fails, one should cancel
      // all pending commands and raise event that node cannot be resolved

      close_addr = rec->fRemoteNode;

      EOUT("!!!!! Node %d cannot be resolved !!!!!", rec->fRemoteNode);
   }

   fRecs->remove(rec);
   rec->Close(false);
   delete rec;


   // do address close at the end while it may cleanup/move some records
   if (close_addr>=0)
      CloseAddr(close_addr);
}

void dabc::SocketCommandChannel::OnThreadAssigned()
{
   DOUT2("Start thread in command channel");
   ActivateTimeout(1.);
}

double dabc::SocketCommandChannel::CheckSocketCanSend(bool activate_timeout)
{
   if (!fCanSendDirect) return 1.;

   double diff = -1.;

   DoSendData(&diff);

   if (activate_timeout && fCanSendDirect)
      if ((diff>0.) && (diff<1.)) ActivateTimeout(diff);

   return (diff>0.) && (diff<1.) && fCanSendDirect ? diff : 1.;
}


void dabc::SocketCommandChannel::ProcessRecvData(SocketCmdPacket* hdr, void* data, int len, void* addr, int addrlen)
{
   int datalen = len - sizeof(SocketCmdPacket);

   if (datalen < 0) {
      EOUT("UDP Message too short %d - just skip it", len);
      return;
   }

   if ((hdr->dabc_header != DABC_CMD_HEADER) ||
       (hdr->app_header != fAppId)) {
      EOUT("Header does not match - ignore it");
      return;
   }

   if ((int) hdr->target_id != fNodeId) {
      EOUT("Target node mismatch %u != %d in the incoming packet from %u, ignore", hdr->target_id, fNodeId, hdr->source_id);
      return;
   }


#ifdef LOST_RATE

   if (lost.lost()) {
      DOUT1("Skip packet %u %u -> %u res:%d", hdr->data_kind, hdr->source_id, hdr->target_id, len);
      return;
   }

#endif

   DOUT3("SocketCommandChannel::ProcessRecvData packet %u %u -> %u res:%d", hdr->data_kind, hdr->source_id, hdr->target_id, len);

   switch (hdr->data_kind) {
      case kindResolveReq: {
         if (!fAcceptRequests) return;

         // refresh address of sender
         RefreshAddr(hdr->source_id, addr, addrlen);

         // add record with reply of the resolve request
         SocketCmdRec* rec = new SocketCmdRec(0, hdr->source_id, SocketCmdRec::stReplyResolve);
         rec->fRecordId = hdr->rec_id;
         rec->fRecordNode = hdr->source_id;
         fRecs->push_back(rec);

         CheckSocketCanSend();

         break;
      }

      case kindResolveRepl: {
         // remove request record
         SocketCmdRec* rec = fRecs->find_rec(hdr->rec_id, fNodeId);
         if (rec==0) {
            DOUT3("No request found for reply");
         }
         CloseRec(rec);

         // refresh address of sender
         RefreshAddr(hdr->source_id, addr, addrlen);

         break;
      }

      case kindCommandReq: {

         if (!fAcceptRequests) return;

         if (!CheckAddr(hdr->source_id, addr, addrlen)) {
            EOUT("IP Address of node %u is wrong", hdr->source_id);
            return;
         }

         // first of all check if command request was already received
         SocketCmdRec* rec = fRecs->find_rec(hdr->rec_id, hdr->source_id);
         if (rec!=0) {
            if (rec->State() == SocketCmdRec::stReceived) {
               // reply as soon as possible
               rec->fNextTm = dabc::Now();
               DOUT2("RECID:%d:%u !!!!!! Confirm as soon as possible !!!!", rec->fRecordNode, rec->fRecordId);
            } else
            if (rec->State() == SocketCmdRec::stConfirmed) {
               // if we already confirm command, do it again
               rec->fNextTm = dabc::Now();
               rec->SetState(SocketCmdRec::stReceived);
               DOUT2("RECID:%d:%u !!!!!! Confirm again !!!!", rec->fRecordNode, rec->fRecordId);
            } else
            if (rec->State() == SocketCmdRec::stExecuted) {
               // in this state command will be replied anyway
               DOUT2("RECID:%d:%u !!!!!! Executed - should be replied !!!!", rec->fRecordNode, rec->fRecordId);
            } else {
               // FIXME: should ignore such message in the future
               EOUT("RECID:%d:%u !!!!!! We get command request again when record state is %d - HARD ERROR!!!!", rec->fRecordNode, rec->fRecordId, rec->State());
               exit(1);
            }
            // send confirmed message again
            rec->SetRetryLimit(1);
            break;
         }

         rec = fProcessed->find_rec(hdr->rec_id, hdr->source_id);
         if (rec!=0) {
            // command already processed - reactivate record that reply send back again
            fProcessed->remove(rec);
            fRecs->push_front(rec);

            rec->fNextTm = dabc::Now();
            rec->SetRetryLimit(1);
            break;
         }

         if (datalen==0) {
            EOUT("<<<<<<<< ZERO command length - should not happen >>>>>>>>>>");
            return;
         }

         std::string str;
         str.append((const char*)data, datalen);

         dabc::Command cmd;
         if (!cmd.ReadFromString(str.c_str())) {
            EOUT("Cannot interpret command: %s - ignore it", str.c_str());
            return;
         }

         rec = new SocketCmdRec(cmd, hdr->source_id, SocketCmdRec::stReceived);
         rec->fRecordId = hdr->rec_id;
         rec->fRecordNode = hdr->source_id;
         rec->fNextTm = dabc::Now() + 0.5*DABC_SEND_RETRY_TM; // wait 0.05 second before confirm record
         fRecs->push_back(rec);

         DOUT2("RECID:%d:%u !!!!!! Get command from remote !!!!", rec->fRecordNode, rec->fRecordId);

         dabc::mgr()->Submit(Assign(cmd));

         CheckSocketCanSend();

         break;
      }

      case kindCommandConf: {

         if (!CheckAddr(hdr->source_id, addr, addrlen)) {
            EOUT("IP Address of node %u is wrong", hdr->source_id);
            return;
         }

         SocketCmdRec* rec = fRecs->find_rec(hdr->rec_id, fNodeId);
         if (rec==0) {
            EOUT("No request found for reply");
         } else {
            rec->SetState(SocketCmdRec::stGetConfirm);
            DOUT2("RECID:%d:%u !!!!!! Get confirmation !!!!", rec->fRecordNode, rec->fRecordId);
            // FIXME: exact number of retires and timedelay for first retry should be configurable
            rec->SetRetryLimit(DABC_EXECUTE_RETRY_LIMIT, SocketCmdRec::kindProgressive, DABC_EXECUTE_RETRY_TIME);
            rec->fNextTm = dabc::Now() + 0.5/* DABC_EXECUTE_FIRST_TIME */;
         }
         break;
      }

      case kindCommandRepl: {
         // we get reply for the command, process it

         if (!CheckAddr(hdr->source_id, addr, addrlen)) {
            EOUT("IP Address of node %u is wrong", hdr->source_id);
            return;
         }

         SocketCmdRec* rec = fRecs->find_rec(hdr->rec_id, fNodeId);
         if (rec==0) {
            EOUT("No command with such id - ignore");
            break;
         }

         std::string str;
         str.append((const char*)data, datalen);

         dabc::Command cmd;
         if (!cmd.ReadFromString(str.c_str())) {
            EOUT("Cannot interpret command: %s - ignore it", str.c_str());
            return;
         }

         DOUT2("RECID:%d:%u !!!!!! Get reply !!!!", rec->fRecordNode, rec->fRecordId);

         rec->fCmd.AddValuesFrom(cmd);
         int res = cmd.GetResult();

         rec->fCmd.Reply(res);

         // record can be deleted immediately - no matter if
         CloseRec(rec);

         break;
      }

      case kindDisconnect: {

         if (FindAddr(hdr->source_id) && !CheckAddr(hdr->source_id, addr, addrlen)) {
            EOUT("IP Address of node %u is wrong", hdr->source_id);
            return;
         }

         DOUT2("Connection closed by node %u", hdr->source_id);
         CloseAddr(hdr->source_id);
         break;
      }

      default:
         EOUT("Request of unknown type %u - ignore", hdr->data_kind);
         break;
   };
}

dabc::SocketCmdAddr* dabc::SocketCommandChannel::FindAddr(int nodeid)
{
   if ((nodeid<0) || (nodeid >= (int) fAddrs.size())) return 0;

   dabc::SocketCmdAddr* rec = (dabc::SocketCmdAddr*) fAddrs[nodeid];

   // TODO: check if address not yet resolved

   return rec;
}


void dabc::SocketCommandChannel::CloseAddr(int nodeid)
{
   if ((nodeid<0) || (nodeid >= (int) fAddrs.size())) return;

   dabc::SocketCmdAddr* addr = (dabc::SocketCmdAddr*) fAddrs[nodeid];

   if (addr!=0) {

      bool notify = addr->resolved;

      fAddrs[nodeid] = 0;
      delete addr;

      if (notify) ProduceNodesStateNotification(nodeid);
   }

   // FIXME: cancel all submitted records

   // first we delete all resolution requests to that node
   SocketCmdRecList::iterator iter = fRecs->begin();
   while (iter != fRecs->end()) {
      SocketCmdRec* rec = *iter;
      if (rec->fRemoteNode != nodeid) { iter++; continue; }

      iter = fRecs->erase(iter);

      switch (rec->State()) {
         case SocketCmdRec::stRequestResolve:
         case SocketCmdRec::stReplyResolve:
         case SocketCmdRec::stDisconnect:
            rec->Close(false);
            delete rec;
            break;

         case SocketCmdRec::stSendCommand:
         case SocketCmdRec::stGetConfirm:

         case SocketCmdRec::stReceived:
         case SocketCmdRec::stConfirmed:
            rec->fNextTm = dabc::Now() + DABC_WAIT_RECONNECT_TIME;
            fPending->push_back(rec);
            break;

         case SocketCmdRec::stExecuted:
            rec->fNextTm = dabc::Now() + DABC_WAIT_RECONNECT_TIME;
            fProcessed->push_back(rec);
            break;

         default:
            EOUT("INTERNAL ERROR - undefined record state %d", rec->State());
            rec->Close(false);
            delete rec;
            break;
      }
   }

   CheckAllConnCommands();
}



/*! Method used to send data over socket
 *
 * If no
 */

bool dabc::SocketCommandChannel::DoSendData(double* diff)
{
   dabc::TimeStamp tm = dabc::Now();

   if (diff) *diff = -1.;

   SocketCmdRec* find = 0;
   SocketCmdAddr* cmdaddr = 0;

   // set flag true, only at the end when operation started we can switch flag back to the false
   fCanSendDirect = true;

   SocketCommandAddon* addon = (SocketCommandAddon*) fAddon();

   if (addon==0) {
      EOUT("No proper addon");
      return false;
   }

   do {

      find = fRecs->find_rec_by_tm(tm, diff);
      if (find==0) return false;

      // check if we can process record - maybe retry counter is expired
      if (!find->IncRetry()) {
         EOUT("Record retry counter is over, delete it");
         ErrorCloseRec(find);
         find = 0;
         continue;
      }

      // if one can not define address for record, it should be discarded
      cmdaddr = FindAddr(find->fRemoteNode);
      if (cmdaddr == 0) {
         EOUT("Command rec for node %d without address", find->fRemoteNode);
         ErrorCloseRec(find);
         find = 0;
         continue;
      }

   } while (find==0);


   SocketCmdPacket sendhdr;

   sendhdr.dabc_header = DABC_CMD_HEADER;
   sendhdr.app_header = fAppId;
   sendhdr.source_id = fNodeId;
   sendhdr.target_id = find->fRemoteNode;
   sendhdr.rec_id = find->fRecordId;
   sendhdr.data_kind = 0;
   sendhdr.data_len = 0;

   void* send_data = 0;

   switch (find->State()) {
      case SocketCmdRec::stRequestResolve:
         // TODO: when sending address resolution request, one can also send known nodes together
         sendhdr.data_kind = kindResolveReq;
         break;
      case SocketCmdRec::stReplyResolve:
         // TODO: when sending address resolution reply, one can also send known nodes together
         sendhdr.data_kind = kindResolveRepl;
         break;
      case SocketCmdRec::stSendCommand:
         sendhdr.data_kind = kindCommandReq;
         sendhdr.data_len = find->fStrBuf.length();
         send_data = (void*) find->fStrBuf.c_str();
         break;
      case SocketCmdRec::stGetConfirm:
         // we send request again but without command - it should be available on the remote side
         sendhdr.data_kind = kindCommandReq;
         break;
      case SocketCmdRec::stReceived:
         sendhdr.data_kind = kindCommandConf;
         break;
      case SocketCmdRec::stConfirmed:
         // one should cancel command execution here and send cancellation back
         EOUT("Command %s was not executed correctly to the end", find->fCmd.GetName());

         find->fCmd.ReplyFalse();
         CloseRec(find);

         sendhdr.data_kind = kindCommandCancel;
         break;
      case SocketCmdRec::stExecuted:
         sendhdr.data_kind = kindCommandRepl;
         sendhdr.data_len = find->fStrBuf.length();
         send_data = (void*) find->fStrBuf.c_str();
         break;
      case SocketCmdRec::stDisconnect:
         sendhdr.data_kind = kindDisconnect;
         break;
      default:
         EOUT("Nothing to do here");
         return false;
   }

   fSendPackets++;
   if (find->RetryCnt() > 1) { fRetryPackets++; /* DOUT1("??? retry ???"); */ }

   DOUT3("SocketCommandChannel::DoSendData packet %u %u -> %u", sendhdr.data_kind, sendhdr.source_id, sendhdr.target_id);

   if (!addon->SendData(&sendhdr, sizeof(sendhdr),
                        send_data, sendhdr.data_len,
                        &(cmdaddr->address), sizeof(cmdaddr->address))) {
      EOUT("Error sending command buffer kind %u", sendhdr.data_kind);
      ErrorCloseRec(find);
      return false;
   }

   fCanSendDirect = false;

   switch (find->State()) {
      case SocketCmdRec::stRequestResolve:
         // just try to resend request with progressively increasing timeouts
         find->fNextTm = tm + find->NextDelay();
         break;
      case SocketCmdRec::stReplyResolve:
         // keep record for 5 seconds
         CloseRec(find, 5.);
         break;
      case SocketCmdRec::stSendCommand:
         // just try to resend data after 0.1 sec
         find->fNextTm = tm + find->NextDelay();

         DOUT2("RECID:%d:%u !!!!!! Send command !!!!", find->fRecordNode, find->fRecordId);

         break;
      case SocketCmdRec::stGetConfirm:
         // retry with increasing period
         find->fNextTm = tm + find->NextDelay();
         break;
      case SocketCmdRec::stReceived:
         find->SetState(SocketCmdRec::stConfirmed);
         // FIXME: here time should depend from expected execution time.
         // In worse case time can be infinite - command sender should inform that command must be cancelled
         find->fNextTm = tm + 100.;
         find->SetRetryLimit(1);
         break;
      case SocketCmdRec::stExecuted:
         // keep record for 10 seconds
         CloseRec(find, 10.);
         break;
      case SocketCmdRec::stDisconnect:
         CloseRec(find);
         CloseAddr(find->fRemoteNode);
         break;

      default:
         EOUT("Still here");
         break;
   }

   return true;
}

double dabc::SocketCommandChannel::ProcessTimeout(double diff)
{
   // TODO: process per timeout records which should be repeated

   fCleanupTm += diff;

   if (fCleanupTm>1.) {
      fCleanupTm = 0.;
      TimeStamp tm = dabc::Now();

      fProcessed->CleanupByTime(tm);
      fPending->CleanupByTime(tm);

      // check nodes from that we have no packets since a long time
      for (unsigned nodeid=0; nodeid<fAddrs.size(); nodeid++) {
         dabc::SocketCmdAddr* addr = (dabc::SocketCmdAddr*) fAddrs[nodeid];
         if ((addr==0) || (!addr->resolved)) continue;

         double dist = tm - addr->fLastRecvTm;
         if (dist<DABC_WAIT_RECONNECT_TIME) continue;

         if (fRecs->HasRequestResolve(nodeid)) continue;

         SocketCmdRec* rec = new SocketCmdRec(0, nodeid, SocketCmdRec::stRequestResolve);
         rec->fRecordId = fRecIdCounter++;
         rec->fRecordNode = fNodeId;
         rec->SetRetryLimit(DABC_RESOLVE_RETRY_LIMIT, SocketCmdRec::kindProgressive, DABC_RESOLVE_RETRY_TIME*2.);
         fRecs->push_back(rec);

         CheckSocketCanSend();
      }
   }

   // in timeout event check if we need to send some more data
   // return value used as time for next timeout (at maximum - 1 sec)
   return CheckSocketCanSend(false);
}

dabc::SocketCommandChannel* dabc::SocketCommandChannel::CreateChannel(const std::string& objname)
{

   int nport = dabc::mgr()->cfg()->MgrPort();

   if (nport <= 0) nport = 1237;

   int fd = dabc::SocketThread::StartUdp(nport, 7000, 7100);

   if (fd<=0) {
      EOUT("Cannot open cmd socket");
      return 0;
   }

   DOUT1("Start CMD SOCKET port:%d", nport);

   SocketCommandAddon* addon = new SocketCommandAddon(fd, nport);

   return new SocketCommandChannel(objname, addon);
}
