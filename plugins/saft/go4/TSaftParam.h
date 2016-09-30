//-------------------------------------------------------------
//        Go4 Release Package v3.03-05 (build 30305)
//                      05-June-2008
//---------------------------------------------------------------
//   The GSI Online Offline Object Oriented (Go4) Project
//   Experiment Data Processing at EE department, GSI
//---------------------------------------------------------------
//
//Copyright (C) 2000- Gesellschaft f. Schwerionenforschung, GSI
//                    Planckstr. 1, 64291 Darmstadt, Germany
//Contact:            http://go4.gsi.de
//----------------------------------------------------------------
//This software can be used under the license agreements as stated
//in Go4License.txt file which is part of the distribution.
//----------------------------------------------------------------
#ifndef SAFTPAR_H
#define SAFTPAR_H

#include "TGo4Parameter.h"


class TSaftParam : public TGo4Parameter {
 public:


      TSaftParam(const char* name=0);
      virtual ~TSaftParam();

      Bool_t fVerbose;  // print out received timing events

      Int_t fSubeventID; // saft mbs subevent id as defined in dabc set up

      Bool_t fHadaqMode; // switch to hadtu format inside mbs containers

    ClassDef(TSaftParam,2)
};

#endif //SPAR_H

//----------------------------END OF GO4 SOURCE FILE ---------------------
