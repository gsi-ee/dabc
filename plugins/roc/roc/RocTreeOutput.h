#ifndef ROC_RocTreeOutput
#define ROC_RocTreeOutput

#include "dabc/DataIO.h"

#ifdef DABC_ROOT

#include "TTree.h"
#include "TFile.h"

#else

class TTree;
class TFile;
typedef unsigned UChar_t;
typedef unsigned UShort_t;
typedef unsigned UInt_t;
typedef unsigned ULong64_t;

#endif

class SysCoreData;

namespace roc {
   class RocTreeOutput : public dabc::DataOutput {
      public:
         RocTreeOutput(const char* name, int sizelimit_mb = 0);
         
         virtual bool WriteBuffer(dabc::Buffer* buf);

         virtual ~RocTreeOutput();
         
         bool IsOk() const { return fTree!=0; }
         
      protected: 
         void WriteNextData(SysCoreData* data);
      
         TTree*   fTree;
         TFile*   fFile;
         
         UChar_t     f_rocid;
         UChar_t     f_nxyter;
         UChar_t     f_id;
         ULong64_t   f_timestamp;
         UInt_t      f_value;
         
         UInt_t     fLastEpoch[8]; // maximum 8 rocs id are supported
         bool       fValidEpoch[8];
   };   
}


#endif
