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
#include "mbs_root/Factory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"

#include "mbs_root/RootTreeOutput.h"

dabc::FactoryPlugin mbsrootfactory(new mbs_root::Factory("mbs-root"));

dabc::DataInput* mbs_root::Factory::CreateDataInput(const std::string &typ)
{
   return nullptr;
}


dabc::DataOutput* mbs_root::Factory::CreateDataOutput(const std::string &typ)
{
   DOUT3("mbs_root::Factory::CreateDataOutput typ:%s", typ.c_str());

   dabc::Url url(typ);

   if (url.GetProtocol() == "mbsroot")
      return new mbs_root::RootTreeOutput(url);

   return nullptr;
}
