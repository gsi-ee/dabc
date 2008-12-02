#include "dabc/CommandChannelModule.h"

#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/collections.h"
#include "dabc/Command.h"
#include "dabc/PoolHandle.h"
#include "dabc/Buffer.h"
#include "dabc/Port.h"
#include "dabc/Manager.h"

const int CommandBufferSize = 16*1024 - 16;

dabc::CommandChannelModule::CommandChannelModule(int numnodes) :
   ModuleAsync("CommandChannel"),
   fPool(0),
   fCmdOutQueue(0)
{
   fPool = CreatePool("CommandChannelPool", 100, CommandBufferSize);

   fCmdOutQueue = new CommandsQueue(false, false);

   for (int n=0;n<numnodes;n++)
      CreatePort(FORMAT(("Port%d",n)), fPool, 5, 5, 0);
}

dabc::CommandChannelModule::~CommandChannelModule()
{
   delete fCmdOutQueue;
   fCmdOutQueue = 0;

   DOUT1(("CommandChannelModule destroyed"));
}

int dabc::CommandChannelModule::ExecuteCommand(Command* cmd)
{
   if (cmd->IsName("SendCommand")) {
      fCmdOutQueue->Push(cmd);
      SendSubmittedCommands();
      return cmd_postponed;
   } else
      return dabc::ModuleAsync::ExecuteCommand(cmd);
}

void dabc::CommandChannelModule::ConnDissPort(Port* port, bool on)
{
   int portid = IOPortNumber(port);

   DOUT4((" CommandChannelModule::ConnDissPort port %p id %d", port, portid));

   DOUT4(("Port %d %s %s", portid, (on ? "connected" : "disconnected"), port->GetName()));

   dabc::Command* cmd = new dabc::Command("ActivateSlave");
   cmd->SetInt("PortId", portid);
   cmd->SetInt("IsActive", on ? 1 : 0);

   dabc::mgr()->Submit(cmd);

   DOUT4((" CommandChannelModule::ConnDissPort done"));
}

void dabc::CommandChannelModule::ProcessConnectEvent(Port* port)
{
   ConnDissPort(port, true);
}

void dabc::CommandChannelModule::ProcessDisconnectEvent(Port* port)
{
   ConnDissPort(port, false);
}

void dabc::CommandChannelModule::ProcessInputEvent(Port* port)
{
   dabc::Buffer* ref = 0;
   port->Recv(ref);
   if (ref==0) {
      EOUT(("Port cannot recv data"));
      return;
   }

   DOUT2(("CommandChannelModule:: Recv command from port %d cmd %s", IOPortNumber(port), ref->GetDataLocation()));

   Command* cmd = new Command("DoCommandRecv");
   cmd->SetStr("cmddata", (const char*) ref->GetDataLocation());
   dabc::mgr()->Submit(cmd);

   dabc::Buffer::Release(ref);

   SendSubmittedCommands();
}

void dabc::CommandChannelModule::ProcessOutputEvent(Port*)
{
   SendSubmittedCommands();
}

void dabc::CommandChannelModule::ProcessPoolEvent(PoolHandle* pool)
{
   dabc::ModuleAsync::ProcessPoolEvent(pool);
}

void dabc::CommandChannelModule::SendSubmittedCommands()
{
   Command* cmd = 0;

   while ((cmd = fCmdOutQueue->Front()) != 0) {

      int portid = cmd->GetInt("PortId", 0);

      Port* outport = IOPort(portid);

      if (outport==0) {
         EOUT(("FALSE PORT id %d", portid));
         exit(1);
      }

      if (outport->OutputBlocked()) break;

      // take buffer without request, while we anyhow get event when send is completed and
      // device will release buffer anyhow
      dabc::Buffer* buf = fPool->TakeBuffer(CommandBufferSize, false);
      // if one has no buffers in pool, one should wait more time
      if (buf==0) break;

      fCmdOutQueue->Pop();

      const char* cmdbuf = cmd->GetPar("Value");
      int cmdlen = cmdbuf==0 ? 0 : strlen(cmdbuf);

      if ((cmdlen==0) || (cmdlen >= (int) buf->GetTotalSize())) {
         EOUT(("Wrong command length %d max %u", cmdlen, buf->GetTotalSize()));
         dabc::Command::Reply(cmd, false);
         dabc::Buffer::Release(buf);
         continue;
      }

      DOUT3(("Send Port:%d Cmd:%s", portid, cmdbuf));

      memcpy(buf->GetDataLocation(), cmdbuf, cmdlen+1);
      buf->SetDataSize(cmdlen+1);

//      DOUT1(("Need to send command to port:%d busy:%s buf:%p", nport, DBOOL(Output(nport)->OutputBlocked()), buf));

      outport->Send(buf);

      dabc::Command::Reply(cmd, true);

      DOUT5(("Send Port:%d done", portid));
   }
}

dabc::MemoryPool* dabc::CommandChannelModule::GetPool()
{
   return fPool ? fPool->getPool() : 0;
}
