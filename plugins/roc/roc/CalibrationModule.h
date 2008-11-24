#ifndef ROC_CalibrationModule
#define ROC_CalibrationModule

#include "dabc/ModuleAsync.h"
#include "dabc/Pointer.h"

#include <vector>

class SysCoreSorter;
class SysCoreData;

namespace roc {

   class CalibrationModule : public dabc::ModuleAsync {

      public:

         CalibrationModule(const char* name, dabc::Command* cmd);
         virtual ~CalibrationModule();

         virtual void BeforeModuleStart() {}
         virtual void AfterModuleStop() {}

         virtual void ProcessInputEvent(dabc::Port* port);

         virtual void ProcessOutputEvent(dabc::Port* port);


      protected:
         struct CalibRec {
            unsigned       rocid;      // rocid
            double         calibr_k;   // calibration coeff K
            uint32_t       calibr_b_e; // calibration coeff B, integer part (in epochs size)
            double         calibr_b_f; // calibration coeff B, float path (in epoch fraction)
            unsigned       weight;     // weight of coefficients
            SysCoreSorter* sorter;

            uint64_t       evnt1_tm;    // time of event in the beginning
            uint64_t       evnt2_tm;    // time of event in the end
            uint32_t       evnt1_num;   // number of event in the beginning
            uint32_t       evnt2_num;   // number of event in the end
            uint64_t       evnt_len;    // time length of the event

            SysCoreData*   data;        // data (original or sorted, depeding from stage)
            unsigned       numdata;     // number of data sill to process

            uint32_t       front_epoch; // value of front epoch, relative to which delta t is calculating
            double         front_shift; // shift to calibrated time to adjust to output front epoch
            uint32_t       last_epoch;  // last epoch, taken from stream
            bool           is_last_epoch; // do we get at all any epoch
            uint32_t       last_stamp;    // last stamp, detected on the system

            uint32_t       stamp;       // current timestamp in stream (calibrated, relative to FrontOutEpoch)
            bool           stamp_copy;  // indicate that message should be copied to output

            CalibRec(unsigned id = 0);
            inline bool NeedBCoef() const { return !is_last_epoch || (weight==0); }
            void RecalculateCalibr(double k, uint32_t b_e = 0, double b_f =0.);
            double CalibrEpoch(uint32_t& epoch);
            void TimeStampOverflow();
         };

         bool DoCalibration();

         bool FlushOutputBuffer();

         dabc::PoolHandle*    fPool;
         unsigned             fNumRocs;
         dabc::BufferSize_t   fBufferSize;

         dabc::Pointer        f_inpptr; // pointer on current event in input buffer

         dabc::Buffer*        fOutBuf;
         dabc::Pointer        f_outptr; // pointer on free space in output buffer

         long                 fBuildEvents;
         long                 fSkippedEvents;
         long                 fBuildSize;

         std::vector<CalibRec> fCalibr;

         uint32_t             fFrontOutEpoch; // epoch, relative to which all time stamps are calculating
         uint32_t             fLastOutEpoch;  // last epoch, delivered to output
         bool                 fIsOutEpoch;    // indicates if there was epoch delivered to output
   };
}

#endif
