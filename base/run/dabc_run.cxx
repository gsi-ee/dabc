#include "dabc/logging.h"
#include "dabc/Manager.h"
#ifdef __USE_STANDALONE__
#include "dabc/StandaloneManager.h"
#endif

#include <iostream>

int RunSimpleApplication(const char* configuration, const char* appclass)
{
   dabc::Manager manager("dabc", true);

   dabc::Logger::Instance()->LogFile("dabc.log");

   manager.InstallCtrlCHandler();

   manager.Read_XDAQ_XML_Libs(configuration);

   if (!manager.CreateApplication(appclass)) {
      EOUT(("Cannot create application of specified class %s", (appclass ? appclass : "???")));
      return 1;
   }

   manager.Read_XDAQ_XML_Pars(configuration);

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

bool SMChange(dabc::Manager& m, const char* smcmdname)
{
   dabc::CommandClient cli;

   dabc::Command* cmd = new dabc::CommandStateTransition(smcmdname);

   if (!m.InvokeStateTransition(smcmdname, cli.Assign(cmd))) return false;

   bool res = cli.WaitCommands(10);

   if (!res) {
      EOUT(("State change %s fail EXIT!!!! ", smcmdname));

      EOUT(("EMERGENCY EXIT!!!!"));

      exit(1);
   }

   return res;
}

bool RunBnetTest(dabc::Manager& m)
{
   dabc::CpuStatistic cpu;

   DOUT1(("RunTest start"));

//   ChangeRemoteParameter(m, 2, "Input0Cfg", "ABB");

   SMChange(m, dabc::Manager::stcmdDoConfigure);

   DOUT1(("Create done"));

   SMChange(m, dabc::Manager::stcmdDoEnable);

   DOUT1(("Connection done"));

   SMChange(m, dabc::Manager::stcmdDoStart);

   cpu.Reset();

   dabc::ShowLongSleep("Main loop", 15); //15

   SMChange(m, dabc::Manager::stcmdDoStop);

   sleep(1);

   SMChange(m, dabc::Manager::stcmdDoStart);

   dabc::ShowLongSleep("Again main loop", 10); //10

   cpu.Measure();

   DOUT1(("Calling stop"));

   SMChange(m, dabc::Manager::stcmdDoStop);

   DOUT1(("Calling halt"));

   SMChange(m, dabc::Manager::stcmdDoHalt);

   DOUT1(("CPU usage %5.1f", cpu.CPUutil()*100.));

   DOUT1(("RunTest done"));

   return true;
}


int main(int numc, char* args[])
{
   dabc::SetDebugLevel(1);

   const char* configuration = "SetupRoc.xml";
   const char* appclass = 0;

   if(numc > 1) configuration = args[1];

   int nodeid = 0;
   int numnodes = 1;
   const char* connid = 0;

   int cnt = 2;
   while (cnt<numc) {

      const char* arg = args[cnt++];

      if (strcmp(arg,"-number")==0) {
         std::cout << dabc::Manager::Read_XDAQ_XML_NumNodes(configuration);
         std::cout.flush();
         return 0;
      } else
      if (strstr(arg,"-name")==arg) {
         arg+=5;
         unsigned cnt = *arg ? atoi(arg) : 0;
         std::cout << dabc::Manager::Read_XDAQ_XML_NodeName(configuration, cnt);
         std::cout.flush();
         return 0;
      } else
      if (strstr(arg,"-id")==arg) {
         nodeid = atoi(arg+3);
      } else
      if (strstr(arg,"-num")==arg) {
         numnodes = atoi(arg+4);
      } else
      if (strstr(arg,"-conn")==arg) {
         connid = arg+5;
      } else
         if (appclass==0) appclass = arg;
   }

   DOUT1(("Using config file: %s", configuration));

   if (numnodes<2)
      return RunSimpleApplication(configuration, appclass);

   dabc::StandaloneManager manager(nodeid, numnodes, true);

   DOUT0(("Run cluster application!!! %d %d %s", nodeid, numnodes, (connid ? connid : "---")));

   dabc::Logger::Instance()->LogFile(FORMAT(("LogFile%d.log", nodeid)));

   manager.InstallCtrlCHandler();

   manager.Read_XDAQ_XML_Libs(configuration, nodeid);

   if (!manager.CreateApplication(appclass)) {
      EOUT(("Cannot create application %s", appclass));
      return 1;
   }

   manager.ConnectCmdChannel(numnodes, 1, connid);

   if (nodeid==0) {
      if (!manager.HasClusterInfo()) {
         EOUT(("Cannot access cluster information from main node"));
         return 1;
      }

      dabc::SetDebugLevel(1);

      RunBnetTest(manager);

      //dabc::ShowLongSleep("Main loop", 5); //15

      //SMChange(manager, dabc::Manager::stcmdDoHalt);
   }

   return 0;
}
