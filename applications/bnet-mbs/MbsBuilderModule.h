#ifndef BNET_MbsBuilderModule
#define BNET_MbsBuilderModule

#include "bnet/BuilderModule.h"

#include "dabc/Pointer.h"

#include "mbs/MbsTypeDefs.h"

#include <vector>

namespace bnet {
    
   class WorkerPlugin;
   
   class MbsBuilderModule : public BuilderModule {
      protected:
         struct OutputRec {
            bool          ready; // indicates that output buffer ready to be send
            dabc::Buffer* buf; 
            dabc::Pointer evptr;
            mbs::sMbsBufferHeader* bufhdr;
            mbs::sMbsBufferHeader tmp_bufhdr;
            OutputRec() : ready(false), buf(0), bufhdr(0) {}
         }; 
      
         int                  fCfgEventsCombine; 
         OutputRec            fOut;
         dabc::RateParameter* fEvntRate;
         
         void FinishOutputBuffer();
         void StartOutputBuffer(dabc::BufferSize_t minsize = 0);
         void SendOutputBuffer();
         
      public:
         MbsBuilderModule(dabc::Manager* m, const char* name, WorkerPlugin* factory);
         virtual ~MbsBuilderModule();

         virtual void DoBuildEvent(std::vector<dabc::Buffer*>& bufs);

         virtual void BeforeModuleStart();
         virtual void AfterModuleStop();
   };   
}

#endif
