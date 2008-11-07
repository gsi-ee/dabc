#include "dabc/logging.h"
#include "dabc/Manager.h"
#ifdef __USE_STANDALONE__
#include "dabc/StandaloneManager.h"
#endif

int RunApplication()
{
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

   return 0;
}

int main(int numc, char* args[])
{
   dabc::SetDebugLevel(1);

   const char* configuration = "SetupRoc.xml";
   const char* appclass = 0;

   if(numc > 1) configuration = args[1];
   if(numc > 2) appclass = args[2];

   DOUT1(("Using config file: %s", configuration));

   dabc::Manager manager("dabc", true);

   dabc::Logger::Instance()->LogFile("dabc.log");

   manager.InstallCtrlCHandler();

   manager.Read_XDAQ_XML_Libs(configuration);

   if (!manager.CreateApplication(appclass)) {
      EOUT(("Cannot create application of specified class %s", (appclass ? appclass : "???")));
      return 1;
   }

   manager.Read_XDAQ_XML_Pars(configuration);

   int res = RunApplication();

   dabc::Logger::Instance()->ShowStat();

   return res;
}
