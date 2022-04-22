#ifndef DABC_DabcEvent
#define DABC_DabcEvent

#include "TObject.h"
#include "TClonesArray.h"
#include "mbs_root/DabcSubEvent.h"

namespace mbs_root {

	class DabcEvent : public TObject {
	public:
		Int_t fCount;
		Int_t fTrigger;
		Int_t fDummy;
		TClonesArray *fSubEvts;	//->

		Int_t fMaxSubs;
		Int_t fSubCursor;
		Int_t fUsedSubs;

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

		void Clear(Option_t *opt="");

	ClassDef(DabcEvent,1)
	};
}

#endif
