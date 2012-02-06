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

#include "dabc/CpuInfoModule.h"

dabc::Thread_t SigThrd = 0;
int SigCnt = 0;

void dabc_CtrlCHandler(int number)
{
   if (SigThrd != dabc::PosixThread::Self()) return;

   SigCnt++;

   if ((SigCnt>2) || (dabc::mgr()==0)) {
      EOUT(("Force application exit"));
      dabc::lgr()->CloseFile();
      exit(0);
   }

   dabc::mgr()->ProcessCtrlCSignal();
}

bool dabc_InstallCtrlCHandler()
{
   if (SigThrd!=0) {
      EOUT(("Signal handler was already installed !!!"));
      return false;
   }

   SigThrd = dabc::PosixThread::Self();

   if (signal(SIGINT, dabc_CtrlCHandler)==SIG_ERR) {
      EOUT(("Cannot change handler for SIGINT"));
      return false;
   }

   DOUT2(("Install Ctrl-C handler from thrd %d", SigThrd));

   return true;
}



int RunApplication(dabc::Configuration& cfg, int nodeid, int numnodes, bool dorun, bool external_control)
{
   if (dabc::mgr.null()) return 1;

   int cpuinfo = cfg.ShowCpuInfo();
   if (cpuinfo>=0) {
      dabc::mgr.CreateModule("dabc::CpuInfoModule", "/CpuInfo", dabc::Manager::MgrThrdName());
      dabc::mgr.StartModule("/CpuInfo");
   }

   dabc::Application::ExternalFunction* runfunc =
         (dabc::Application::ExternalFunction*)
         dabc::Factory::FindSymbol(cfg.RunFuncName());

   if (runfunc!=0) {
      runfunc();
      return 0;
   }

   DOUT2(("ConnectControl"));
   if (!dabc::mgr()->ConnectControl()) {
      EOUT(("Cannot establish connection to command system"));
      return 1;
   }

   // activate application only with non-controlled mode
   if (!external_control) {

      dabc::mgr.GetApp().Submit(dabc::InvokeAppRunCmd());

      DOUT0(("Application mainloop is now running"));
      DOUT0(("       Press Ctrl-C for stop"));
   }

   // manager main loop will be run for specified time
   // at the exit application either stopped or will be requested to stop

   dabc::mgr()->RunManagerMainLoop(cfg.GetRunTime(), external_control);

   return 0;
}


int main(int numc, char* args[])
{
   dabc_InstallCtrlCHandler();

//   dabc::SetDebugLevel(0);

   DOUT2(("Start  cnt = %u", dabc::Object::NumInstances()));

   const char* cfgfile(0);

   if(numc > 1) cfgfile = args[1];

   unsigned nodeid = 1000000;
   unsigned numnodes = 0;
   unsigned configid = 0;
   bool dorun = false;
   bool external_control = false;

   dabc::Configuration cfg(cfgfile);
   if (!cfg.IsOk()) {
      EOUT(("Cannot read configuration from file %s - Exit", (cfgfile ? cfgfile : "---")));
      return 7;
   }

   int cnt = 2;
   while (cnt<numc) {

      const char* arg = args[cnt++];

      if (strcmp(arg,"-cfgid")==0) {
         if (cnt < numc)
            configid = (unsigned) atoi(args[cnt++]);
      } else
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

   if (numnodes==0) numnodes = cfg.NumControlNodes();
   if (nodeid > numnodes) nodeid = configid;

   DOUT2(("Using config file: %s id: %u", cfgfile, configid));

   if (!cfg.SelectContext(configid, nodeid, numnodes)) {
      EOUT(("Did not found context"));
      return 1;
   }

   DOUT2(("Create manager"));

   new dabc::Manager(cfg.MgrName(), &cfg);

   // ensure that all submitted events are processed
   dabc::mgr.SyncWorker();

   int res = 0;

   if (!cfg.LoadLibs()) res = -2;

   if (res==0)
      dabc::mgr.Execute("InitFactories");

   // TODO: is some situations application is not required
   if (res==0)
     if (!dabc::mgr()->CreateApplication(cfg.ConetextAppClass())) res = -3;

   if (res==0) {

      res = RunApplication(cfg, nodeid, numnodes, dorun, external_control);
   }

   dabc::mgr()->HaltManager();

   dabc::mgr.Destroy();

   dabc::Object::InspectGarbageCollector();

   DOUT2(("Exit  cnt = %u", dabc::Object::NumInstances()));

   return res;
}
