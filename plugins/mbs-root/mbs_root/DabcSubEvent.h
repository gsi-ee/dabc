#ifndef DABC_DabcSubEvent
#define DABC_DabcSubEvent

#include "TObject.h"

namespace mbs_root {

   class DabcSubEvent : public TObject {
   private:
      Int_t fSubCrate;
        Int_t  fProcid{0};
        Int_t  fControl{0};
        Int_t  fAllocLen{0};    // maximum allocated length
        Int_t  fLen{0};     // used length- root will only store this len to tree
        Int_t *fData{nullptr};      //[fLen]
   public:
      Int_t GetSubCrate();
      void SetSubCrate(Int_t cr);

      Int_t GetProcid();
      void SetProcid(Int_t pr);

      Int_t GetControl();
      void SetControl(Int_t cr);

      Int_t GetUsedLength();

      Int_t Data(Int_t cursor);

      void AddData(Int_t* src, Int_t len);

      void ResetData();

      DabcSubEvent();

      DabcSubEvent(Int_t crate, Int_t control, Int_t procid, Bool_t init=kFALSE, Int_t capacity=4096);

      virtual ~DabcSubEvent();

      void Clear(Option_t* opt="") override;

      ClassDefOverride(DabcSubEvent,1)
   };
}

#endif
