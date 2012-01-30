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
#ifndef BNET_TestFilterModule
#define BNET_TestFilterModule

#include "dabc/ModuleAsync.h"

namespace bnet {

   class TestFilterModule : public dabc::ModuleAsync {
      public:
         TestFilterModule(const char* name, dabc::Command cmd = 0);

         virtual void ProcessUserEvent(dabc::ModuleItem* item, uint16_t evid);
   };
}

#endif
