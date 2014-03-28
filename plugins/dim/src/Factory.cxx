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

#include "dimc/Factory.h"
#include "dimc/Player.h"

dabc::FactoryPlugin dimcfactory(new dimc::Factory("dimc"));

dabc::Module* dimc::Factory::CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd)
{
   if (classname == "dimc::Player")
      return new dimc::Player(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}
