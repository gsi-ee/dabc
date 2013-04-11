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

#ifndef ROC_READOUTMODULE_H
#define ROC_READOUTMODULE_H

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

namespace roc {

   class ReadoutModule : public dabc::ModuleAsync {
      protected:
         dabc::Condition fDataCond;
         dabc::Buffer   fCurrBuf; // this buffer accessed from user program without lock, means one cannot use it in module thread
         dabc::Buffer   fNextBuf; // this buffer provided by module to user, locked by fDataCond mutex

      public:

         ReadoutModule(const std::string& name, dabc::Command cmd = 0);
         virtual ~ReadoutModule();

         virtual bool ProcessRecv(unsigned port);

         virtual void AfterModuleStop();

         bool getNextBuffer(void* &buf, unsigned& len, double tmout = 1.);
   };
}

#endif
