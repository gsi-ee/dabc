#ifndef TANALYSIS_H
#define TANALYSIS_H

#include "TGo4Analysis.h"

class TFile;
class TGo4MbsEvent;
class TRocFastParam;
class TRocFastControl;

class TRocFastAnalysis : public TGo4Analysis {
   public:
      TRocFastAnalysis();
      TRocFastAnalysis(const char* input, Int_t type, Int_t port, const char* out, Bool_t enable);
      virtual ~TRocFastAnalysis() ;
      virtual Int_t UserPreLoop();
      virtual Int_t UserEventFunc();
      virtual Int_t UserPostLoop();
   private:
      TFile             *fUserFile;
      TGo4MbsEvent      *fMbsEvent;
      TRocFastParam     *fPar;
      TRocFastControl   *fCtl;
      Int_t             fEvents;
      Int_t             fLastEvent;

   ClassDef(TRocFastAnalysis,1)
};
#endif //TANALYSIS_H
