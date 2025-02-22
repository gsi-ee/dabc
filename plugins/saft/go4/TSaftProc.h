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

#ifndef TSAFTPROCESSOR_H
#define TSAFTPROCESSOR_H

#include "TGo4EventProcessor.h"

#include "TH1.h"
#include "TH2.h"
#include "TGraph.h"

#include <inttypes.h>

#include "TSaftParam.h"


#define GO4_PMODE_NONE  0x0
#define GO4_PMODE_DEC   0x1
#define GO4_PMODE_HEX   0x2
#define GO4_PMODE_VERBOSE  0x4


class TSaftProc : public TGo4EventProcessor {
   protected:


    TSaftParam* fPar;

   /** "executed" time difference of subsequent events*/
   TH1* hDeltaT;

   /** "deadline" time difference of subsequent events*/
   TH1* hDeltaT_deadline;

   /** time difference of subsequent events, coarse range overview*/
   TH1* hDeltaT_coarse;

   /** time difference of subsequent events, fine detail of deadline*/
   TH1* hDeltaT_deadline_fine;

   /** sequence number diff of events*/
   TH1* hDeltaN;

   /** counts lost events by checking leading/trailing edge alternation*/
   TH1* hLostSequence;

   /** histogram of increase of overflow counter as taken from action sink. Part of our timing event structure*/
   TH1* hOverflowCounts;


   /** scaler for total overflow counter versus timing events without loss*/
   TH1* hOverflowScaler;

   /** remember last mbs event number received*/
   unsigned fLastEventNumber;

   /** remember last time stamp (executed) received*/
   uint64_t fLastTime;


   /** remember last time stamp (deadling) received*/
   uint64_t fLastTimeDeadline;

   /** remember last time stamp of large deltat*/
   uint64_t fLastFlipTime;


   public:
      TSaftProc() ;
      TSaftProc(const char *name);
      virtual ~TSaftProc() ;

      Bool_t BuildEvent(TGo4EventElement*); // event processing function


      static std::string FormatDate(uint64_t time, uint32_t pmode);


   ClassDef(TSaftProc,1)
};

#endif //TUNPACKPROCESSOR_H
