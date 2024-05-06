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

#include "elderdabc/Factory.h"

#include "dabc/Url.h"
#include "dabc/Port.h"
#include "dabc/Manager.h"

#include "elderdabc/RunModule.h"

dabc::FactoryPlugin elderdabcfactory(new elderdabc::Factory("elder"));

dabc::Module* elderdabc::Factory::CreateModule(const std::string &classname, const std::string &modulename, dabc::Command cmd)
{
   if (classname == "elderdabc::RunModule")
      return new elderdabc::RunModule(modulename, cmd);



   return dabc::Factory::CreateModule(classname, modulename, cmd);
}



