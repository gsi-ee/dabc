#ifndef BNET_CombinerModule
#define BNET_CombinerModule

#include "dabc/ModuleAsync.h"

#include "bnet/common.h"

#include <vector>

namespace bnet {
    
   class WorkerApplication;
    
   class TestCombinerModule : public dabc::ModuleAsync {
      protected:
         int                      fNumReadout; 
         int                      fModus;
         dabc::PoolHandle*      fInpPool;
         dabc::PoolHandle*      fOutPool;
         uint64_t                 fOutBufferSize;
         dabc::Port*              fOutPort;
         std::vector<dabc::Buffer*> fBuffers;
         int                      fLastInput; // indicate id of the port where next packet must be read
         dabc::Buffer*            fOutBuffer;
         uint64_t                 fLastEvent;
         
         
         dabc::Buffer* MakeSegmenetedBuf(uint64_t& evid);
         dabc::Buffer* MakeMemCopyBuf(uint64_t& evid);   
         
      public:
         TestCombinerModule(dabc::Manager* mgr, const char* name, 
                        WorkerApplication* factory);
         
         virtual ~TestCombinerModule();
         
         virtual void BeforeModuleStart();
         virtual void AfterModuleStop();
         
         virtual void ProcessUserEvent(dabc::ModuleItem* item, uint16_t evid);
   };   
}

#endif
