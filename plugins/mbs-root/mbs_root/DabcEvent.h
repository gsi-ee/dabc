#ifndef DABC_DabcEvent
#define DABC_DabcEvent

#include "TObject.h"
#include "TClonesArray.h"
#include "mbs_root/DabcSubEvent.h"

namespace mbs_root {

   class DabcEvent : public TObject {
   public:
      Int_t fCount{0};
      Int_t fTrigger{0};
      Int_t fDummy{0};
      TClonesArray *fSubEvts{nullptr};  //->

      Int_t fMaxSubs{0};
      Int_t fSubCursor{0};
      Int_t fUsedSubs{0};

   public:
      DabcEvent();

      DabcEvent(Int_t count, Int_t crate, Int_t subevts=20);

      virtual ~DabcEvent();

      void SetCount(Int_t num);
      Int_t GetCount();

      void SetTrigger(Int_t num);
      Int_t GetTrigger();

      void SetDummy(Int_t num);
      Int_t GetDummy();

      mbs_root::DabcSubEvent * AddSubEvent(Int_t crate, Int_t control, Int_t procid);

      void ResetSubCursor();

      mbs_root::DabcSubEvent * NextSubEvent();

      void Clear(Option_t *opt="") override;

      ClassDefOverride(DabcEvent,1)
   };
}

#endif
