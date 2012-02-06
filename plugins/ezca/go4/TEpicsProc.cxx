// $Id$
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

#include "TEpicsProc.h"

#include <stdlib.h>
#include "Riostream.h"

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
   cout << "**** TEpicsProc: Delete instance " << endl;
}

//***********************************************************
// this one is used in standard factory
TEpicsProc::TEpicsProc(const char* name) : TGo4EventProcessor(name)
{
   cout << "**** TEpicsProc: Create instance " << name << endl;


   for(int i=0;i<_NUM_LONG_RECS_;i++) {
	   fLongRecords[i] = MakeTH1('I', Form("LongRecs/Long%02d",i), Form("Long Record %2d",i), 5000, 1., 5000);
   }

   for(int i=0;i<_NUM_DOUBLE_RECS_;i++) {
   	   fDoubleRecords[i] = MakeTH1('I', Form("DoubleRecs/Double%02d",i), Form("Double Record %2d",i), 5000, 1., 5000);
      }

}

//-----------------------------------------------------------
// event function
Bool_t TEpicsProc::BuildEvent(TGo4EventElement*)
{  // called by framework. We dont fill any output event here at all

   if ((GetInputEvent()==0) || (GetInputEvent()->IsA() != TGo4MbsEvent::Class())) {
      cout << "TEpicsProc: no input MBS event found!" << endl;
      return kFALSE;
   }

   TGo4MbsEvent* evnt = (TGo4MbsEvent*) GetInputEvent();

   if(evnt->GetTrigger() > 11) {
      cout << "**** TEpicsProc: Skip trigger event" << endl;
      return kFALSE;
   }

   evnt->ResetIterator();
   TGo4MbsSubEvent *psubevt(0);
   while((psubevt = evnt->NextSubEvent()) != 0) { // loop over subevents
	   cout << "**** TEpicsProc: found procid "<< psubevt->GetProcid()<< endl;
	   if (psubevt->GetProcid() != 8)
      			continue; // only evaluate epics events here

	   if(_VARPRINT_)
	  		   {
	  			   cout <<endl<<"Print Event number:"<< evnt->GetCount()<< endl;
	  		   }

	   int* pdata= psubevt->GetDataField();

	   // first we have event id:
	   if(_VARPRINT_)
		   {
			   cout <<endl<<"Got Event id:"<< pdata++ << endl;
		   }
	   // then sender time expression:
	  	   if(_VARPRINT_)
	  		   {
	  		   	   UInt_t timeseconds=*pdata++;
	  		   	   UInt_t timemicros=0;
	  		   	   char sbuf[1000]; f_ut_utime(timeseconds, timemicros, sbuf);
	  			   cout <<endl<<"time:"<< sbuf << endl;
	  		   }


	   // real payload: header with number of long records
	   int numlong=*pdata++;
	   for(int i=0; i<numlong;++i)
	   {
		   long val=*pdata++;
		   if(_VARPRINT_)
		   {
			   cout <<"Long "<<i<<"="<< val<< endl;
		   }
		   if(i>=_NUM_LONG_RECS_) continue;
		   fLongRecords[i]->Fill(val);
	   }
	   int numdub=*pdata++;
	   for(int i=0; i<numdub;++i)
	   {
		   double val=*pdata++;
		   if(_VARPRINT_)
		   {
			   cout <<"Double "<<i<<"="<< val<< endl;
		   }
		   if(i>=_NUM_DOUBLE_RECS_) continue;
		   fDoubleRecords[i]->Fill(val);
	   }

   }// while

   return kTRUE;
}
