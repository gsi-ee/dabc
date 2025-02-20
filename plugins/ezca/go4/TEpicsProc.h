// $Id$
//-----------------------------------------------------------------------
//       The GSI Online Offline Object Oriented (Go4) Project
//         Experiment Data Processing at EE department, GSI
//-----------------------------------------------------------------------
// Copyright (C) 2000- GSI Helmholtzzentrum f�r Schwerionenforschung GmbH
//                     Planckstr. 1, 64291 Darmstadt, Germany
// Contact:            http://go4.gsi.de
//-----------------------------------------------------------------------
// This software can be used under the license agreements as stated
// in Go4License.txt file which is part of the distribution.
//-----------------------------------------------------------------------

#ifndef TUNPACKPROCESSOR_H
#define TUNPACKPROCESSOR_H

#include "TGo4EventProcessor.h"

#include "TH1.h"
#include "TH2.h"
#include "TGraph.h"


class TEpicsProc : public TGo4EventProcessor {
   protected:

      struct VariableHist {
         TH1*    fTrend{nullptr};
         TH1*    fStat{nullptr};
         TGraph* fGraph{nullptr};
         VariableHist() {}
         bool Empty() const { return !fGraph; }
         bool IsName(const std::string &name) const { return fGraph ? name==fGraph->GetName() : false; }
      };

      std::vector<VariableHist> all_hists;           //! list of all histograms

      bool fVerbose;
      Int_t fEventId;
      UInt_t fUTimeSeconds;
      char fcTimeString[200];

      const char *GetUpdateTimeString();

      VariableHist* FindVariable(const char *name);

      VariableHist* CreateHist(const char *varname);

      TGraph* MakeTimeGraph(const TString& name, const TString& dir);

      void UpdateHist(VariableHist* hst, double val, const char *varname);

      void UpdateTrending(TH1* histo, Double_t val, time_t time);

      void  IncTrending( TH1 * histo, Double_t value, bool forwards );

      void UpdateTimeGraph(TGraph* gr, Double_t value, time_t time);


   public:
      TEpicsProc() ;
      TEpicsProc(const char *name);
      virtual ~TEpicsProc() ;

      Bool_t BuildEvent(TGo4EventElement*); // event processing function


   ClassDef(TEpicsProc,1)
};

#endif //TUNPACKPROCESSOR_H
