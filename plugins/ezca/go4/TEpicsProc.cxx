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

#include "TEpicsProc.h"

#include <cstdlib>
#include <sstream>

#include "TH1.h"
#include "TH2.h"
#include "TCutG.h"
#include "TCanvas.h"
#include "TLine.h"

#include "TGo4MbsEvent.h"
#include "TGo4WinCond.h"
#include "TGo4PolyCond.h"
#include "TGo4CondArray.h"
#include "TGo4Picture.h"
#include "TGo4StepFactory.h"
#include "TGo4Analysis.h"
#include "TGo4Version.h"


#define CBM_EPIX_SUBID 0xa

#define CBM_EPIX_NUMHIS_LONG    200
#define CBM_EPIX_NUMHIS_DOUBLE  200
#define CBM_EPIX_TRENDSIZE      120
#define CBM_EPIX_STATRANGE      5000

#define CBM_EPIX_useDOUBLES true

extern "C"
{
    INTS4 f_ut_utime(INTS4 l_sec, INTS4 l_msec, CHARS *pc_time);
}

#include "f_ut_utime.h"



//***********************************************************
TEpicsProc::TEpicsProc() : TGo4EventProcessor()
{
}

//***********************************************************
TEpicsProc::~TEpicsProc()
{
   TGo4Log::Info("TEpicsProc: Delete instance");
}

//***********************************************************
// this one is used in standard factory
TEpicsProc::TEpicsProc(const char* name) : TGo4EventProcessor(name)
{
   TGo4Log::Info("TEpicsProc: Create instance %s", name);

   fVerbose = false;
}

const char* TEpicsProc::GetUpdateTimeString()
{
   if(fUTimeSeconds==0) return "EPICS time not available";

   UInt_t timeseconds=fUTimeSeconds;
   UInt_t timemicros=0;
   f_ut_utime(timeseconds, timemicros, fcTimeString);
   return fcTimeString;
}


//-----------------------------------------------------------
// event function
Bool_t TEpicsProc::BuildEvent(TGo4EventElement*)
{  // called by framework. We dont fill any output event here at all

   if ((GetInputEvent()==0) || (GetInputEvent()->IsA() != TGo4MbsEvent::Class())) {
      TGo4Log::Error("TEpicsProc: no input MBS event found!");
      return kFALSE;
   }

   TGo4MbsEvent* evnt = (TGo4MbsEvent*) GetInputEvent();

   if(evnt->GetTrigger() > 11) {
      TGo4Log::Info("TEpicsProc: Skip trigger event");
      return kFALSE;
   }

   evnt->ResetIterator();
   TGo4MbsSubEvent *psubevt = nullptr;
   while((psubevt = evnt->NextSubEvent()) != nullptr) { // loop over subevents

      if (psubevt->GetProcid() != CBM_EPIX_SUBID) return kFALSE;

      Int_t* pdata = psubevt->GetDataField();
      Int_t* theEnd = psubevt->GetDataField() + psubevt->GetIntLen();
      //first payload: event id number
      fEventId = *pdata++;
      if (fVerbose)
         printf("Found EPICS event id %u\n", fEventId);

      // secondly get dabc record time expression
      fUTimeSeconds = (UInt_t) *pdata++;
      if (fVerbose)
         printf("Found EPICS time: %s\n", GetUpdateTimeString());

      //int numlong = *pdata++;

      Long64_t* plong = (Long64_t*) pdata;

      int numlong= *plong++;
      if (fVerbose)
         printf("Num long: %d\n", numlong);

      std::vector<Long64_t> fLongRecords;

      //plong= (Long64_t*) pdata;
      for (int i = 0; i < numlong; ++i) {
         Long64_t val = *plong++;
         if (fVerbose)
            printf("   Long[%d] = %lld\n", i, val);
         fLongRecords.emplace_back(val);
      }

      // get header with number of double records
      //int numdub = *pdata++;
      int numdub = *plong++;
      if (fVerbose)
         printf("Num double: %d\n", numdub);

      std::vector<double> fDoubleRecords;

      //plong= (Long64_t*) pdata;
      Double_t* pdouble= (Double_t*) (plong);
      for (int i = 0; i < numdub; ++i) {
   #ifdef CBM_EPIX_useDOUBLES
         Double_t val = *pdouble++;
         Long64_t valint = val;
         plong++;
         // cout << "double("<<i<<")=" << val << endl;
   #else
         Long64_t valint = *plong++;
         Double_t val = (Double_t) (valint) / CBM_EPIX_DOUBLESCALE; // workaround to avoid number format tricks
   #endif

         if (fVerbose)
            printf("   Double[%d] = int:%lld float:%f\n", i, valint, val);

         fDoubleRecords.emplace_back(val);
      }

      pdata = (Int_t*) plong;
      // process optional additional variable names and update names map
      if(pdata<theEnd) {
         unsigned rest = (char*)theEnd - (char*)pdata;
         if (fVerbose)
            printf("Found description at mbs event end, pdata:0x%p end: 0x%p, rest=0x%x\n",
                  pdata, theEnd, rest);

         std::istringstream input(std::string((const char*) pdata, rest), std::istringstream::in);
         char curbuf[256];
         unsigned numlong = 0, numdub = 0;
         // first get number of long records:
         input.getline(curbuf,256,'\0'); // use string separators '\0' here
         sscanf(curbuf, "%u", &numlong);
         for (unsigned i = 0; i < numlong; i++) {
            input.getline(curbuf,256,'\0');

            VariableHist* hist = FindVariable(curbuf);
            if (!hist) hist = CreateHist(curbuf);

            if (fVerbose)
               printf("Var:%s value %ld\n", curbuf, (long) fLongRecords[i]);
            UpdateHist(hist, fLongRecords[i], curbuf);
         }
         // get number of double records:

         input.getline(curbuf,256,'\0');
         sscanf(curbuf,"%u", &numdub);
         for (unsigned i = 0; i < numdub; i++) {
            input.getline(curbuf,256,'\0');

            VariableHist* hist = FindVariable(curbuf);
            if (!hist) hist = CreateHist(curbuf);

            if (fVerbose)
               printf("Var:%s value %f\n", curbuf, fDoubleRecords[i]);

            UpdateHist(hist, fDoubleRecords[i], curbuf);
         }
      }


   }// while

   return kTRUE;
}


