#include <iostream>
#include <cstring>


#include "mbs_root/DabcSubEvent.h"

Int_t mbs_root::DabcSubEvent::GetSubCrate()
{
   return fSubCrate;
}

void mbs_root::DabcSubEvent::SetSubCrate(Int_t cr)
{
   fSubCrate = cr;
}

Int_t mbs_root::DabcSubEvent::GetProcid()
{
   return fProcid;
}

void mbs_root::DabcSubEvent::SetProcid(Int_t pr)
{
   fProcid = pr;
}

Int_t mbs_root::DabcSubEvent::GetControl()
{
   return fControl;
}

void mbs_root::DabcSubEvent::SetControl(Int_t cr)
{
   fControl = cr;
}

Int_t mbs_root::DabcSubEvent::GetUsedLength()
{
   return fLen;
}

Int_t mbs_root::DabcSubEvent::Data(Int_t cursor)
{
   if (cursor >= 0 && cursor < fLen)
      return fData[cursor];
   else
      return 0;
}

void mbs_root::DabcSubEvent::AddData(Int_t *src, Int_t len)
{
   size_t copylen = len * sizeof(Int_t);
   if (fLen + len > fAllocLen) {
      copylen = (fAllocLen - fLen) * sizeof(Int_t);
      std::cout << "Warning: data exceeding allocated length, truncating!1!" << std::endl;
   }
   // memcpy((void*)( fData + fLen), src, copylen);
   memcpy(fData + fLen, src, copylen);
   fLen += copylen / sizeof(Int_t);
}

void mbs_root::DabcSubEvent::ResetData()
{
   fLen = 0; // we only reset cursor, but do not delete allocated fData field
}

mbs_root::DabcSubEvent::DabcSubEvent() : TObject()
{
   // root always needs dummy default constructor for tree reconstruction!
   fLen = 0;
   fAllocLen = 0;
   fData = nullptr;
}

mbs_root::DabcSubEvent::DabcSubEvent(Int_t crate, Int_t control, Int_t procid, Bool_t init, Int_t capacity)
   : TObject(), fSubCrate(crate), fProcid(procid), fControl(control), fAllocLen(capacity)
{
   fLen = 0;
   if (init) {
      std::cout << "Initialized sub event " << crate << " with length " << fAllocLen << std::endl;
      fData = new Int_t[fAllocLen];
   }
}

mbs_root::DabcSubEvent::~DabcSubEvent()
{
   delete[] fData;
   fData = nullptr;
}

void mbs_root::DabcSubEvent::Clear(Option_t *)
{
   ResetData();
   fSubCrate = -1;
   fProcid = -1;
   fControl = -1;
}
