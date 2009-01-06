#ifndef MBS_CombinerModule
#define MBS_CombinerModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef MBS_Iterator
#include "mbs/Iterator.h"
#endif

#include <vector>


namespace mbs {

   class CombinerModule : public dabc::ModuleAsync {
      protected:
         dabc::PoolHandle*          fPool;
         unsigned                   fBufferSize;
         std::vector<ReadIterator>  fInp;
         WriteIterator              fOut;
         dabc::Buffer*              fOutBuf;
         int                        fTmCnt;

         bool                       fFileOutput;
         bool                       fServOutput;

         bool BuildEvent();
         bool FlushBuffer();

      public:
         CombinerModule(const char* name, dabc::Command* cmd = 0);
         virtual ~CombinerModule();

         virtual void ProcessInputEvent(dabc::Port* port);
         virtual void ProcessOutputEvent(dabc::Port* port);

         virtual void ProcessTimerEvent(dabc::Timer* timer);

         bool IsFileOutput() const { return fFileOutput; }
         bool IsServOutput() const { return fServOutput; }
   };

}


#endif
