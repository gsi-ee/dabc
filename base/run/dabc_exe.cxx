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

#include <stdlib.h>
#include <signal.h>

#include "dabc/logging.h"
#include "dabc/threads.h"
#include "dabc/Manager.h"
#include "dabc/Application.h"
#include "dabc/Configuration.h"
#include "dabc/Factory.h"
#include "dabc/ReferencesVector.h"
#include "dabc/api.h"

#include "dabc/CpuInfoModule.h"

dabc::Thread_t SigThrd = 0;
int SigCnt = 0;

void dabc_CtrlCHandler(int number)
{
   if (SigThrd != dabc::PosixThread::Self()) return;

   SigCnt++;

   if ((SigCnt>2) || (dabc::mgr()==0)) {
      EOUT("Force application exit");
      dabc::lgr()->CloseFile();
      exit(0);
   }

   dabc::mgr()->ProcessCtrlCSignal();
}

bool dabc_InstallCtrlCHandler()
{
   if (SigThrd!=0) {
      EOUT("Signal handler was already installed !!!");
      return false;
   }

   SigThrd = dabc::PosixThread::Self();

   if (signal(SIGINT, dabc_CtrlCHandler)==SIG_ERR) {
      EOUT("Cannot change handler for SIGINT");
      return false;
   }

   DOUT2("Install Ctrl-C handler from thrd %d", SigThrd);

   return true;
}



int RunApplication(dabc::Configuration& cfg, int nodeid, int numnodes, bool dorun)
{
   if (dabc::mgr.null()) return 1;

   int cpuinfo = cfg.ShowCpuInfo();
   if (cpuinfo>=0) {
      dabc::mgr.CreateModule("dabc::CpuInfoModule", "/CpuInfo", dabc::Manager::MgrThrdName());
      dabc::mgr.StartModule("/CpuInfo");
   }

   int ctrl = cfg.UseControl();
   std::string master = cfg.MasterName();

   if ((ctrl==0) && (cfg.NumNodes()>1)) ctrl = 1;

   if ((ctrl > 0) || !master.empty()) {
      DOUT2("Connecting control");
      if (!dabc::mgr()->CreateControl(ctrl>0)) {
         EOUT("Cannot establish connection to command system");
         return 1;
      }

      if (!master.empty()) {
         dabc::Command cmd("ConfigureMaster");
         cmd.SetStr("Master", master);
         cmd.SetStr("NameSufix", "DABC");
         if (dabc::mgr.GetCommandChannel().Execute(cmd) != dabc::cmd_true) {
            DOUT0("FAIL to activate connection to master %s", master.c_str());
            return 1;
         }
      }
   }

   // activate application only with non-controlled mode

   dabc::mgr.app().Submit(dabc::CmdInvokeAppRun());

   DOUT0("Application mainloop is now running");
   DOUT0("       Press Ctrl-C for stop");

   // manager main loop will be run for specified time
   // at the exit application either stopped or will be requested to stop

   dabc::mgr.RunMainLoop(cfg.GetRunTime());

   return 0;
}

int command_shell(const char* node)
{
   dabc::CreateManager("cmd", 0);

   dabc::mgr.RunCmdLoop(1000, node);

   dabc::DestroyManager();

   return 0;
}


int main(int numc, char* args[])
{
   dabc_InstallCtrlCHandler();

//   dabc::SetDebugLevel(0);

   DOUT2("Start  cnt = %u", dabc::Object::NumInstances());

   if ((numc>1) and (strcmp(args[1],"cmd")==0)) {
      return command_shell(numc>2 ? args[2] : "");
   }

   const char* cfgfile(0);

   if(numc > 1) cfgfile = args[1];


   unsigned nodeid = 1000000;
   unsigned numnodes = 0;
   bool dorun = false;

   dabc::Configuration cfg(cfgfile);
   if (!cfg.IsOk()) {
      EOUT("Cannot read configuration from file %s - Exit", (cfgfile ? cfgfile : "---") );
      return 7;
   }

   if (cfg.GetVersion() != 2) {
      EOUT("Only dabc version 2 for xml files is supported - Exit");
      return 7;
   }

   int cnt = 2;
   while (cnt<numc) {

      const char* arg = args[cnt++];

      if (strcmp(arg,"-nodeid")==0) {
         if (cnt < numc)
            nodeid = (unsigned) atoi(args[cnt++]);
      } else
      if (strcmp(arg,"-numnodes")==0) {
         if (cnt < numc)
            numnodes = (unsigned) atoi(args[cnt++]);
      } else
      if (strcmp(arg,"-run")==0)
         dorun = true;
      else
      if (strcmp(arg,"-norun")==0)
         dorun = false;
   }

   if (numnodes==0) numnodes = cfg.NumNodes();
   if (nodeid >= numnodes) nodeid = 0;

   DOUT2("Using config file: %s id: %u", cfgfile, nodeid);

   if (!cfg.SelectContext(nodeid, numnodes)) {
      EOUT("Did not found context");
      return 1;
   }

   // reserve special thread
   std::string affinity = cfg.Affinity();
   if (!affinity.empty())
      dabc::PosixThread::SetDfltAffinity(affinity.c_str());
   else
      dabc::PosixThread::SetDfltAffinity();

   char sbuf[200];
   if (dabc::PosixThread::GetDfltAffinity(sbuf, sizeof(sbuf)))
      DOUT2("Process affinity %s", sbuf);

   DOUT2("Create manager");

   new dabc::Manager(cfg.MgrName(), &cfg);

   // ensure that all submitted events are processed
   dabc::mgr.SyncWorker();

   int res = 0;

   if (!cfg.LoadLibs()) res = -2;

   if (res==0)
      dabc::mgr.Execute("InitFactories");

   if (cfg.WithPublisher() > 0)
      dabc::mgr.CreatePublisher();

   dabc::Application::ExternalFunction* runfunc =
         (dabc::Application::ExternalFunction*)
         dabc::Factory::FindSymbol(cfg.RunFuncName());

   if (runfunc!=0) {
      if (res==0) runfunc();
   } else {
      if (res==0)
         if (!dabc::mgr.CreateApplication(cfg.ConetextAppClass())) res = -3;

      if (res==0)
         res = RunApplication(cfg, nodeid, numnodes, dorun);
   }

   dabc::mgr()->HaltManager();

   dabc::mgr.Destroy();

   dabc::Object::InspectGarbageCollector();

   DOUT2("Exit  cnt = %u", dabc::Object::NumInstances());

   return res;
}
