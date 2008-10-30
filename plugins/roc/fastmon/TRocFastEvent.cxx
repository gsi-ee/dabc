#include "TRocFastEvent.h"

#include "Riostream.h"

//***********************************************************
TRocFastEvent::TRocFastEvent() :
   TGo4EventElement()
{
    cout << "**** TRocFastEvent: Create instance" << endl;
}
//***********************************************************
TRocFastEvent::TRocFastEvent(const char* name) :
   TGo4EventElement(name)
{
  cout << "**** TRocFastEvent: Create instance " << name << endl;
}
//***********************************************************
TRocFastEvent::~TRocFastEvent()
{
  cout << "**** TRocFastEvent: Delete instance " << endl;
}

//-----------------------------------------------------------
void  TRocFastEvent::Clear(Option_t *t)
{
  // all members should be cleared.
//   memset((void*) &fCrate1[0],0, sizeof(fCrate1));
//   memset((void*) &fCrate2[0],0, sizeof(fCrate2));
}
