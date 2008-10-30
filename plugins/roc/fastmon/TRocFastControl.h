#ifndef SCONTROL_H
#define SCONTROL_H

#include "TGo4Parameter.h"

class TRocFastControl : public TGo4Parameter {
   public:
     TRocFastControl();
     TRocFastControl(const char* name);
     virtual ~TRocFastControl();
     virtual Int_t    PrintParameter(Text_t * n, Int_t);
     virtual Bool_t   UpdateFrom(TGo4Parameter *);
     void     SaveMacro();

     // control filling of histograms
     Int_t    numROCs;
     Bool_t   fillADC;
   ClassDef(TRocFastControl,1)
};

#endif //SCONTROL_H
