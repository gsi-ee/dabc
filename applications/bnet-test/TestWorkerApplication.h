#ifndef BNET_TestWorkerApplication
#define BNET_TestWorkerApplication

#include "bnet/WorkerApplication.h"

namespace bnet {

   class TestWorkerApplication : public WorkerApplication {
      public:
         TestWorkerApplication();

         virtual bool CreateReadout(const char* portname, int portnumber);

         virtual bool CreateCombiner(const char* name);
         virtual bool CreateBuilder(const char* name);
         virtual bool CreateFilter(const char* name);

         bool CreateStorage(const char* portname);

      protected:
         virtual void DiscoverNodeConfig(dabc::Command* cmd);

      private:
         bool fABBActive;

   };
}

#endif
