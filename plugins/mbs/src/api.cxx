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

mbs::ReadoutModule::ReadoutModule(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fIter(),
   fLastNotFullTm(),
   fCurBufTm(),
   fCmd()
{
   EnsurePorts(1, 0, dabc::xmlWorkPool);

   SetPortSignaling(InputName(), dabc::Port::SignalEvery);

   CreateTimer("SysTimer");

   SetAutoStop(false);
}

int mbs::ReadoutModule::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("NextBuffer")) {
      // previous command not processed - cannot be
      if (!fCmd.null()) return dabc::cmd_false;

      fCmd = cmd;
      double tm = fCmd.TimeTillTimeout();

      if (CanRecv() || (tm<=0))
         ProcessData();
      else
         ShootTimer(0, tm);

      return dabc::cmd_postponed;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

void mbs::ReadoutModule::ProcessData()
{
   // ignore input event as long as command is not specified
   if (fCmd.null()) return;

   double maxage = fCmd.GetDouble("maxage", -1);

   int res = dabc::cmd_false;

   dabc::TimeStamp now = dabc::Now();

   bool cleanqueue = false;
   if ((maxage>0) && InputQueueFull())
      if (fLastNotFullTm.null() || (now - fLastNotFullTm > maxage)) {
         cleanqueue = true;
      }

   if (cleanqueue) fLastNotFullTm = now;

   while (CanRecv()) {
      dabc::Buffer buf = Recv();

      fCurBufTm = now;

      // when EOF buffer received, return immediately stop
      if (buf.GetTypeId() == dabc::mbt_EOF) {
         fCmd.Reply(dabc::cmd_false);
         return;
      }

      // clean queue when it was full too long
      // last buffer will be preserved
      if (cleanqueue && CanRecv()) {
         buf.Release();
         continue;
      }

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

      break;
   }

   fCmd.Reply(res);
}


void mbs::ReadoutModule::ProcessInputEvent(unsigned)
{
   if (!InputQueueFull()) fLastNotFullTm.GetNow();

   ProcessData();
}

void mbs::ReadoutModule::ProcessTimerEvent(unsigned)
{
   // if timeout happened, reply
   ProcessData();
}

bool mbs::ReadoutModule::GetEventInTime(double maxage)
{
   if ((maxage <= 0) || fCurBufTm.null())
      return true;
   return !fCurBufTm.Expired(maxage);
}


// ===================================================================================

mbs::ReadoutHandle mbs::ReadoutHandle::DoConnect(const std::string &url, const char *classname, int bufsz_mb)
{
   if (dabc::mgr.null()) {
      dabc::SetDebugLevel(-1);
      dabc::CreateManager("dabc", -1);
   }

   if (dabc::mgr.FindPool(dabc::xmlWorkPool).null()) {
      if (bufsz_mb < 1)
         bufsz_mb = 1;
      if (!dabc::mgr.CreateMemoryPool(dabc::xmlWorkPool, bufsz_mb*1024*1024, 40)) {
         return nullptr;
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
      return nullptr;
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

   // add timeout to let cleanup DABC sockets
   dabc::Sleep(0.2);

   return true;
}


mbs::EventHeader* mbs::ReadoutHandle::NextEvent(double tmout, double maxage)
{
   if (null()) return nullptr;

   bool intime = GetObject()->GetEventInTime(maxage);

   if (intime && GetObject()->fIter.NextEvent())
      return GetObject()->fIter.evnt();

   dabc::Command cmd("NextBuffer");
   // here maxage means cleanup of complete queue when queue was full for long time
   cmd.SetDouble("maxage", 2.*maxage);

   if (!Execute(cmd, tmout))
      return nullptr;

   if (GetObject()->fIter.NextEvent())
      return GetObject()->fIter.evnt();

   return nullptr;
}

mbs::EventHeader* mbs::ReadoutHandle::GetEvent()
{
  return null() ? nullptr : GetObject()->fIter.evnt();
}

// ============================================================================================

mbs::MonitorHandle mbs::MonitorHandle::Connect(const std::string &mbsnode, int cmdport, int logport, int statport)
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
   cmd.SetInt("stat", statport);
   cmd.SetBool("publish", false);
   cmd.SetBool("printf", true);

   if (!dabc::mgr.Execute(cmd))
      return nullptr;

   mbs::MonitorHandle mdl = dabc::mgr.FindModule(name);

   mdl.Start();

   return mdl;
}

bool mbs::MonitorHandle::Disconnect()
{
   if (null()) return false;

   Execute("DeleteWorkers");

   Stop();

   std::string name = GetName();

   Release();

   dabc::mgr.DeleteModule(name);

   // add timeout to let cleanup DABC sockets
   dabc::Sleep(0.2);

   return true;
}

bool mbs::MonitorHandle::MbsCmd(const std::string &mbscmd, double tmout)
{
   if (null()) return false;

   dabc::Command cmd = dabc::CmdHierarchyExec("CmdMbs");
   cmd.SetStr("cmd", mbscmd);
   cmd.SetTimeout(tmout);

   return Execute(cmd);
}
