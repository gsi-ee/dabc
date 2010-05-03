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
#include <stdlib.h>
#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Application.h"
#include "dabc/Configuration.h"
#include "dabc/Factory.h"

#include "dabc/CpuInfoModule.h"

extern "C" void ClassicalRunFunction()
{
   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoConfigure);

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoEnable);

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoStart);

   DOUT1(("Application mainloop is now running"));
   DOUT1(("       Press Ctrl-C for stop"));

   dabc::mgr()->RunManagerMainLoop(dabc::mgr()->cfg()->GetRunTime());

   DOUT1(("Normal finish of mainloop"));

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoStop);

   // this is workaround for roc transport - it is not fast enough to get reply on put(stop_daq) command
   dabc::MicroSleep(200000);

   dabc::mgr()->ChangeState(dabc::Manager::stcmdDoHalt);
}


int RunSimpleApplication(dabc::Configuration& cfg)
{
   dabc::Application::ExternalFunction* runfunc =
      (dabc::Application::ExternalFunction*)
         dabc::Factory::FindSymbol(cfg.RunFuncName());

   if (runfunc==0) ClassicalRunFunction();
              else runfunc();

   return 0;
}

bool WaitActiveNodes(double tmout)
{
   dabc::TimeStamp_t t1 = TimeStamp();

   do {
     if (dabc::mgr()->NumActiveNodes() == dabc::mgr()->NumNodes())
        if (dabc::mgr()->TestActiveNodes()) return true;

     dabc::MicroSleep(10000);

   } while (dabc::TimeDistance(t1, TimeStamp()) < tmout);

   EOUT(("Cannot connect to all active nodes !!!"));

   return false;
}


int RunSctrlApplication(dabc::Configuration& cfg, const char* connid, int nodeid, int numnodes)
{
   DOUT1(("Run application node:%d numnodes:%d conn:%s", nodeid, numnodes, (connid ? connid : "---")));

   if (connid!=0)
       if (!dabc::mgr()->ConnectControl(connid)) {
          EOUT(("Cannot establish connection to control system"));
          return 1;
       }

   if ((nodeid==0) && !WaitActiveNodes(10.)) return 1;

   dabc::Application::ExternalFunction* runfunc =
      (dabc::Application::ExternalFunction*)
         dabc::Factory::FindSymbol(cfg.RunFuncName());

   if (runfunc!=0)
      runfunc();
   else
   if (!dabc::mgr()->GetApp()->IsSlaveApp())
      ClassicalRunFunction();

   return 0;
}

int RunDimApplication(dabc::Configuration& cfg, int nodeid, bool dorun)
{
   DOUT1(("Run cluster DIM application node %d!!!", nodeid));

   int cpuinfo = cfg.ShowCpuInfo();
   if (cpuinfo>=0) {
      dabc::CpuInfoModule* m = new dabc::CpuInfoModule("CpuInfo", 0, cpuinfo);
      m->SetAppId(76);
      m->AssignProcessorToThread(dabc::mgr()->ProcessorThread());
      m->Start();
   }

   if (dorun) {

      if ((nodeid==0) && !WaitActiveNodes(10.)) return 1;

      dabc::Application::ExternalFunction* runfunc =
         (dabc::Application::ExternalFunction*)
            dabc::Factory::FindSymbol(cfg.RunFuncName());
      if (runfunc!=0)
         runfunc();
      else
      if (!dabc::mgr()->GetApp()->IsSlaveApp())
         ClassicalRunFunction();
      else
         dabc::mgr()->RunManagerMainLoop();
   } else
      dabc::mgr()->RunManagerMainLoop();


   return 0;
}


int main(int numc, char* args[])
{
   dabc::SetDebugLevel(1);

   const char* configuration = "SetupRoc.xml";

   if(numc > 1) configuration = args[1];

   unsigned nodeid = 1000000;
   unsigned numnodes = 0;
   unsigned configid = 0;
   bool dorun = false;

   const char* connid = 0;

   dabc::Configuration cfg(configuration);
   if (!cfg.IsOk()) return 7;
   int ctrlkind = dabc::ConfigBase::kindNone;
   const char* dimnode = 0;

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
      if (strcmp(arg,"-conn")==0) {
         if (cnt < numc)
            connid = args[cnt++];
      } else
      if (strcmp(arg,"-sctrl")==0) {
         ctrlkind = dabc::ConfigBase::kindSctrl;
      } else
      if (strcmp(arg,"-dim")==0) {
         ctrlkind = dabc::ConfigBase::kindDim;
         if (cnt < numc)
            dimnode = args[cnt++];
      } else
      if (strcmp(arg,"-run")==0)
         dorun = true;
      else
      if (strcmp(arg,"-norun")==0)
         dorun = false;
   }

   if (numnodes==0) numnodes = cfg.NumControlNodes();
   if (nodeid > numnodes) nodeid = configid;

   DOUT1(("Using config file: %s id: %u conn: %s", configuration, configid, connid));

   if (!cfg.SelectContext(configid, nodeid, numnodes, dimnode)) {
      EOUT(("Did not found context"));
      return 1;
   }

   const char* mgrclass = "";

   if ((ctrlkind == dabc::ConfigBase::kindNone) && (numnodes > 1) && (cfg.ControlSequenceId(configid)>0))
      ctrlkind = dabc::ConfigBase::kindSctrl;

   if (ctrlkind == dabc::ConfigBase::kindDim) {
      if (!dabc::Factory::LoadLibrary(cfg.ResolveEnv("${DABCSYS}/lib/libDabcDimCtrl.so"))) {
         EOUT(("Cannot load dim control library"));
         return 1;
      }
      connid = 0;
      mgrclass = "DimControl";
   } else
   if (ctrlkind == dabc::ConfigBase::kindSctrl) {
      if (!dabc::Factory::LoadLibrary(cfg.ResolveEnv("${DABCSYS}/lib/libDabcSctrl.so"))) {
         EOUT(("Cannot load control library"));
         return 1;
      }
      mgrclass = "Standalone";
   } else {
      mgrclass = "BasicExtra";
   }

   DOUT2(("Create manager class %s", mgrclass));

   if (!dabc::Factory::CreateManager(mgrclass, &cfg)) {
      EOUT(("Cannot create required manager class %s", mgrclass));
      return 1;
   }
   // ensure that all submitted events are processed
   dabc::mgr()->SyncProcessor();

   dabc::mgr()->InstallCtrlCHandler();

   int res = 0;

   if (!cfg.LoadLibs()) res = -2;

   if (res==0)
     if (!dabc::mgr()->CreateApplication(cfg.ConetextAppClass())) res = -3;

   if (res==0) {
      if (ctrlkind == dabc::ConfigBase::kindDim)
         res = RunDimApplication(cfg, nodeid, dorun);
      else
      if ((numnodes<2) || (ctrlkind == dabc::ConfigBase::kindNone))
         res = RunSimpleApplication(cfg);
      else
         res = RunSctrlApplication(cfg, connid, nodeid, numnodes);
   }

   delete dabc::mgr();

   return res;
}
