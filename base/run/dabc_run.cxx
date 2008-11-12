#include "dabc/logging.h"
#include "dabc/statistic.h"
#include "dabc/Manager.h"
#include "dabc/Configuration.h"
#include "dabc/Factory.h"

#include <iostream>

int RunSimpleApplication(dabc::Configuration* cfg, const char* appclass)
{
   dabc::Manager manager("dabc", true);

   manager.InstallCtrlCHandler();

   cfg->LoadLibs();

   if (!manager.CreateApplication(appclass)) {
      EOUT(("Cannot create application of specified class %s", (appclass ? appclass : "???")));
      return 1;
   }

   cfg->ReadPars();

   // set states of manager to running here:
   if(!manager.DoStateTransition(dabc::Manager::stcmdDoConfigure)) {
      EOUT(("State transition %s failed. Abort", dabc::Manager::stcmdDoConfigure));
      return 1;
   }
   DOUT1(("Did configure"));

   if(!manager.DoStateTransition(dabc::Manager::stcmdDoEnable)) {
      EOUT(("State transition %s failed. Abort", dabc::Manager::stcmdDoEnable));
      return 1;
   }
   DOUT1(("Did enable"));

   if(!manager.DoStateTransition(dabc::Manager::stcmdDoStart)) {
      EOUT(("State transition %s failed. Abort", dabc::Manager::stcmdDoStart));
      return 1;
   }

   DOUT1(("Application mainloop is now running"));
   DOUT1(("       Press ctrl-C for stop"));

   manager.RunManagerMainLoop();

//   while(dabc::mgr()->GetApp()->IsModulesRunning()) { ::sleep(1); }
//   sleep(10);

   DOUT1(("Normal finish of mainloop"));

   if(!manager.DoStateTransition(dabc::Manager::stcmdDoStop)) {
      EOUT(("State transition %s failed. Abort", dabc::Manager::stcmdDoStop));
      return 1;
   }

   if(!manager.DoStateTransition(dabc::Manager::stcmdDoHalt)) {
      EOUT(("State transition %s failed. Abort", dabc::Manager::stcmdDoHalt));
      return 1;
   }

   dabc::Logger::Instance()->ShowStat();

   return 0;
}

bool SMChange(const char* smcmdname)
{
   dabc::CommandClient cli;

   dabc::Command* cmd = new dabc::CommandStateTransition(smcmdname);

   if (!dabc::mgr()->InvokeStateTransition(smcmdname, cli.Assign(cmd))) return false;

   bool res = cli.WaitCommands(10);

   if (!res) {
      EOUT(("State change %s fail EXIT!!!! ", smcmdname));

      EOUT(("EMERGENCY EXIT!!!!"));

      exit(1);
   }

   return res;
}

bool RunBnetTest()
{
   dabc::CpuStatistic cpu;

   DOUT1(("RunTest start"));

//   ChangeRemoteParameter(m, 2, "Input0Cfg", "ABB");

   SMChange(dabc::Manager::stcmdDoConfigure);

   DOUT1(("Create done"));

   SMChange(dabc::Manager::stcmdDoEnable);

   DOUT1(("Connection done"));

   SMChange(dabc::Manager::stcmdDoStart);

   cpu.Reset();

   dabc::ShowLongSleep("Main loop", 15); //15

   SMChange(dabc::Manager::stcmdDoStop);

   sleep(1);

   SMChange(dabc::Manager::stcmdDoStart);

   dabc::ShowLongSleep("Again main loop", 10); //10

   cpu.Measure();

   DOUT1(("Calling stop"));

   SMChange(dabc::Manager::stcmdDoStop);

   DOUT1(("Calling halt"));

   SMChange(dabc::Manager::stcmdDoHalt);

   DOUT1(("CPU usage %5.1f", cpu.CPUutil()*100.));

   DOUT1(("RunTest done"));

   return true;
}


int main(int numc, char* args[])
{
   dabc::SetDebugLevel(1);

   const char* configuration = "SetupRoc.xml";
   const char* appclass = 0;
   const char* workdir = 0;
   const char* logfile = 0;

   if(numc > 1) configuration = args[1];

   unsigned nodeid = 0;
   unsigned numnodes = 0;
   unsigned configid = 0;

   const char* connid = 0;

   dabc::Configuration cfg(configuration);
   if (!cfg.IsOk()) return 7;

   int cnt = 2;
   while (cnt<numc) {

      const char* arg = args[cnt++];

      if (strcmp(arg,"-workdir")==0) {
         if (cnt < numc)
            workdir = args[cnt++];
      } else
      if (strcmp(arg,"-logfile")==0) {
         if (cnt < numc)
            logfile = args[cnt++];
      } else
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
         if (appclass==0) appclass = arg;
   }

   if (numnodes==0) numnodes = cfg.NumNodes();

   if (logfile!=0)
      dabc::Logger::Instance()->LogFile(logfile);

   DOUT1(("Using config file: %s id: %u", configuration, configid));

   if (!cfg.SelectContext(configid, nodeid, numnodes)) {
      EOUT(("Did not found context"));
      return 1;
   }

   if (numnodes<2)
      return RunSimpleApplication(&cfg, appclass);

   if (!dabc::Manager::LoadLibrary("${DABCSYS}/lib/libDabcSctrl.so")) {
      EOUT(("Cannot load control library"));
      return 1;
   }

   if (!dabc::Factory::CreateManager("Standalone", 0, nodeid, numnodes)) {
      EOUT(("Cannot create required manager class"));
      return 1;
   }

//   dabc::StandaloneManager manager(0, nodeid, numnodes, true);

   DOUT0(("Run cluster application!!! %u %u %s", nodeid, numnodes, (connid ? connid : "---")));

   dabc::mgr()->InstallCtrlCHandler();

   cfg.LoadLibs();

   if (!dabc::mgr()->CreateApplication(appclass)) {
      EOUT(("Cannot create application %s", appclass));
      delete dabc::mgr();
      return 1;
   }

//   cfg.ReadPars();

   if (!dabc::mgr()->ConnectControl(connid)) {
      EOUT(("Cannot establish connection to control system"));
      delete dabc::mgr();
      return 1;
   }

   //((dabc::StandaloneManager*) dabc::mgr())->ConnectCmdChannel(numnodes, 1, connid);

   if (nodeid==0) {
      if (!dabc::mgr()->HasClusterInfo()) {
         EOUT(("Cannot access cluster information from main node"));
         delete dabc::mgr();
         return 1;
      }

      dabc::SetDebugLevel(1);

      RunBnetTest();

      //dabc::ShowLongSleep("Main loop", 5); //15

      //SMChange(manager, dabc::Manager::stcmdDoHalt);
   }

   delete dabc::mgr();

   return 0;
}
