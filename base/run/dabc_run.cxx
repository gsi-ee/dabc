#include "dabc/logging.h"
#include "dabc/statistic.h"
#include "dabc/Manager.h"
#include "dabc/Configuration.h"
#include "dabc/Factory.h"

#include <iostream>

int RunSimpleFunc(dabc::Configuration* cfg, std::string funcname)
{
   cfg->LoadLibs(funcname.c_str());

   cfg->StoreObject((funcname + ".xml").c_str(), dabc::mgr());

   DOUT0(("Start main loop"));

   dabc::mgr()->RunManagerMainLoop();

   DOUT0(("Finish main loop"));

   return 0;
}

int RunSimpleApplication(dabc::Configuration* cfg)
{
   cfg->LoadLibs();

   if (!dabc::mgr()->CreateApplication()) {
      EOUT(("Cannot create application"));
      return 1;
   }

   cfg->ReadPars();

//   cfg->StoreObject("Manager.xml", dabc::mgr());

   // set states of manager to running here:
   if(!dabc::mgr()->DoStateTransition(dabc::Manager::stcmdDoConfigure)) {
      EOUT(("State transition %s failed. Abort", dabc::Manager::stcmdDoConfigure));
      return 1;
   }
   DOUT1(("Did configure"));

   if(!dabc::mgr()->DoStateTransition(dabc::Manager::stcmdDoEnable)) {
      EOUT(("State transition %s failed. Abort", dabc::Manager::stcmdDoEnable));
      return 1;
   }
   DOUT1(("Did enable"));

   if(!dabc::mgr()->DoStateTransition(dabc::Manager::stcmdDoStart)) {
      EOUT(("State transition %s failed. Abort", dabc::Manager::stcmdDoStart));
      return 1;
   }

   DOUT1(("Application mainloop is now running"));
   DOUT1(("       Press ctrl-C for stop"));

   cfg->StoreObject("Manager.xml", dabc::mgr());

   dabc::mgr()->RunManagerMainLoop();

//   while(dabc::mgr()->GetApp()->IsModulesRunning()) { ::sleep(1); }
//   sleep(10);

   DOUT1(("Normal finish of mainloop"));

   if(!dabc::mgr()->DoStateTransition(dabc::Manager::stcmdDoStop)) {
      EOUT(("State transition %s failed. Abort", dabc::Manager::stcmdDoStop));
      return 1;
   }

   if(!dabc::mgr()->DoStateTransition(dabc::Manager::stcmdDoHalt)) {
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

int RunClusterApplucation(dabc::Configuration* cfg, const char* connid, int nodeid, int numnodes)
{
   DOUT0(("Run cluster application!!! %d %d %s", nodeid, numnodes, (connid ? connid : "---")));

   cfg->LoadLibs();

   if (!dabc::mgr()->CreateApplication()) {
      EOUT(("Cannot create application"));
      return 1;
   }

   if (connid!=0)
       if (!dabc::mgr()->ConnectControl(connid)) {
          EOUT(("Cannot establish connection to control system"));
          return 1;
       }

   if (nodeid==0) {
       if (!dabc::mgr()->HasClusterInfo()) {
          EOUT(("Cannot access cluster information from main node"));
          return 1;
       }

//       dabc::SetDebugLevel(1);

       dabc::CpuStatistic cpu;

       DOUT1(("RunTest start"));

    //   ChangeRemoteParameter(m, 2, "Input0Cfg", "ABB");

       SMChange(dabc::Manager::stcmdDoConfigure);

       DOUT1(("Create done"));

       //dabc::SetDebugLevel(5);

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
   }
   return 0;
}

int RunClusterDimApplucation(dabc::Configuration* cfg, int nodeid)
{
   DOUT0(("Run cluster DIM application node %d!!!", nodeid));

   cfg->LoadLibs();

   if (!dabc::mgr()->CreateApplication()) {
      EOUT(("Cannot create application"));
      return 1;
   }

   dabc::mgr()->RunManagerMainLoop();

   return 0;
}


int main(int numc, char* args[])
{
   dabc::SetDebugLevel(1);

   const char* configuration = "SetupRoc.xml";
   const char* logfile = 0;

   if(numc > 1) configuration = args[1];

   unsigned nodeid = 1000000;
   unsigned numnodes = 0;
   unsigned configid = 0;

   const char* connid = 0;

   dabc::Configuration cfg(configuration);
   if (!cfg.IsOk()) return 7;
   int ctrlkind = dabc::ConfigBase::kindNone;
   const char* dimnode = 0;

   int cnt = 2;
   while (cnt<numc) {

      const char* arg = args[cnt++];

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
      if (strcmp(arg,"-sctrl")==0) {
         ctrlkind = dabc::ConfigBase::kindSctrl;
      } else
      if (strcmp(arg,"-dim")==0) {
         ctrlkind = dabc::ConfigBase::kindDim;
         if (cnt < numc)
            dimnode = args[cnt++];
      }
   }

   if (numnodes==0) numnodes = cfg.NumControlNodes();
   if (nodeid > numnodes) nodeid = configid;

   DOUT1(("Using config file: %s id: %u", configuration, configid));

   if (!cfg.SelectContext(configid, nodeid, numnodes, logfile, dimnode)) {
      EOUT(("Did not found context"));
      return 1;
   }

   std::string funcname = cfg.StartFuncName();

   const char* mgrclass = "";

   if ((ctrlkind == dabc::ConfigBase::kindNone) && (numnodes > 1) && (funcname.length()==0))
      ctrlkind = dabc::ConfigBase::kindSctrl;

   if (ctrlkind == dabc::ConfigBase::kindDim) {
      if (!dabc::Manager::LoadLibrary("${DABCSYS}/lib/libDabcDimCtrl.so")) {
         EOUT(("Cannot load dim control library"));
         return 1;
      }
      connid = 0;
      mgrclass = "DimControl";
   } else
   if (ctrlkind == dabc::ConfigBase::kindSctrl) {
      if (!dabc::Manager::LoadLibrary("${DABCSYS}/lib/libDabcSctrl.so")) {
         EOUT(("Cannot load control library"));
         return 1;
      }
      mgrclass = "Standalone";
   } else {
      mgrclass = "Basic";
   }

   if (!dabc::Factory::CreateManager(mgrclass, &cfg)) {
      EOUT(("Cannot create required manager class %s", mgrclass));
      return 1;
   }

   dabc::mgr()->InstallCtrlCHandler();

   int res = 0;

   if (funcname.length()>0)
      res = RunSimpleFunc(&cfg, funcname);
   else
   if (numnodes<2)
      res = RunSimpleApplication(&cfg);
   else
   if (ctrlkind == dabc::ConfigBase::kindDim)
      res = RunClusterDimApplucation(&cfg, nodeid);
   else
      res = RunClusterApplucation(&cfg, connid, nodeid, numnodes);

   delete dabc::mgr();

   return res;
}
