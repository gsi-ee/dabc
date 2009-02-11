#ifndef DABC_CpuInfoModule
#define DABC_CpuInfoModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_statistic
#include "dabc/statistic.h"
#endif

namespace dabc {

   class CpuInfoModule : public dabc::ModuleAsync {
      protected:
         CpuStatistic    fStat;
         /** value of "Kind" configuration parameter, mask for
          *  0 - just show cpu info as double parameter
          *  1 - show CPUinfo as rate parameter
          *  2 - show infos for every cpu
          *  4 - show info for every cpu as ratemeters
          *  8 - show memory usage
          */
         int             fKind;

      public:
         CpuInfoModule(const char* name, dabc::Command* cmd = 0);

         virtual void ProcessTimerEvent(Timer* timer);
   };

}

#endif
