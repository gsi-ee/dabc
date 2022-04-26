
#include "mbs_root/DabcEvent.h"

mbs_root::DabcEvent::DabcEvent():TObject(), fCount(0), fTrigger(0),fSubCursor(0), fUsedSubs(0)
    {
        // root always needs dummy default constructor for tree reconstruction!
        fSubEvts=0;
    }

mbs_root::DabcEvent::DabcEvent(Int_t count, Int_t crate, Int_t subevts):
    TObject(),
    fCount(count),fTrigger(crate),fMaxSubs(0),fSubCursor(0),fUsedSubs(0)
    {
        // this is the user ctor. Event object will be created
        // once before data filling loop,
        // then for new data event contents are just reset by Clear()
        fSubEvts = new TClonesArray("mbs_root::DabcSubEvent", subevts);
    }

mbs_root::DabcEvent::~DabcEvent()
{
   delete fSubEvts;
}

void mbs_root::DabcEvent::SetCount(Int_t num)
        {
        fCount=num;
        }

Int_t mbs_root::DabcEvent::GetCount()
        {
        return fCount;
        }

void mbs_root::DabcEvent::SetTrigger(Int_t num)
        {
            fTrigger=num;
        }

Int_t mbs_root::DabcEvent::GetTrigger()
        {
            return fTrigger;
        }

void mbs_root::DabcEvent::SetDummy(Int_t num)
        {
            fDummy=num;
        }

Int_t mbs_root::DabcEvent::GetDummy()
        {
            return fDummy;
        }

mbs_root::DabcSubEvent* mbs_root::DabcEvent::AddSubEvent(Int_t crate, Int_t control, Int_t procid)
	{
        Bool_t init=false;

        if(++fSubCursor >fMaxSubs)
            {
                // only initialize data fields if subevent of that index
                // was never used before
                init=true;
                fMaxSubs=fSubCursor;
            }
        TClonesArray &subarray = *fSubEvts;
        DabcSubEvent *sube = new(subarray[fSubCursor-1]) DabcSubEvent(crate, control, procid, init);
        fUsedSubs++;
        return sube;
    	}

void mbs_root::DabcEvent::ResetSubCursor()
{
   fSubCursor = 0;
}

mbs_root::DabcSubEvent* mbs_root::DabcEvent::NextSubEvent()
{
   if (++fSubCursor > fUsedSubs) {
      fSubCursor--;
      return nullptr; // exceed used length in array
   }
   return dynamic_cast<DabcSubEvent*>(fSubEvts->At(fSubCursor - 1));
}

void mbs_root::DabcEvent::Clear(Option_t *)
{
   fSubEvts->Clear("C"); //will also call Track::Clear
   fCount = -1;
   fTrigger = -1;
   fSubCursor = 0;
   fUsedSubs = 0;
}
