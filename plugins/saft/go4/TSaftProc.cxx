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

#include "TSaftProc.h"

#include <cstdlib>
#include <cstdio>
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
#include "../../hadaq/hadaq/defines.h"


#define SAFT_PROCID 0xa


//extern "C"
//{
//    INTS4 f_ut_utime(INTS4 l_sec, INTS4 l_msec, CHARS *pc_time);
//}
//
//#include "f_ut_utime.h"



//***********************************************************
TSaftProc::TSaftProc() : TGo4EventProcessor(), fLastEventNumber(0), fLastTime(0),fLastTimeDeadline(0), fLastFlipTime(0)
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
   hDeltaT=MakeTH1('I',"DeltaT","Timing Events  execution difference", 1000000, 0, 1000000,"us","N");    // resolution is milliseconds
   hDeltaT_deadline=MakeTH1('I',"DeltaT_deadline","Timing Events  deadline difference", 1000000, 0, 1000000,"us","N");

   hDeltaT_coarse=MakeTH1('I',"DeltaT_coarse","Coarse Timing Events  execution difference",
       10000, 0, 1.0e12,"ns","N"); // 100s range, 10 ms resolution


   hDeltaT_deadline_fine=MakeTH1('I',"DeltaT_deadline_fine","Timing Events deadline fine difference", 1000000, 0, 1000000,"ns","N");

   hLostSequence=MakeTH1('I',"LostEdges","Lost rising or falling edges",2,0,2);
   if(IsObjMade())
   {
     hLostSequence->GetXaxis()->SetBinLabel(1,"Rising");
     hLostSequence->GetXaxis()->SetBinLabel(2,"Falling");
   }


   hOverflowCounts=MakeTH1('I',"OverflowCounts","Relative Action sink overflows",1000,0,1000);
   hOverflowScaler=MakeTH1('I',"OverflowScaler","Action sink Overflow sum",2,0,2);
   if(IsObjMade())
     {
       hOverflowScaler->GetXaxis()->SetBinLabel(1,"No loss");
       hOverflowScaler->GetXaxis()->SetBinLabel(2,"Overflows");
     }


   fPar=dynamic_cast<TSaftParam*>(MakeParameter("SaftParam", "TSaftParam", "set_SaftParam.C"));

}



