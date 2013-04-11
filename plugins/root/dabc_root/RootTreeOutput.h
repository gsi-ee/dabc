/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#ifndef DABC_RootTreeOutput
#define DABC_RootTreeOutput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

// This is class to access TTree interface from the DABC

class TTree;

namespace dabc_root {

   class RootTreeOutput : public dabc::DataOutput {
      public:

         RootTreeOutput();
         virtual ~RootTreeOutput();

         virtual bool Write_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd) { return false; }

         virtual unsigned Write_Buffer(dabc::Buffer& buf);

      protected:

         TTree*       fTree;
   };

}

#endif
