// $Id: TSaftProc.cxx 2208 2014-04-02 15:30:23Z linev $
//-----------------------------------------------------------------------
//       The GSI Online Offline Object Oriented (Go4) Project
//         Experiment Data Processing at EE department, GSI
//-----------------------------------------------------------------------
// Copyright (C) 2000- GSI Helmholtzzentrum für Schwerionenforschung GmbH
//                     Planckstr. 1, 64291 Darmstadt, Germany
// Contact:            http://go4.gsi.de
//-----------------------------------------------------------------------
// This software can be used under the license agreements as stated
// in Go4License.txt file which is part of the distribution.
//-----------------------------------------------------------------------

#include "TSaftProc.h"

#include <stdlib.h>
#include <stdio.h>
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


#include "../saftdabc/Definitions.h"


#define SAFT_PROCID 0xa


//extern "C"
//{
//    INTS4 f_ut_utime(INTS4 l_sec, INTS4 l_msec, CHARS *pc_time);
//}
//
//#include "f_ut_utime.h"



//***********************************************************
TSaftProc::TSaftProc() : TGo4EventProcessor(), fLastEventNumber(0), fLastTime(0)
{


}

//***********************************************************
TSaftProc::~TSaftProc()
{
   TGo4Log::Info("TSaftProc: Delete instance");
}

//***********************************************************
// this one is used in standard factory
TSaftProc::TSaftProc(const char* name) : TGo4EventProcessor(name),fLastEventNumber(0), fLastTime(0)
{
   TGo4Log::Info("TSaftProc: Create instance %s", name);
   hDeltaN=MakeTH1('I',"DeltaN","MBS Eventnumber difference", 1000, 0,1000);
   hDeltaT=MakeTH1('I',"DeltaT","Timing Events  execution difference", 5000000, 0, 5000000,"us","N");    // resolution is milliseconds
   fPar=dynamic_cast<TSaftParam*>(MakeParameter("SaftParam", "TSaftParam", "set_SaftParam.C"));

}



//-----------------------------------------------------------
// event function
Bool_t TSaftProc::BuildEvent(TGo4EventElement*)
{  // called by framework. We dont fill any output event here at all

   if ((GetInputEvent()==0) || (GetInputEvent()->IsA() != TGo4MbsEvent::Class())) {
      TGo4Log::Error("TSaftProc: no input MBS event found!");
      return kFALSE;
   }

   TGo4MbsEvent* evnt = (TGo4MbsEvent*) GetInputEvent();

   if(evnt->GetTrigger() > 11) {
      TGo4Log::Info("TSaftProc: Skip trigger event");
      return kFALSE;
   }



   evnt->ResetIterator();
   TGo4MbsSubEvent *psubevt(0);
   while((psubevt = evnt->NextSubEvent()) != 0) { // loop over subevents

      //if (psubevt->GetProcid() != SAFT_PROCID) return kFALSE;
     if (psubevt->GetFullId() != fPar->fSubeventID) return kFALSE;

      Int_t* pdata = psubevt->GetDataField();
      Int_t* theEnd = psubevt->GetDataField() + psubevt->GetIntLen();
      char buf[1024];
      saftdabc::Timing_Event theEvent;
      uint64_t lo=0, hi=0;

      while(pdata<theEnd)
        {
          lo=(uint64_t)(*pdata++);
          hi=(uint64_t)(*pdata++);
          theEvent.fEvent = (hi<<32) + lo;
          lo=(uint64_t)(*pdata++);
          hi=(uint64_t)(*pdata++);
          theEvent.fParam = (hi<<32) + lo;
          lo=(uint64_t)(*pdata++);
          hi=(uint64_t)(*pdata++);
          theEvent.fDeadline = (hi<<32) + lo;
          lo=(uint64_t)(*pdata++);
          hi=(uint64_t)(*pdata++);
          theEvent.fExecuted = (hi<<32) + lo;
          lo=(uint64_t)(*pdata++);
          hi=(uint64_t)(*pdata++);
          theEvent.fFlags = (hi<<32) + lo;
          snprintf(theEvent.fDescription,SAFT_DABC_DESCRLEN, "%s", (const char*)(pdata));
          if(theEvent.InfoMessage(buf,1024)<0)
          {
            printf("Problem with info message!");
            pdata++;
            continue;
          }
          if(fPar->fVerbose)
          {
            printf("TSaftProc::BuildEvent sees %s",buf);
            std::cout << std::endl;
          }

//// DEBUG occurence of unwanted events?
          if(strstr(buf,"IO1")==0)
          {
            printf("TSaftProc::BuildEvent unexpected timing event %s",buf);
            std::cout << std::endl;
          }
/////////// end debug
          // here histograms of delta t and event numbers

          if(fLastTime>0)
            {
            uint64_t delta=theEvent.fExecuted - fLastTime;
            Double_t deltaus=delta/1.0e3; // ns to microseconds
            hDeltaT->Fill(deltaus);
            }
          fLastTime=theEvent.fExecuted;

          // skip size of description text:
          int textsize=SAFT_DABC_DESCRLEN/sizeof(Int_t);
          while(textsize--)
            {
              pdata++;
            }

        } // while pdata

   }// while((psubevt


   if(fLastEventNumber>0)
   {
       unsigned deltaN=evnt->GetCount() - fLastEventNumber;
       hDeltaN->Fill(deltaN);
   }
   fLastEventNumber=evnt->GetCount();
   return kTRUE;
}