//-----------------------------------------------------------
// event function
Bool_t TSaftProc::BuildEvent(TGo4EventElement*)
{  // called by framework. We dont fill any output event here at all
   static saftdabc::Timing_Event lastEvent;

   if (!GetInputEvent() || (GetInputEvent()->IsA() != TGo4MbsEvent::Class())) {
      TGo4Log::Error("TSaftProc: no input MBS event found!");
      return kFALSE;
   }

   TGo4MbsEvent* evnt = (TGo4MbsEvent*) GetInputEvent();

   if(evnt->GetTrigger() > 11) {
      TGo4Log::Info("TSaftProc: Skip trigger event");
      return kFALSE;
   }

   evnt->ResetIterator();
   TGo4MbsSubEvent *psubevt = nullptr;
   while((psubevt = evnt->NextSubEvent()) != nullptr) { // loop over subevents

      //if (psubevt->GetProcid() != SAFT_PROCID) return kFALSE;
     if (psubevt->GetFullId() != fPar->fSubeventID) return kFALSE;

      Int_t* pdata = psubevt->GetDataField();
      Int_t* theEnd = psubevt->GetDataField() + psubevt->GetIntLen();
      Int_t* evtEnd = theEnd;
      Int_t* evtPaddedEnd = theEnd;
      Int_t* subEnd = theEnd;
      Int_t* subPaddedEnd = theEnd;
      hadaq::RawEvent* hadevt = nullptr;
      hadaq::RawSubevent* hadsub = nullptr;

      if(fPar->fHadaqMode)
      {
        // skip hades event and subevent headers:
        hadevt = (hadaq::RawEvent*) pdata;
        pdata += sizeof(hadaq::RawEvent)/sizeof(Int_t);
        hadsub = (hadaq::RawSubevent*) pdata;
        pdata += sizeof(hadaq::RawSubevent)/sizeof(Int_t);
          // adjust end of payload with respect to hades padding
        evtEnd=   (Int_t*)( ((char*) hadevt) + hadevt->GetSize());
        evtPaddedEnd=   (Int_t*)( ((char*) hadevt) + hadevt->GetPaddedSize());
        subEnd=(Int_t*)( ((char*) hadsub) + hadsub->GetSize());
        subPaddedEnd=(Int_t*)( ((char*) hadsub) + hadsub->GetPaddedSize());
        if(fPar->fVerbose)
            {
                printf("TSaftProc::BuildEvent sees first hadaq event in buffer of size %d, padded:%d, subevt size:%d padded:%d\n",
                    hadevt->GetSize(), hadevt->GetPaddedSize(),hadsub->GetSize(), hadsub->GetPaddedSize());
                std::cout << std::endl;
            }
      }

      char buf[1024];
      saftdabc::Timing_Event theEvent;
      uint64_t lo=0, hi=0;

      while(pdata<theEnd)
        {
            // check here for ends of hades events and subevents in mbs:
            if(fPar->fHadaqMode)
              {
                  if(pdata==evtEnd)
                    {



                      // new hades event in buffer:
                      pdata=evtPaddedEnd+1;
                      if(pdata>=theEnd) break;
                      hadevt = (hadaq::RawEvent*) pdata;
                      pdata += sizeof(hadaq::RawEvent)/sizeof(Int_t);
                      hadsub = (hadaq::RawSubevent*) pdata;
                      pdata += sizeof(hadaq::RawSubevent)/sizeof(Int_t);
                      evtEnd=   (Int_t*)( ((char*) hadevt) + hadevt->GetSize());
                      evtPaddedEnd=   (Int_t*)( ((char*) hadevt) + hadevt->GetPaddedSize());
                      subEnd=(Int_t*)( ((char*) hadsub) + hadsub->GetSize());
                      subPaddedEnd=(Int_t*)( ((char*) hadsub) + hadsub->GetPaddedSize());
                      if(fPar->fVerbose)
                         {
                           printf("TSaftProc::BuildEvent sees new hadaq event of size %d, padded:%d, subevt size:%d padded:%d\n",
                               hadevt->GetSize(), hadevt->GetPaddedSize(),hadsub->GetSize(), hadsub->GetPaddedSize());
                           std::cout << std::endl;
                         }
                      continue;
                    }

                if(pdata==subEnd)
                {
                    // new hades subevent in buffer
                    pdata=subPaddedEnd+1;
                    if(pdata>=theEnd) break;
                    hadsub = (hadaq::RawSubevent*) pdata;
                    pdata += sizeof(hadaq::RawSubevent)/sizeof(Int_t);
                    subEnd=(Int_t*)( ((char*) hadsub) + hadsub->GetSize());
                    subPaddedEnd=(Int_t*)( ((char*) hadsub) + hadsub->GetPaddedSize());
                    if(fPar->fVerbose)
                        {
                          printf("TSaftProc::BuildEvent sees new hadaq subevent of size:%d padded:%d\n",
                              hadsub->GetSize(), hadsub->GetPaddedSize());
                          std::cout << std::endl;
                        }

                    continue;
                }

              }




          lo=(uint64_t)(*pdata++);
          hi=(uint64_t)(*pdata++);
          theEvent.fEvent = (hi<<32) | (lo & 0xFFFFFFFF);
          lo=(uint64_t)(*pdata++);
          hi=(uint64_t)(*pdata++);
          theEvent.fParam = (hi<<32) | (lo & 0xFFFFFFFF);
          lo=(uint64_t)(*pdata++);
          hi=(uint64_t)(*pdata++);
          theEvent.fDeadline = (hi<<32) | (lo & 0xFFFFFFFF);
          lo=(uint64_t)(*pdata++);
          hi=(uint64_t)(*pdata++);
          theEvent.fExecuted = (hi<<32) | (lo & 0xFFFFFFFF);
          lo=(uint64_t)(*pdata++);
          hi=(uint64_t)(*pdata++);
          theEvent.fFlags = (hi<<32) | (lo & 0xFFFFFFFF);
          lo=(uint64_t)(*pdata++);
          hi=(uint64_t)(*pdata++);
          theEvent.fOverflows = (hi<<32) | (lo & 0xFFFFFFFF);


          snprintf(theEvent.fDescription,SAFT_DABC_DESCRLEN, "%s", (const char*)(pdata));
          if(theEvent.InfoMessage(buf,1024)<0)
          {
            printf("Problem with info message!");
            pdata++;
            continue;
          }
          if(fPar->fVerbose)
          {
            printf("TSaftProc::BuildEvent of event %d sees %s\n",evnt->GetCount(), buf);
            printf("Date: %s",TSaftProc::FormatDate(theEvent.fExecuted, GO4_PMODE_VERBOSE).c_str());

            std::cout << std::endl;
          }

//// DEBUG occurrence of unwanted events?
//          if(strstr(buf,"IO1") == 0)
//          {
//            printf("TSaftProc::BuildEvent unexpected timing event %s",buf);
//            std::cout << std::endl;
//          }
/////////// end debug
          // here histograms of delta t and event numbers

          if(fLastTime>0)
            {
            uint64_t delta=theEvent.fExecuted - fLastTime;
            hDeltaT_coarse->Fill(delta);
            Double_t deltaus=delta/1.0e3; // ns to microseconds
            hDeltaT->Fill(deltaus);

            uint64_t deltaDead=theEvent.fDeadline - fLastTimeDeadline;
            Double_t deltadeadaus=deltaDead/1.0e3; // ns to microseconds
            hDeltaT_deadline->Fill(deltadeadaus);



            hDeltaT_deadline_fine->Fill(deltaDead);
            // DEBUG big deltas:
            if(delta>1.0e+9)
            {
              printf("******** Found delta= 0x%lx units (%ld ns) (%E us)\n", delta, delta, deltaus);
              printf("** %s\n",buf);
              printf("** Last time:0x%lx\n",fLastTime);
              lastEvent.InfoMessage(buf,1024);
              printf("** Last Event: %s\n",buf);
              uint64_t deltaflip=fLastTime - fLastFlipTime;
              printf("** Delta since last flip=0x%lx units\n",deltaflip);
              printf("*************************************");
              std::cout << std::endl;


              fLastFlipTime=fLastTime;
            }

            if((lastEvent.fEvent & 0x1) == (theEvent.fEvent & 0x1))
            {
              // same type of input edges (rising or falling) since last event. not possible with pulser test

              if(theEvent.fEvent & 0x1)
                hLostSequence->Fill(1); // we are rising edge and have lost a falling edge before
              else
                hLostSequence->Fill(0); // we are falling edge and have lost a rising before
            }

            if(theEvent.fOverflows)
            {
                hOverflowCounts->Fill(theEvent.fOverflows);
                hOverflowScaler->Fill(1,theEvent.fOverflows);
            }
            else
            {
                hOverflowScaler->Fill(0);
            }

          } //if(fLastTime>0)
          fLastTime=theEvent.fExecuted;
          fLastTimeDeadline=theEvent.fDeadline;
          lastEvent=theEvent;



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


/** stolen from saftlib to check the real time stamp delivered:*/
std::string TSaftProc::FormatDate(uint64_t time, uint32_t pmode)
{
  uint64_t ns    = time % 1000000000;
  time_t  s     = time / 1000000000;
  struct tm *tm = gmtime(&s);
  char date[40];
  char full[80];
  std::string temp;

  if (pmode & GO4_PMODE_VERBOSE) {
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
    snprintf(full, sizeof(full), "%s.%09ld", date, (long)ns);
  }
  else if (pmode & GO4_PMODE_DEC)
    snprintf(full, sizeof(full), "0d%lu.%09ld",s,(long)ns);
  else
    snprintf(full, sizeof(full), "0x%016llx", (unsigned long long)time);

  temp = full;

  return temp;
} //tr_formatDate

