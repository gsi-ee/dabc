#ifndef MBS_GeneratorModule
#define MBS_GeneratorModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

namespace mbs {

   class GeneratorModule : public dabc::ModuleAsync {

      protected:

         dabc::PoolHandle*       fPool;

         dabc::BufferSize_t      fBufferSize;

         uint32_t    fEventCount;
         uint32_t    fStartStopPeriod;
         uint16_t    fNumSubevents;
         uint16_t    fFirstProcId;
         uint32_t    fSubeventSize;
         bool        fIsGo4RandomFormat;

         void   FillRandomBuffer(dabc::Buffer* buf);

      public:
         GeneratorModule(const char* name, dabc::Command* cmd = 0);

         virtual void ProcessOutputEvent(dabc::Port* port);
   };

}


#endif
