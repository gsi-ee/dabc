#include "dabc/Device.h"

#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/Transport.h"

dabc::Device::Device(Basic* parent, const char* name) : 
   Folder(parent, name, true),
   WorkingProcessor(),
   fDeviceMutex(true),
   fTransports()
{
}

dabc::Device::~Device()
{
   // by this call we synchronise us with anything else 
   // that can happen on the device

   DOUT5((" ~Device %s prior:%d", GetName(), ProcessorPriority()));

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
   
//   EOUT(("Transport %p deleted ubnormal", tr));
   
   // this is not a normal situation, but do it 
   LockGuard lock(fDeviceMutex);
   fTransports.remove(tr);
}

bool dabc::Device::DoDeviceCleanup(bool full)
{
   // first, sync all other thread - means force them make at least once mainloop
   // after that they should have any pointers on inactive transports 
    
   DOUT3(("DoDeviceCleanup %s", GetName())); 
 
   PointersVector fDelTr;

   { 
      LockGuard lock(fDeviceMutex);
         
      DOUT5(("Check transports %u", fTransports.size()));
         
      for (unsigned n=0;n<fTransports.size();n++) {
         Transport *tr = (Transport*) fTransports.at(n);
         if (tr==0) continue;

         DOUT5(("Check transport %p", tr));
            
         if (tr->CheckNeedCleanup(full)) {
             // by this we exclude any new access to transport from device
             fTransports.remove(tr);
             fDelTr.push_back(tr);
         }
      }
   }
   
   for(unsigned n=0;n<fDelTr.size();n++) {
      Transport* tr = (Transport*) fDelTr[n];
 
      DOUT5(("Device %s Delete transport %p", GetName(), tr));
      
      delete tr;

      DOUT5(("Device %s Delete transport %p done", GetName(), tr));

   }

   DOUT3(("DoDeviceCleanup %s done", GetName())); 
    
   return true; 
}

void dabc::Device::InitiateCleanup()
{
   DOUT5(("InitiateCleanup")); 
    
   // do asynchron cleanup only when separate thread exists 
   if (ProcessorThread()) 
      Submit(new dabc::Command("CleanupDevice"));
}

void dabc::Device::CleanupDevice(bool force)
{
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
    
   if (cmd->IsName("CleanupDevice"))
      cmd_res = DoDeviceCleanup(cmd->GetBool("Force", false));
   else
   if (cmd->IsName(CmdCreateTransport::CmdName())) {
      Port* port = GetManager()->FindPort(cmd->GetPar("PortName")); 
      if (port==0)
         cmd_res = false;
      else 
         cmd_res = CreateTransport(cmd, port);
   } else
   if (cmd->IsName("_DeviceRemoteCommand_")) {
       
      cmd->SetCommandName(cmd->GetPar("_OriginName_")); 
      dabc::String serv = cmd->GetPar("_ServerId_");
      dabc::String channel = cmd->GetPar("_ChannelId_");
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
