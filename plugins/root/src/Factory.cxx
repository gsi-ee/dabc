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
#include "dabc_root/Factory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Url.h"

#include "dabc_root/RootSniffer.h"

dabc::FactoryPlugin dabc_root_factory(new dabc_root::Factory("dabc_root"));

void dabc_root::Factory::Initialize()
{
   dabc_root::RootSniffer* serv = new dabc_root::RootSniffer("/ROOT");
   if (!serv->IsEnabled()) {
      dabc::WorkerRef ref = serv;
      ref.SetOwner(true);
   } else {
      DOUT0("Initialize ROOT sniffer");
      dabc::WorkerRef(serv).MakeThreadForWorker("RootThread");
   }
}

dabc::Reference dabc_root::Factory::CreateObject(const std::string& classname, const std::string& objname, dabc::Command cmd)
{
   if (classname=="dabc_root::RootSniffer")
      return new dabc_root::RootSniffer(objname, cmd);

   return dabc::Factory::CreateObject(classname, objname, cmd);
}


dabc::DataInput* dabc_root::Factory::CreateDataInput(const std::string& typ)
{
   return 0;
}


dabc::DataOutput* dabc_root::Factory::CreateDataOutput(const std::string& typ)
{

   DOUT3("dabc_root::Factory::CreateDataOutput typ:%s", typ);

   dabc::Url url(typ);

   if (url.GetProtocol() == "root") {
      return 0;
   }

   return 0;
}
