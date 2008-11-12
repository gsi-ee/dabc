#include <dlfcn.h>

#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/statistic.h"
#include "dabc/StandaloneManager.h"

#include "bnet/WorkerApplication.h"

void ChangeRemoteParameter(dabc::Manager& m, int nodeid, const char* parname, const char* parvalue)
{
   dabc::Command* cmd = new dabc::CommandSetParameter(parname, parvalue);

   dabc::CommandClient cli;

   m.SubmitRemote(cli, cmd, nodeid, bnet::WorkerApplication::ItemName());

   bool res = cli.WaitCommands(5);

   DOUT1(("Set parameter %s res = %s", parname, DBOOL(res)));

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

bool RunTest(dabc::Manager& m)
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

bool RunTestConn(dabc::Manager& m)
{
   dabc::CpuStatistic cpu;

   DOUT1(("RunTest start"));

   SMChange(m, dabc::Manager::stcmdDoConfigure);

   DOUT1(("Create done"));

   SMChange(m, dabc::Manager::stcmdDoEnable);

   DOUT1(("Connection done"));

/*   SMChange(m, dabc::Manager::stcmdDoStart);

   dabc::ShowLongSleep("Main loop", 3); //15

   SMChange(m, dabc::Manager::stcmdDoStop);

*/
   sleep(1);

   SMChange(m, dabc::Manager::stcmdDoHalt);

   DOUT1(("CPU usage %5.1f", cpu.CPUutil()*100.));

   DOUT1(("RunTestConn done"));

   return true;
}

bool RunTestNew(dabc::Manager& m)
{
   DOUT1(("RunTestNew start"));

   ChangeRemoteParameter(m, 2, "Input0Cfg", "ABB");

   for (int ntry=0; ntry<10; ntry++) {

       SMChange(m, dabc::Manager::stcmdDoConfigure);

       SMChange(m, dabc::Manager::stcmdDoEnable);

       SMChange(m, dabc::Manager::stcmdDoStart);

       dabc::ShowLongSleep("Main loop", 4); //15

       SMChange(m, dabc::Manager::stcmdDoStop);

       SMChange(m, dabc::Manager::stcmdDoHalt);
   }

   DOUT1(("RunTestNew done"));

   return true;
}

bool RunMbsFileTest(dabc::Manager& m)
{
   DOUT1(("RunMbsFileTest start"));

//   ChangeRemoteParameter(m, 2, "Input0Cfg", "ABB");

   for (int node=1;node<m.NumNodes();node++)
      ChangeRemoteParameter(m, node, "StoragePar", "OutType:MbsNewFile; OutName:/tmp/testoutput; SizeLimit:20000000; NumMulti:-1;");

   SMChange(m, dabc::Manager::stcmdDoConfigure);

   SMChange(m, dabc::Manager::stcmdDoEnable);

   SMChange(m, dabc::Manager::stcmdDoStart);

   dabc::ShowLongSleep("Main loop", 4); //15

   SMChange(m, dabc::Manager::stcmdDoStop);

   SMChange(m, dabc::Manager::stcmdDoHalt);

   DOUT1(("RunMbsFileTest done"));

   return true;
}



void CleanupAll(dabc::StandaloneManager &m)
{
   dabc::CommandClient cli;

   for (int node=0;node<m.NumNodes();node++)
      if (m.IsNodeActive(node))
         m.SubmitRemote(cli, new dabc::CmdCleanupManager(), m.GetNodeName(node));
   bool res = cli.WaitCommands(5);
   DOUT1(("CleanupAll res = %s", DBOOL(res)));
}


int main(int numc, char* args[])
{
   dabc::SetDebugLevel(1);

   if (numc<6) {
      EOUT(("Too few arguments for bnet_exe"));
      DOUT1(( "Usage: bnet_exe nodeid numnodes devices controllerId apptype [firstlib secondlib, thrirdlib, ...]"));
      return 1;
   }

   int nodeid = atoi(args[1]);
   int numnodes = atoi(args[2]);
   const char* cfg = args[3];
   int devices = 11;
   while (*cfg!=0) {
      if (*cfg=='u') bnet::NetBidirectional = false; else
      if (*cfg=='b') bnet::NetBidirectional = true; else
      if (*cfg=='g') bnet::UseFileSource = false; else
      if (*cfg=='f') bnet::UseFileSource = true; else
      if ((*cfg>='0') && (*cfg<='9')) {
         devices = atoi(cfg) % 100;
         break;
      }
      cfg++;
   }

   const char* controllerID = args[4];

   if (devices % 10 == 2) bnet::NetDevice = "VerbsDevice";

   const char* appname = args[5];

//   dabc::SetDebugLevel(10);


   dabc::StandaloneManager manager(0, nodeid, numnodes);

   // init plugins

//   dabc::SetDebugLevel(5);

   int argcnt = 6;
   while (argcnt<numc) {
      if (!manager.LoadLibrary(args[argcnt])) return 1;
      argcnt++;
   }

   if (!manager.CreateApplication(appname)) {
      EOUT(("Cannot create application %s", appname));
      return 1;
   }

   manager.ConnectCmdChannel(numnodes, devices / 10, controllerID);


   if (nodeid==0) {
      if (!manager.HasClusterInfo()) {
         EOUT(("Cannot access cluster information from main node"));
         return 1;
      }

      dabc::SetDebugLevel(1);

      RunTest(manager);

//      RunTestConn(manager);

//      RunTestNew(manager);

//      RunMbsFileTest(manager);

//      CleanupAll(manager);
   }

   return 0;
}
