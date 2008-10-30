#ifndef BNET_MbsFilterModule
#define BNET_MbsFilterModule

#include "bnet/FilterModule.h"

namespace bnet {
   
   class WorkerPlugin;
   
   class MbsFilterModule : public FilterModule {
      protected:
         long fTotalCnt;
       
         virtual bool TestBuffer(dabc::Buffer*);

      public:
         MbsFilterModule(dabc::Manager* m, const char* name, WorkerPlugin* factory);
   };   
}

#endif
