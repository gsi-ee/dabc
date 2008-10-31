#ifndef BNET_CombinerModule
#define BNET_CombinerModule

#include "bnet/FormaterModule.h"

#include "dabc/Pointer.h"

#include "bnet/common.h"

#include <vector>

#include "dabc/Parameter.h"

#include "mbs/MbsTypeDefs.h"


namespace bnet {
    
   class WorkerPlugin;
    
   class MbsCombinerModule : public FormaterModule {
      protected:
      
         struct InputRec {
            int            bufindx; // index of buffer in input queue
            dabc::Buffer*  headbuf; // pointer on head buffer, indicate if we still using it
            dabc::Pointer  evptr;
            mbs::eMbs101EventHeader *evhdr;
            mbs::eMbs101EventHeader tmp_evhdr;
            
            InputRec() : bufindx(-1), headbuf(0), evptr(), evhdr(0), tmp_evhdr() {}
         };
         
         int                      fCfgEventsCombine;
         dabc::BufferSize_t       fTransportBufferSize;
         std::vector<InputRec>    fRecs;
         
         // range of events, packed in one output buffer
         mbs::MbsEventId          fMinEvId; 
         mbs::MbsEventId          fMaxEvId; 
         // list of subevents, which should be used to produce output buffer
         std::vector<dabc::Pointer> fSubEvnts; 
         
         dabc::HistogramParameter* fUsedEvents;
         
         dabc::RateParameter*      fEvntRate;
         
         dabc::Buffer* ProduceOutputBuffer();
         void SkipNotUsedBuffers();
         
      public:
         MbsCombinerModule(dabc::Manager* mgr, const char* name, 
                           WorkerPlugin* factory);
         
         virtual ~MbsCombinerModule();

         virtual void MainLoop();
   };   
}

#endif
