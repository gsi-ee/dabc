#ifndef BNET_MbsWorkerApplication
#define BNET_MbsWorkerApplication

#include "bnet/WorkerApplication.h"

namespace bnet {

   class MbsWorkerApplication : public WorkerApplication {
      public:
         MbsWorkerApplication(const char* classname = 0);

         virtual int ExecuteCommand(dabc::Command* cmd);

         virtual bool CreateReadout(const char* portname, int portnumber);

         virtual bool CreateCombiner(const char* name);
         virtual bool CreateBuilder(const char* name);
         virtual bool CreateFilter(const char* name);

         virtual bool CreateStorage(const char* portname);

         virtual bool CreateOutFile(const char* portname, const std::string& filename);

         virtual bool BeforeAppModulesDestroyed();

         bool CreateOutServer(const std::string& serverkind);
   };
}

#endif
