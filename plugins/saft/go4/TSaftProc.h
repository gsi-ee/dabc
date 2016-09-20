// $Id: TSaftProc.h 1549 2013-04-11 14:25:03Z linev $
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

#ifndef TSAFTPROCESSOR_H
#define TSAFTPROCESSOR_H

#include "TGo4EventProcessor.h"

#include "TH1.h"
#include "TH2.h"
#include "TGraph.h"

#include <inttypes.h>

#include "TSaftParam.h"

class TSaftProc : public TGo4EventProcessor {
   protected:


    TSaftParam* fPar;

   /** time difference of subsequent events*/
   TH1* hDeltaT;

   /** sequence number diff of events*/
   TH1* hDeltaN;



   /** remember last mbs event number received*/
   unsigned fLastEventNumber;

   /** remember last time stamp received*/
   uint64_t fLastTime;

   public:
      TSaftProc() ;
      TSaftProc(const char* name);
      virtual ~TSaftProc() ;

      Bool_t BuildEvent(TGo4EventElement*); // event processing function


   ClassDef(TSaftProc,1)
};

#endif //TUNPACKPROCESSOR_H
