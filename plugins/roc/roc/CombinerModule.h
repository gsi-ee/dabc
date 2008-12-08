#ifndef ROC_CombinerModule
#define ROC_CombinerModule

#include "dabc/ModuleAsync.h"
#include "dabc/Pointer.h"

#include <vector>

namespace roc {

   class CombinerModule : public dabc::ModuleAsync {

      public:

         CombinerModule(const char* name, dabc::Command* cmd);
         virtual ~CombinerModule();

         virtual void ProcessInputEvent(dabc::Port* port);

         virtual void ProcessOutputEvent(dabc::Port* port);

      protected:
         struct InputRec {
            unsigned   rocid;

            uint32_t   curr_epoch;
            bool       iscurrepoch;
            unsigned   curr_nbuf;
            unsigned   curr_indx;

            uint32_t   prev_epoch;  // previous epoch marker
            bool       isprev;
            unsigned   prev_nbuf;  // id of buffer where epoch is started
            unsigned   prev_indx;  // indx inside buffer where epoch found
            uint32_t   prev_evnt;  // sync event number
            uint32_t   prev_stamp; // sync time stamp

            uint32_t   next_epoch;  // number of the next epoch
            bool       isnext;      //
            unsigned   next_nbuf;   // id of buffer where epoch is started
            unsigned   next_indx;  // indx inside buffer where epoch found
            uint32_t   next_evnt;  // sync event number
            uint32_t   next_stamp; // sync time stamp

            bool       isready;    // indicate if epoch and next are defined and ready for combining

            bool          use;     // use input for combining
            bool          data_err; // event numbers are wrong
            dabc::Pointer ptr;     // used in combining
            unsigned      nbuf;    // used in combining
            uint32_t      epoch;   // original epoch used in combining
            unsigned      stamp;   // corrected stamp value in combining
            unsigned      stamp_shift; // (shift to event begin)

            InputRec() :
               rocid(0),
               curr_epoch(0), iscurrepoch(false), curr_nbuf(0), curr_indx(0),

               prev_epoch(0), isprev(false), prev_nbuf(0), prev_indx(0), prev_evnt(0), prev_stamp(0),
               next_epoch(0), isnext(false), next_nbuf(0), next_indx(0), next_evnt(0), next_stamp(0),
               isready(false), use(false), data_err(false), ptr(), nbuf(0), epoch(0), stamp(0), stamp_shift(0) {}
         };

         bool FindNextEvent(unsigned ninp);
         bool SkipEvent(unsigned ninp);

         void TryToProduceEventBuffers();

         unsigned FillRawSubeventsBuffer(dabc::Pointer& outptr);

         dabc::BufferSize_t CalcDistance(unsigned ninp, unsigned nbuf1, unsigned indx1, unsigned nbuf2, unsigned indx2);

         void DumpData(dabc::Pointer ptr);

         dabc::PoolHandle*    fPool;
         dabc::BufferSize_t   fBufferSize;

         bool                 fSimpleMode;

         std::vector<InputRec> fInp;

         dabc::Buffer*        fOutBuf;
         dabc::Pointer        f_outptr;

         int                  fiEventCnt;

         bool                 fErrorOutput;

   };
}

#endif
