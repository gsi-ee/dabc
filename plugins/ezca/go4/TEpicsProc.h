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

#ifndef TUNPACKPROCESSOR_H
#define TUNPACKPROCESSOR_H

#include "TGo4EventProcessor.h"


#define _NUM_LONG_RECS_ 8
#define _NUM_DOUBLE_RECS_ 8

#define _VARPRINT_ 1

class TEpicsProc : public TGo4EventProcessor {
   public:
      TEpicsProc() ;
      TEpicsProc(const char* name);
      virtual ~TEpicsProc() ;

      Bool_t BuildEvent(TGo4EventElement*); // event processing function

   private:
      TH1           *fLongRecords[_NUM_LONG_RECS_];
      TH1           *fDoubleRecords[_NUM_DOUBLE_RECS_];



   ClassDef(TEpicsProc,1)
};

#endif //TUNPACKPROCESSOR_H
