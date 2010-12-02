#ifndef DABC_DabcSubEvent
#define DABC_DabcSubEvent

#include "TObject.h"

namespace mbs_root {

	class DabcSubEvent : public TObject {
	private:
		Int_t fSubCrate;
  		Int_t fProcid;
  		Int_t fControl;   
  		Int_t    fAllocLen;    // maximum allocated length   
  		Int_t    fLen;     // used length- root will only store this len to tree
  		Int_t   *fData;      //[fLen]
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

		void Clear(Option_t* opt="");

	ClassDef(DabcSubEvent,1)
	};
}

#endif