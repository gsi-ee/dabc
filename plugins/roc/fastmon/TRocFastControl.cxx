#include "TRocFastControl.h"

#include <stdio.h>
#include "Riostream.h"

//***********************************************************
TRocFastControl::TRocFastControl() :
   TGo4Parameter("Control"),
   numROCs(3),
   fillADC(kTRUE)
{
}
//***********************************************************
TRocFastControl::TRocFastControl(const char* name) :
   TGo4Parameter(name),
   numROCs(3),
   fillADC(kTRUE)
{
}
//***********************************************************
TRocFastControl::~TRocFastControl() {}
//***********************************************************

//-----------------------------------------------------------
Int_t TRocFastControl::PrintParameter(Text_t * n, Int_t){
  cout << "**** TRocFastControl: ";
  cout << "ADC histograms filling " << (fillADC ? "enabled" : "disabled") << endl;
  return 0;

}
//-----------------------------------------------------------
Bool_t TRocFastControl::UpdateFrom(TGo4Parameter *pp)
{
  cout << "**** TRocFastControl " << GetName() << " updated from auto save file" << endl;
  if(pp->InheritsFrom(TRocFastControl::Class()))
    {
      TRocFastControl * from = (TRocFastControl *) pp;
      numROCs = from->numROCs;
      fillADC = from->fillADC;
    }
  else
    cout << "Wrong parameter object: " << pp->ClassName() << endl;

  SaveMacro();
  PrintParameter("",0);
  return kTRUE;
}
//-----------------------------------------------------------
void TRocFastControl::SaveMacro()
{
  FILE *pf;
  pf=fopen("histofill.C","w+");
  if(pf)
    {
      fputs("// written by setfill.C macro\n",pf);
      fputs("// executed in TRocFastProc\n",pf);
      fputs("{\n",pf);
      fputs("TRocFastControl * fCtl;\n",pf);
      fputs("fCtl = (TRocFastControl *)(TGo4Analysis::Instance()->GetParameter(\"Control\"));\n",pf);
      if(fillADC) fputs("fCtl->fillADC=kTRUE;\n",pf);
      else        fputs("fCtl->fillADC=kFALSE;\n",pf);
      fputs("}\n",pf);
      fclose(pf);
      cout << "**** TRocFastControl: new histofill.C written" << endl;
    }
  else cout << "**** TRocFastControl: Error writing histofill.C!" << endl;

  PrintParameter("",0);
}
