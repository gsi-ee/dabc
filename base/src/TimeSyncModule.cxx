#include "dabc/TimeSyncModule.h"

#include "dabc/timing.h"

#include "dabc/MemoryPool.h"
#include "dabc/PoolHandle.h"
#include "dabc/Port.h"

typedef struct TymeSyncMessage
{
   uint64_t msgid;
   uint64_t select_clock_source;
   double   master_time;
   double   slave_time;
   double   slave_init_time;
   double   slave_shift;
   double   slave_scale;
};

const int TimeSyncBufferSize = sizeof(TymeSyncMessage);


dabc::TimeSyncModule::TimeSyncModule(const char* name, Command* cmd) :
   ModuleAsync(name),
   fMasterConn(cmd->GetBool("MasterConn", false)),
   fNumSlaves(cmd->GetInt("NumSlaves", 0)),
   fSyncTimes(0),
   fPoolName(cmd->GetStr("PoolName","TimeSyncPool")),
   fTimeSource(0),
   fCurrCmd(0),
   fSlaveCnt(0),
   fPktCnt(0),
   fLoopTime(),
   fMasterToSlave(),
   fSlaveToMaster(),
   fSetShift(0.),
   fSetScale(1.),
   fSkipSlavePacket(false),
   fPool(0)
{
   fMasterSendCnt = 0;
   fMasterRecvCnt = 0;

   fPool = CreatePoolHandle(fPoolName.c_str(), 0, 5);

   if (fMasterConn)
      CreateIOPort("Master", fPool, 5, 10, TimeSyncBufferSize);

   for (int n=0; n<fNumSlaves; n++)
      CreateIOPort(FORMAT(("Slave%d",n)), fPool, 5, 10, TimeSyncBufferSize);

   if (fNumSlaves>0) {
      fSyncTimes = new double [fNumSlaves];
      for (int n=0;n<fNumSlaves;n++) fSyncTimes[n] = 0;
   }
}


dabc::TimeSyncModule::~TimeSyncModule()
{
   delete [] fSyncTimes; fSyncTimes = 0;
}

void dabc::TimeSyncModule::OnThreadAssigned()
{
   Module::OnThreadAssigned();

   if (ProcessorThread())
      ProcessorThread()->Execute("EnableFastModus");
}


int dabc::TimeSyncModule::ExecuteCommand(Command* cmd)
{
   if (cmd->IsName("DoTimeSync")) {
      if (fNumSlaves<=0) {
         EOUT(("Slave cannot initiate TimeSync"));
         return cmd_false;
      }

      if (fCurrCmd!=0) {
         EOUT(("Previous TimeSync command not yet completed"));
         return cmd_false;
      }

      fCurrCmd = cmd;
      fSlaveCnt = fNumSlaves;
      fPktCnt = 0;

      if (cmd->GetBool("ThrdClock", true))
         fTimeSource = GetThrdTimeSource();

      if (fTimeSource==0) fTimeSource = &gStamping;

      fMasterSendCnt = 0;
      fMasterRecvCnt = 0;

      GenerateNextEvent(0);

      return cmd_postponed;
   }

   return ModuleAsync::ExecuteCommand(cmd);
}

void dabc::TimeSyncModule::GenerateNextEvent(Buffer* buf)
{
   if (fCurrCmd==0) {
      dabc::Buffer::Release(buf);
      return;
   }

   int nport = 0;

   fPktCnt--;
   if (fPktCnt<0) {
      if (fLoopTime.Number()>0) {
         double time_shift = (fSlaveToMaster.Mean() - fMasterToSlave.Mean()) / 2.;

         DOUT1(("Round trip to %2d: %5.1f microsec", fSlaveCnt, fLoopTime.Mean()));
         DOUT1(("   Master -> Slave  : %5.1f  +- %3.1f (max = %5.1f min = %5.1f)", fMasterToSlave.Mean(), fMasterToSlave.Dev(), fMasterToSlave.Max(), fMasterToSlave.Min()));
         DOUT1(("   Slave  -> Master : %5.1f  +- %3.1f (max = %5.1f min = %5.1f)", fSlaveToMaster.Mean(), fSlaveToMaster.Dev(), fSlaveToMaster.Max(), fSlaveToMaster.Min()));

         if ((fSetShift != 0.) || (fSetScale != 1.))
            DOUT1(("   SET: Shift = %5.1f  Coef = %12.10f", fSetShift, fSetScale));
         else
            DOUT1(("   GET: Shift = %5.1f", time_shift));
      }

      fLoopTime.Reset();
      fMasterToSlave.Reset();
      fSlaveToMaster.Reset();
      fSetShift = 0.;
      fSetScale = 1.;

      fSlaveCnt--;
      fPktCnt = fCurrCmd->GetInt("NumIter", 100);
   }

   if (fSlaveCnt<0) {
      DOUT1(("Finish DoTimeSync"));
      dabc::Command::Reply(fCurrCmd, true);
      fCurrCmd = 0;
      dabc::Buffer::Release(buf);
      return;
   }

   if (buf==0)
      buf = fPool->TakeEmptyBuffer(TimeSyncBufferSize);

   nport = fMasterConn ? fSlaveCnt+1 : fSlaveCnt;

   if (fMasterSendCnt > fMasterRecvCnt)
       { EOUT(("Missmtach %d %d diff %d", fMasterSendCnt, fMasterRecvCnt, fMasterSendCnt - fMasterRecvCnt)); }

   if (FillMasterPacket(buf))
      fMasterSendCnt++;
   else
      nport = -1;

   if ((nport>=0) && buf)
      Output(nport)->Send(buf);
   else
      dabc::Buffer::Release(buf);
}

