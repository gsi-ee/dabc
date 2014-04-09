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

#include "mbs/api.h"

#include "dabc/api.h"
#include "dabc/Manager.h"
#include "dabc/Publisher.h"

mbs::ReadoutModule::ReadoutModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fIter(),
   fCmd()
{
   EnsurePorts(1, 0, dabc::xmlWorkPool);

   CreateTimer("SysTimer");
}

int mbs::ReadoutModule::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("NextBuffer")) {
      // previous command not processed - cannot be
      if (!fCmd.null()) return dabc::cmd_false;

      // DOUT0("Call nextbuffer");

      fCmd = cmd;
      double tm = fCmd.TimeTillTimeout();

      if (CanRecv() || (tm<=0))
         ProcessInputEvent(0);
      else
         ShootTimer(0, tm);

      return dabc::cmd_postponed;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

void mbs::ReadoutModule::ProcessInputEvent(unsigned)
{
   // ignore input event as long as command is not specified
   if (fCmd.null()) return;

   // DOUT0("process input event");

   int res = dabc::cmd_false;

   if (CanRecv()) {
      dabc::Buffer buf = Recv();
      // when EOF buffer received, return immediately stop

      switch (buf.GetTypeId()) {
         case dabc::mbt_EOF:
            res = dabc::cmd_false;
            break;
         case mbs::mbt_MbsEvents:
            res = cmd_bool(fIter.Reset(buf));
            break;
         default:
            res = AcceptBuffer(buf);
            break;
      }
   }

   fCmd.Reply(res);
}

void mbs::ReadoutModule::ProcessTimerEvent(unsigned)
{
   // DOUT0("process timer event");

   // if timeout happened, reply
   ProcessInputEvent(0);
}

// ===================================================================================

mbs::ReadoutHandle mbs::ReadoutHandle::DoConnect(const std::string& url, const char* classname)
{
   if (dabc::mgr.null()) {
      dabc::SetDebugLevel(-1);
      dabc::CreateManager("dabc", -1);
   }

   if (dabc::mgr.FindPool(dabc::xmlWorkPool).null()) {
      if (!dabc::mgr.CreateMemoryPool(dabc::xmlWorkPool, 512*1024, 100)) {
         return 0;
      }
   }

   int cnt = 0;
   std::string name;
   do {
      name = dabc::format("MbsReadout%d", cnt);
   } while (!dabc::mgr.FindModule(name).null());

   mbs::ReadoutHandle mdl = dabc::mgr.CreateModule(classname, name);

   if (mdl.null()) return mdl;

   if (!dabc::mgr.CreateTransport(mdl.InputName(), url)) {
      EOUT("Cannot create transport %s",url.c_str());
      mdl.Release();
      dabc::mgr.DeleteModule(name);
      return 0;
   }

   mdl.Start();

   return mdl;
}

bool mbs::ReadoutHandle::Disconnect()
{
   if (null()) return false;

   FindPort(InputName()).Disconnect();

   Stop();

   std::string name = GetName();
   Release();
   dabc::mgr.DeleteModule(name);

   return true;
}


mbs::EventHeader* mbs::ReadoutHandle::NextEvent(double tm)
{
   if (null()) return 0;

   if (GetObject()->fIter.NextEvent())
      return GetObject()->fIter.evnt();

   if (!Execute(dabc::Command("NextBuffer"), tm)) return 0;

   if (GetObject()->fIter.NextEvent())
      return GetObject()->fIter.evnt();

   return 0;
}

mbs::EventHeader* mbs::ReadoutHandle::GetEvent()
{
  return null() ? 0 : GetObject()->fIter.evnt();
}

// ============================================================================================

mbs::MonitorHandle mbs::MonitorHandle::Connect(const std::string& mbsnode, int cmdport, int logport)
{
   if (dabc::mgr.null()) {
      dabc::SetDebugLevel(-1);
      dabc::CreateManager("dabc", -1);
   }

   int cnt = 0;
   std::string name;
   do {
      name = dabc::format("MbsCtrl%d", cnt);
   } while (!dabc::mgr.FindModule(name).null());

   dabc::CmdCreateModule cmd("mbs::Monitor", name);
   cmd.SetStr("node", mbsnode);
   cmd.SetInt("logger", logport);
   cmd.SetInt("cmd", cmdport);
   cmd.SetBool("publish", false);
   cmd.SetBool("printf", true);

   if (!dabc::mgr.Execute(cmd)) return 0;

   mbs::MonitorHandle mdl = dabc::mgr.FindModule(name);

   mdl.Start();

   return mdl;
}

bool mbs::MonitorHandle::Disconnect()
{
   if (null()) return false;

   Stop();

   std::string name = GetName();

   Release();

   dabc::mgr.DeleteModule(name);

   return true;
}

bool mbs::MonitorHandle::MbsCmd(const std::string& mbscmd, double tmout)
{
   if (null()) return false;

   dabc::Command cmd = dabc::CmdHierarchyExec("CmdMbs");
   cmd.SetStr("cmd", mbscmd);
   cmd.SetTimeout(tmout);

   return Execute(cmd);
}
