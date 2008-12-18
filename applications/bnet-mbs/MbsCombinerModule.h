#ifndef BNET_MbsCombinerModule
#define BNET_MbsCombinerModule

#include "bnet/CombinerModule.h"

#include "dabc/Pointer.h"

#include "bnet/common.h"

#include <vector>

#include "dabc/Parameter.h"

#include "mbs/MbsTypeDefs.h"

#include "mbs/Iterator.h"

namespace bnet {

   class WorkerApplication;

   class MbsCombinerModule : public CombinerModule {
      protected:

         struct InputRec {
            int                bufindx; // index of buffer in input queue
            dabc::Buffer*      headbuf; // pointer on head buffer, indicate if we still using it
            mbs::ReadIterator  iter;

            InputRec() : bufindx(-1), headbuf(0), iter(0) {}
         };

         int                      fCfgEventsCombine;
         dabc::BufferSize_t       fTransportBufferSize;
         std::vector<InputRec>    fRecs;

         // range of events, packed in one output buffer
         mbs::EventNumType        fMinEvId;
         mbs::EventNumType        fMaxEvId;
         // list of subevents, which should be used to produce output buffer
         std::vector<dabc::Pointer> fSubEvnts;

         dabc::HistogramParameter* fUsedEvents;

         dabc::RateParameter*      fEvntRate;

         dabc::Buffer* ProduceOutputBuffer();
         void SkipNotUsedBuffers();

      public:
         MbsCombinerModule(const char* name, dabc::Command* cmd = 0);

         virtual ~MbsCombinerModule();

         virtual void MainLoop();
   };
}

#endif
