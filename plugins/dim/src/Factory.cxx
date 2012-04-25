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
#include "dimc/Observer.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"

dabc::FactoryPlugin dimcfactory(new dimc::Factory("dimc"));


dabc::Module* dimc::Factory::CreateModule(const char* classname, const char* modulename, dabc::Command cmd)
{

//   if (strcmp(classname, "dimc::Monitor")==0)
//      return new mbs::GeneratorModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}


void dimc::Factory::Initialize()
{
   DOUT0(("Initialize DIM control"));

   dimc::Observer* o = new dimc::Observer("/dim");

   dabc::mgr()->MakeThreadFor(o,"DimThread");

//   o->thread()()->SetLogging(true);
}
