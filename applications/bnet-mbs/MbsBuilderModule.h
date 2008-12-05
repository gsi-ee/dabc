#ifndef BNET_MbsBuilderModule
#define BNET_MbsBuilderModule

#include "bnet/BuilderModule.h"

#include "dabc/Pointer.h"

#include "mbs/MbsTypeDefs.h"

#include "mbs/Iterator.h"

#include <vector>

namespace bnet {

   class MbsBuilderModule : public BuilderModule {
      protected:
         struct OutputRec {
            bool                ready; // indicates that output buffer ready to be send
            dabc::Buffer*       buf;
            mbs::WriteIterator  iter;

            OutputRec() : ready(false), buf(0), iter(0) {}
         };

         int                  fCfgEventsCombine;
         OutputRec            fOut;
         dabc::RateParameter* fEvntRate;

         void FinishOutputBuffer();
         void StartOutputBuffer(dabc::BufferSize_t minsize = 0);
         void SendOutputBuffer();

      public:
         MbsBuilderModule(const char* name, dabc::Command* cmd = 0);
         virtual ~MbsBuilderModule();

         virtual void DoBuildEvent(std::vector<dabc::Buffer*>& bufs);

         virtual void BeforeModuleStart();
         virtual void AfterModuleStop();
   };
}

#endif
