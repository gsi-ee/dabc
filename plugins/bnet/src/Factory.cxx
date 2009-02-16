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
#include "bnet/Factory.h"

#include "bnet/common.h"
#include "bnet/ClusterApplication.h"
#include "bnet/GlobalDFCModule.h"
#include "bnet/SenderModule.h"
#include "bnet/ReceiverModule.h"

bnet::Factory bnetfactory("bnet");

bnet::Factory::Factory(const char* name) :
   dabc::Factory(name)
{
}


dabc::Application* bnet::Factory::CreateApplication(const char* classname, dabc::Command* cmd)
{
   if (strcmp(classname, xmlClusterClass)==0)
      return new bnet::ClusterApplication();

   return dabc::Factory::CreateApplication(classname, cmd);
}

dabc::Module* bnet::Factory::CreateModule(const char* classname, const char* modulename, dabc::Command* cmd)
{

   if (strcmp(classname, "bnet::GlobalDFCModule")==0)
      return new bnet::GlobalDFCModule(modulename, cmd);
   else
   if (strcmp(classname, "bnet::SenderModule")==0)
      return new bnet::SenderModule(modulename, cmd);
   else
   if (strcmp(classname, "bnet::ReceiverModule")==0)
      return new bnet::ReceiverModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}