TEpicsProc::VariableHist* TEpicsProc::FindVariable(const char* name)
{
   if (!name || (strlen(name)==0)) return nullptr;
   for (unsigned n=0;n<all_hists.size();n++)
      if (all_hists[n].IsName(name)) return & all_hists[n];

   return nullptr;
}

TEpicsProc::VariableHist* TEpicsProc::CreateHist(const char* varname)
{
   if (!varname || (strlen(varname)==0)) return nullptr;

   const char* dirname = "EPICS";
   Double_t range = CBM_EPIX_STATRANGE;
   Int_t bins = 2*CBM_EPIX_STATRANGE;

   TString obname;
   TString obtitle;

   VariableHist dummy;
   all_hists.emplace_back(dummy);

   obname.Form("%s/Statistics/Stat_%s", dirname, varname);
   obtitle.Form("EPICS variable %s statistics", varname);
   all_hists.back().fStat = MakeTH1('I', obname.Data(), obtitle.Data(), bins, -range, range);
   obname.Form("%s/Trending/Trend_%s",dirname, varname);
   obtitle.Form("EPICS variable %s trending", varname);
   all_hists.back().fTrend = MakeTH1('I', obname.Data(), obtitle.Data(), CBM_EPIX_TRENDSIZE, 0, CBM_EPIX_TRENDSIZE);

   all_hists.back().fGraph = MakeTimeGraph(varname, dirname);

   return & all_hists.back();
}

TGraph* TEpicsProc::MakeTimeGraph(const TString& name, const TString& dir)
{
   TString fullname=dir+"/"+name;
   //cout <<"fullname "<<fullname << endl;
   TGraph* gr=dynamic_cast<TGraph*>(GetObject(fullname));
   if(!gr) {
      gr = new TGraph();
      gr->SetName(name);
      gr->SetMarkerStyle(33);
      TCanvas* c1 = new TCanvas("c1","c1",3);
      gr->Draw("A");// dummy create axis:
      delete c1;
      gr->GetXaxis()->SetTimeDisplay(1);
      gr->GetXaxis()->SetNdivisions(-503);
      gr->GetXaxis()->SetTimeFormat("%H:%M:%S");
      gr->GetXaxis()->SetTimeOffset(0,"gmt");
      AddObject(gr,dir.Data());
   }
   gr->GetXaxis()->SetTimeOffset(0,"gmt");
   return gr;
}

void TEpicsProc::UpdateTimeGraph(TGraph* gr, Double_t value, time_t time)
{
   TDatime offs(1995,1,1,0,0,0); // substract begin of root time  from unix epoch
   TDatime dt(time, kFALSE);
   Int_t npoints = gr->GetN();
   gr->SetPoint(npoints,(dt.Convert() - offs.Convert()),value);
}

void TEpicsProc::UpdateTrending(TH1* histo, Double_t val, time_t time)
{
   // first test: just use simple binwise trending
   IncTrending(histo, val, kTRUE);
}


void  TEpicsProc::IncTrending( TH1 * histo, Double_t value, bool forwards )
{
   if(!histo) return;
   int bins = histo->GetNbinsX();
   //bool forwards=true;
   int j = forwards ? bins : 0;
   int dj = forwards ? -1 : +1;
   for(int i=0;i<bins;i++) {
      j = forwards ? bins-i : i;
      Double_t oldval = histo->GetBinContent(j+dj);
      histo->SetBinContent(j,oldval);
   }
   histo->SetBinContent(j+dj,value);
}

void TEpicsProc::UpdateHist(VariableHist* hst, double val, const char* varname)
{
   if (!varname || !hst) return;

   hst->fStat->Fill(val);
   hst->fStat->SetTitle(Form("Statistics of %s at %s", varname, GetUpdateTimeString()));
   UpdateTrending(hst->fTrend, val, fUTimeSeconds);
   hst->fTrend->SetTitle(Form("Trending of %s at %s", varname, GetUpdateTimeString()));
   UpdateTimeGraph(hst->fGraph, val, fUTimeSeconds);
   hst->fGraph->SetTitle(Form("Timegraph of %s", varname));
}

