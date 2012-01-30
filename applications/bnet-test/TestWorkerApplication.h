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
#ifndef BNET_TestWorkerApplication
#define BNET_TestWorkerApplication

#include "bnet/WorkerApplication.h"

namespace bnet {

   class TestWorkerApplication : public WorkerApplication {
      public:
         TestWorkerApplication();

         virtual bool CreateReadout(const char* portname, int portnumber);

         virtual bool CreateCombiner(const char* name);
         virtual bool CreateBuilder(const char* name);
         virtual bool CreateFilter(const char* name);

         bool CreateStorage(const char* portname);

      protected:
         virtual void DiscoverNodeConfig(dabc::Command cmd);

      private:
         bool fABBActive;

   };
}

#endif
