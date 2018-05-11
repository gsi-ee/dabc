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

#include "fesa/Factory.h"

#include "fesa/Player.h"
#include "fesa/Monitor.h"

dabc::FactoryPlugin fesafactory(new fesa::Factory("fesa"));


dabc::Module* fesa::Factory::CreateModule(const std::string &classname, const std::string &modulename, dabc::Command cmd)
{
   if (classname == "fesa::Player")
      return new fesa::Player(modulename, cmd);

   if (classname == "fesa::Monitor")
      return new fesa::Monitor(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}

/** \page fesa_plugin FESA plugin for DABC (libDabcFesa.so)
 *
 *  \subpage fesa_plugin_doc
 *
 *  \ingroup dabc_plugins
 *
 */


/** \page fesa_plugin_doc Short description of FESA plugin
 *
 * This should be description of FESA plugin for DABC.
 *
 */
