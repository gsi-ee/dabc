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

#include "dim/Factory.h"
#include "dim/Monitor.h"

dabc::FactoryPlugin dimcfactory(new dim::Factory("dim"));

dabc::Module* dim::Factory::CreateModule(const std::string &classname, const std::string &modulename, dabc::Command cmd)
{
   if (classname == "dim::Monitor")
      return new dim::Monitor(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}
