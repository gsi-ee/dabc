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
#include "TestFactory.h"

#include "TestWorkerApplication.h"
#include "TestGeneratorModule.h"
#include "TestCombinerModule.h"
#include "TestBuilderModule.h"
#include "TestFilterModule.h"

bnet::TestFactory bnettestfactory("bnet-test");

const char* bnet::xmlTestWorkerClass = "bnet::TestWorker";

dabc::Application* bnet::TestFactory::CreateApplication(const std::string& classname, dabc::Command cmd)
{
   if (classname == xmlTestWorkerClass)
      return new bnet::TestWorkerApplication();

   return dabc::Factory::CreateApplication(classname, cmd);
}

dabc::Module* bnet::TestFactory::CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd)
{
   if (classname == "bnet::TestGeneratorModule")
      return new bnet::TestGeneratorModule(modulename, cmd);

   if (classname == "bnet::TestCombinerModule")
      return new bnet::TestCombinerModule(modulename, cmd);

   if (classname == "bnet::TestBuilderModule")
      return new bnet::TestBuilderModule(modulename, cmd);

   if (classname == "bnet::TestFilterModule")
      return new bnet::TestFilterModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}
