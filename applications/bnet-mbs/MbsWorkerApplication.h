#ifndef BNET_MbsWorkerApplication
#define BNET_MbsWorkerApplication

#include "bnet/WorkerApplication.h"

namespace bnet {

   class MbsWorkerApplication : public WorkerApplication {
      public:
         MbsWorkerApplication();

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

#endif
