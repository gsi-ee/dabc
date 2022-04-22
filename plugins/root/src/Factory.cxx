// $Id$

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

#include "root/Factory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Url.h"

#include "root/Monitor.h"
#include "root/TreeStore.h"

dabc::FactoryPlugin root_factory(new root::Factory("root"));

void root::Factory::Initialize()
{
}

dabc::Reference root::Factory::CreateObject(const std::string &classname, const std::string &objname, dabc::Command cmd)
{
   if (classname=="root::Monitor")
      return new root::Monitor(objname, cmd);

   if (classname=="root::TreeStore")
      return new root::TreeStore(objname);

   return dabc::Factory::CreateObject(classname, objname, cmd);
}


dabc::DataInput* root::Factory::CreateDataInput(const std::string &typ)
{
   return nullptr;
}


dabc::DataOutput* root::Factory::CreateDataOutput(const std::string &typ)
{
   dabc::Url url(typ);

   if (url.GetProtocol() == "root")
      return nullptr;

   return nullptr;
}
