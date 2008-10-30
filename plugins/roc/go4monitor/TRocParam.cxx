#include "TRocParam.h"

#include "Riostream.h"

//***********************************************************
TRocParam::TRocParam(const char* name) : TGo4Parameter(name)
{
   numRocs = 3;
   fillADC = true;
}

//***********************************************************
TRocParam::~TRocParam()
{
}
//***********************************************************

//-----------------------------------------------------------
Int_t TRocParam::PrintParameter(Text_t * n, Int_t)
{
  cout << "Parameter " << GetName()<<":" <<endl;
  cout << " numRocs = "<< numRocs<<endl;
  cout << " fillADC = "<< fillADC<<endl;
  return 0;
}

//-----------------------------------------------------------
Bool_t TRocParam::UpdateFrom(TGo4Parameter *pp)
{
  if(pp->InheritsFrom("TRocParam")) {
     TRocParam * from = (TRocParam *) pp;
     cout << "**** TRocParam " << GetName() << " updated from auto save file" << endl;
     numRocs=from->numRocs;
     fillADC=from->fillADC;
   } else
     cout << "Wrong parameter object: " << pp->ClassName() << endl;
  return kTRUE;
}
