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
#ifndef BNET_TestFactory
#define BNET_TestFactory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace bnet {

   extern const char* xmlTestWorkerClass;

   class TestFactory : public dabc::Factory {
   public:
      TestFactory(const char* name) : dabc::Factory(name) {}

      virtual dabc::Application* CreateApplication(const char* classname, dabc::Command cmd);

      virtual dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command cmd);
   };
}

#endif
