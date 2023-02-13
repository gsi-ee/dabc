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

#include "dofi/Factory.h"

#include "dofi/Player.h"
#include "dofi/TerminalModule.h"

char dofi::Command::CommandDescription[DOFI_MAXTEXT];

dabc::FactoryPlugin dofifactory(new dofi::Factory("dofi"));

dabc::Module* dofi::Factory::CreateModule(const std::string &classname, const std::string &modulename, dabc::Command cmd)
{
   if (classname == "dofi::Player")
      return new dofi::Player(modulename, cmd);
   if (classname == "dofi::TerminalModule")
      return new dofi::TerminalModule(modulename, cmd);
   return dabc::Factory::CreateModule(classname, modulename, cmd);
}

