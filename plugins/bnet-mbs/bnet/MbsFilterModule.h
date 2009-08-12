#ifndef BNET_MbsFilterModule
#define BNET_MbsFilterModule

#include "bnet/FilterModule.h"

namespace bnet {

   class MbsFilterModule : public FilterModule {
      protected:
         long fTotalCnt;

         virtual bool TestBuffer(dabc::Buffer*);

      public:
         MbsFilterModule(const char* name, dabc::Command* cmd = 0);
   };
}

#endif
