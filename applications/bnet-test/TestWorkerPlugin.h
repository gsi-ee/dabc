#ifndef BNET_TestWorkerPlugin
#define BNET_TestWorkerPlugin

#include "bnet/WorkerPlugin.h"


namespace bnet {
    
   class TestWorkerPlugin : public WorkerPlugin {
      public:
         TestWorkerPlugin(dabc::Manager* m);
         
 
         
         virtual dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command* cmd);
         
         virtual bool CreateReadout(const char* portname, int portnumber);
         
         virtual dabc::Module* CreateCombiner(const char* name);
         virtual dabc::Module* CreateBuilder(const char* name);
         virtual dabc::Module* CreateFilter(const char* name);
         
         bool CreateStorage(const char* portname);
         
     
    private: 
         bool fABBActive;   
         
   }; 
}

extern "C" void InitUserPlugin(dabc::Manager* mgr);


#endif
