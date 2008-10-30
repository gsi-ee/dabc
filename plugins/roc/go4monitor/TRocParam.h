#ifndef SPAR_H
#define SPAR_H

#include "TGo4Parameter.h"

class TRocParam : public TGo4Parameter {
   public:
      TRocParam(const char* name = 0);
      virtual ~TRocParam();
      virtual Int_t  PrintParameter(Text_t * n, Int_t);
      virtual Bool_t UpdateFrom(TGo4Parameter *);
      
      Int_t numRocs;
      Bool_t fillADC;

   ClassDef(TRocParam,1)
};

#endif //SPAR_H
