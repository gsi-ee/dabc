// $Id$

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

#include "ltsm/Factory.h"

#include "ltsm/FileInterface.h"

#include "dabc/logging.h"

dabc::FactoryPlugin ltsmfactory(new ltsm::Factory("ltsm"));

void* ltsm::Factory::CreateAny(const std::string &classname, const std::string&, dabc::Command)
{
   if (classname == "ltsm::FileInterface") {
      DOUT0("Create file interface instance for the LTSM");
      return new ltsm::FileInterface;
   }

   return nullptr;
}
