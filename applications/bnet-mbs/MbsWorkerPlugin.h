#ifndef BNET_MbsWorkerPlugin
#define BNET_MbsWorkerPlugin

#include "bnet/WorkerPlugin.h"

namespace bnet {
    
   class MbsWorkerPlugin : public WorkerPlugin {
      public:
         MbsWorkerPlugin(dabc::Manager* m) : WorkerPlugin(m) { }
         
         virtual bool CreateReadout(const char* portname, int portnumber);
         
         virtual dabc::Module* CreateCombiner(const char* name);
         virtual dabc::Module* CreateBuilder(const char* name);
         virtual dabc::Module* CreateFilter(const char* name);
         
         virtual bool CreateStorage(const char* portname);
         
         void SetMbsFilePars(const char* filebase);

         void SetMbsTransportPars();
         
         void SetMbsGeneratorsPars();

   }; 
}

extern "C" void InitUserPlugin(dabc::Manager* mgr);

#endif
