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

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#include "nxyter/Data.h"
#include "nxyter/Sorter.h"

namespace roc {

   class ReadoutModule : public dabc::ModuleAsync {
      protected:
         unsigned fBufferSize;

         dabc::Condition fDataCond;
         dabc::Buffer*   fCurrBuf;
         dabc::BufferSize_t fCurrBufPos;

         nxyter::Sorter*  fSorter;
         unsigned fSorterPos; // position in sorter output buffer

      public:

         ReadoutModule(const char* name, dabc::Command* cmd = 0);
         virtual ~ReadoutModule();

         virtual void ProcessInputEvent(dabc::Port* port);

         virtual void ProcessOutputEvent(dabc::Port* port);

         virtual void AfterModuleStop();

         void setUseSorter(bool on = true);

         bool getNextData(nxyter::Data& data, double tmout = 1.);
   };
}

#endif
