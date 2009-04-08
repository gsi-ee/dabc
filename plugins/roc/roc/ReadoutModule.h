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

#ifndef ROC_ReadoutModule
#define ROC_ReadoutModule

#include "dabc/ModuleAsync.h"

namespace roc {

   class ReadoutModule : public dabc::ModuleAsync {
      protected:
         unsigned fBufferSize;

      public:

         ReadoutModule(const char* name, dabc::Command* cmd = 0);
         virtual ~ReadoutModule();

         virtual void ProcessInputEvent(dabc::Port* port);

         virtual void ProcessOutputEvent(dabc::Port* port);
   };
}

#endif
