/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "rfio/Factory.h"

#include "rfio/FileInterface.h"

dabc::FactoryPlugin rfiofactory(new rfio::Factory("rfio"));

void* rfio::Factory::CreateAny(const std::string &classname, const std::string&, dabc::Command)
{
   if (classname == "rfio::FileInterface") {
      DOUT0("Create file interface instance for the RFIO");
      return new rfio::FileInterface;
   }

   return 0;
}
