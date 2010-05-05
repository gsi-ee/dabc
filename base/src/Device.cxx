/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "dabc/Device.h"

#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/Transport.h"

dabc::Device::Device(Basic* parent, const char* name) :
   Folder(parent, name, true),
   WorkingProcessor(),
   fDeviceMutex(true),
   fTransports(),
   fCleanupCmds(CommandsQueue::kindSubmit),
   fCleanupForce(false)
{
}

dabc::Device::~Device()
{
   // by this call we synchronise us with anything else
   // that can happen on the device

   DOUT3((" ~Device %s prior:%d", GetName(), ProcessorPriority()));

   CleanupDevice(true);
}

void dabc::Device::TransportCreated(Transport *tr)
{
   LockGuard lock(fDeviceMutex);
   fTransports.push_back(tr);
}

void dabc::Device::TransportDestroyed(Transport *tr)
{
   // in normal situation transport is removed from the vector at that place
   // while we destroy transport only in Cleanup branch
   if (tr->IsDoingCleanup()) return;

   EOUT(("Transport %p of device %s deleted abnormal", tr, GetName()));

   // this is not a normal situation, but do it
   LockGuard lock(fDeviceMutex);
   fTransports.remove(tr);
}

bool dabc::Device::DoDeviceCleanup(bool full)
{
   // first, sync all other thread - means force them make at least once mainloop
   // after that they should have any pointers on inactive transports

   DOUT3(("DoDeviceCleanup %s mutex locked %s", GetName(), DBOOL(fDeviceMutex.IsLocked())));

   PointersVector fDelTrans;

   {
      LockGuard lock(fDeviceMutex);

      DOUT3(("Check transports %u", fTransports.size()));

      for (unsigned n=0;n<fTransports.size();n++) {
         Transport *tr = (Transport*) fTransports.at(n);
         if (tr==0) continue;

         DOUT3(("Check transport %p", tr));

         if (tr->CheckNeedCleanup(full)) {
             // by this we exclude any new access to transport from device
             fTransports.remove(tr);
             fDelTrans.push_back(tr);
             DOUT3(("Device %s remove transport %p before destructor", GetName(), tr));
         }
      }
   }

   DOUT3(("DoDeviceCleanup %s middle locked %s", GetName(), DBOOL(fDeviceMutex.IsLocked())));

   while (fDelTrans.size()>0) {
      Transport* tr = (Transport*) fDelTrans[0];
      fDelTrans.remove_at(0);
      if (tr==0) continue;

      DOUT3(("Device %s Halt transport %p", GetName(), tr));

      tr->DoTransportHalt();

      DOUT3(("Device %s Delete transport %p", GetName(), tr));

      delete tr;

      DOUT3(("Device %s Delete transport %p done", GetName(), tr));

   }

   DOUT3(("DoDeviceCleanup %s done locked %s", GetName(), DBOOL(fDeviceMutex.IsLocked())));

   return true;
}

void dabc::Device::InitiateCleanup()
{
   DOUT5(("@@@ InitiateCleanup %s", GetName()));

   // do asynchronous cleanup only when separate thread exists
   if (ProcessorThread())
      Submit(new dabc::Command("CleanupDevice"));
}

void dabc::Device::CleanupDevice(bool force)
{
   DOUT5(("@@@ PerformCleanup %s", GetName()));

   dabc::Command* cmd = new dabc::Command("CleanupDevice");
   cmd->SetBool("Force", force);
   Execute(cmd);
}

dabc::Command* dabc::Device::MakeRemoteCommand(Command* cmd, const char* serverid, const char* channel)
{
   if (cmd==0) return 0;

   cmd->SetStr("_OriginName_", cmd->GetName());
   cmd->SetStr("_ServerId_", serverid);
   cmd->SetStr("_ChannelId_", channel);
   cmd->SetCommandName("_DeviceRemoteCommand_");

   return cmd;
}

int dabc::Device::ExecuteCommand(dabc::Command* cmd)
{
   int cmd_res = cmd_true;

   if (cmd->IsName("CleanupDevice")) {
      fCleanupCmds.Push(cmd);

      if (cmd->GetBool("Force", false)) fCleanupForce = true;

      // this is workaround for async and sync call of cleanup device
      // commands will be queued and completed only when we really finish device cleanup
      if (fCleanupCmds.Size()==1) {

         bool cleanup_res = true;

         unsigned cmd_cnt(1);

         do {
            bool force = fCleanupForce;
            cmd_cnt = fCleanupCmds.Size();
            fCleanupForce = false;
            cleanup_res = DoDeviceCleanup(force);
         } while (fCleanupForce || (cmd_cnt != fCleanupCmds.Size()));

         while (fCleanupCmds.Size()>0)
            dabc::Command::Reply(fCleanupCmds.Pop(), cleanup_res);
      }

      cmd_res = cmd_postponed;
   } else
   if (cmd->IsName(CmdCreateTransport::CmdName()) ||
       cmd->IsName(CmdDirectConnect::CmdName())) {
      Port* port = dabc::mgr()->FindPort(cmd->GetPar("PortName"));
      if (port==0)
         cmd_res = false;
      else
         cmd_res = CreateTransport(cmd, port);
   } else
   if (cmd->IsName("_DeviceRemoteCommand_")) {

      cmd->SetCommandName(cmd->GetPar("_OriginName_"));
      std::string serv = cmd->GetPar("_ServerId_");
      std::string channel = cmd->GetPar("_ChannelId_");
      cmd->RemovePar("_OriginName_");
      cmd->RemovePar("_ServerId_");
      cmd->RemovePar("_ChannelId_");
      if (SubmitRemoteCommand(serv.c_str(), channel.c_str(), cmd))
         cmd_res = cmd_postponed;
      else
         cmd_res = cmd_false;
   } else
      cmd_res = dabc::WorkingProcessor::ExecuteCommand(cmd);

   return cmd_res;
}
