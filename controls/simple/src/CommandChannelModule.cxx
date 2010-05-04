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
#include "dabc/CommandChannelModule.h"

#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/collections.h"
#include "dabc/Command.h"
#include "dabc/PoolHandle.h"
#include "dabc/Buffer.h"
#include "dabc/Port.h"
#include "dabc/Manager.h"

dabc::CommandChannelModule::CommandChannelModule(int numnodes) :
   ModuleAsync("CommandChannel"),
   fCmdOutQueue(0)
{
   CreatePoolHandle(CmdPoolName(), CmdBufSize(), 10);

   fCmdOutQueue = new CommandsQueue(false, false);

   for (int n=0;n<numnodes;n++)
      CreateIOPort(FORMAT(("Port%d",n)), Pool(), 5, 5, 0);
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
   dabc::Buffer* ref = port->Recv();
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
         exit(114);
      }

      if (!outport->CanSend()) break;

      // take buffer without request, while we anyhow get event when send is completed and
      // device will release buffer anyhow
      dabc::Buffer* buf = Pool()->TakeBuffer(CmdBufSize());
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

//      DOUT1(("Need to send command to port:%d busy:%s buf:%p", nport, DBOOL(!Output(nport)->CanSend()), buf));

      outport->Send(buf);

      dabc::Command::Reply(cmd, true);

      DOUT5(("Send Port:%d done", portid));
   }
}

dabc::MemoryPool* dabc::CommandChannelModule::GetPool()
{
   return Pool() ? Pool()->getPool() : 0;
}