bool dabc::TimeSyncModule::FillMasterPacket(Buffer* buf)
{
   if (buf==0) return false;

//   dabc::BufferSize_t size = sizeof(TymeSyncMessage);
//   buf->SetDataSize(size);
   buf->SetTypeId(mbt_TymeSync);

   TymeSyncMessage* msg = (TymeSyncMessage*) buf->GetHeader();

   int maxcnt = fCurrCmd->GetInt("NumIter", 100);
   bool doshift = fCurrCmd->GetBool("DoShift", false);
   bool doscale = fCurrCmd->GetBool("DoScale", false);
   bool thrdclock = fCurrCmd->GetBool("ThrdClock", true);

   memset(msg, 0, sizeof(TymeSyncMessage));
   msg->msgid = fPktCnt;
   msg->select_clock_source = 0;
   msg->master_time = 0.;
   msg->slave_time = 0.;
   msg->slave_init_time = 0.;
   msg->slave_shift = 0.;
   msg->slave_scale = 1.;

//   DOUT1(("Generate PKT %d  tm %9.1f", msg->msgid, msg->master_time));

   double now = fTimeSource->GetTimeStamp();

//   DOUT1(("  Sending msg%3d buf %p at %8.1f", fPktCnt, buf, now));
//   last_send = now;

   fSkipSlavePacket = false;

   msg->master_time = now;

   if (fPktCnt == maxcnt)
      msg->select_clock_source =  thrdclock ? 1 : 2;

   if ((fPktCnt == maxcnt-1) && doshift && !doscale) {
      msg->slave_init_time = now + 10.;
      fSkipSlavePacket = true;
   }

   // skip first 3 packets while they are typically too far from reality
   if ((maxcnt>10) && (fPktCnt == maxcnt-3))
      fSkipSlavePacket = true;

   // apply fine shift when 2/3 of work is done
   if ((fPktCnt == maxcnt/3) && doshift) {

      double time_shift = (fSlaveToMaster.Mean() - fMasterToSlave.Mean()) / 2.;

      msg->slave_shift = time_shift;

      if (doscale)
         msg->slave_scale = 1./(1.-time_shift/(now - fSyncTimes[fSlaveCnt]));

      fSetShift = msg->slave_shift;
      fSetScale = msg->slave_scale;

      fSyncTimes[fSlaveCnt] = now;

      fSkipSlavePacket = true;
   }

   return true;
}

bool dabc::TimeSyncModule::FillSlavePacket(Buffer* buf)
{
   if (buf==0) return false;

   TymeSyncMessage* msg = (TymeSyncMessage*) buf->GetHeader();

//   DOUT1(("Master msg   %d  tm %9.1f", msg->msgid, msg->master_time));

   // first, select clock source
   if (msg->select_clock_source>0) {
      if (msg->select_clock_source==1)
         fTimeSource = GetThrdTimeSource();
      if (fTimeSource==0) fTimeSource = &gStamping;
   }

   if (fTimeSource==0) { EOUT(("Error here")); fTimeSource = &gStamping; }

   double now = fTimeSource->GetTimeStamp();

   msg->slave_time = now;

   if (msg->slave_init_time>0)
      fTimeSource->AdjustTimeStamp(msg->slave_init_time);

   if (msg->slave_shift!=0.)
      fTimeSource->AdjustCalibr(msg->slave_shift, msg->slave_scale);

   return true;
}

void dabc::TimeSyncModule::ProcessInputEvent(Port* port)
{
   Buffer* buf = port->Recv();

   if (buf==0) {
      EOUT(("Cannot get buffer from input"));
      return;
   }

   if (buf->GetTypeId() != mbt_TymeSync) return;

   if (fMasterConn && (port==Input(0))) {
      if (FillSlavePacket(buf)) {
          port->Send(buf);
      } else {
         EOUT(("Work buffer is empty. Hard error. Halt"));
         exit(1);
      }
   } else {
      double now = fTimeSource->GetTimeStamp();

      TymeSyncMessage* msg = (TymeSyncMessage*) buf->GetHeader();

      if (fSkipSlavePacket) {
         fSkipSlavePacket = false;
         fLoopTime.Reset();
         fMasterToSlave.Reset();
         fSlaveToMaster.Reset();
      } else {
         fLoopTime.Fill(now - msg->master_time);
         fMasterToSlave.Fill(msg->slave_time - msg->master_time);
         fSlaveToMaster.Fill(now - msg->slave_time);
      }

      fMasterRecvCnt++;

      GenerateNextEvent(buf);
   }
}
