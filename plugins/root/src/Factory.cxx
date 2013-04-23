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
#include "dabc/Command.h"
#include "dabc/Url.h"
#include "dabc/Manager.h"

dabc::FactoryPlugin dabc_root_factory(new dabc_root::Factory("dabc_root"));

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
