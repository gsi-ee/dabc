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

#include "gosip/Factory.h"

#include "gosip/Player.h"

dabc::FactoryPlugin fesafactory(new gosip::Factory("gosip"));


dabc::Module* gosip::Factory::CreateModule(const std::string &classname, const std::string &modulename, dabc::Command cmd)
{
   if (classname == "gosip::Player")
      return new gosip::Player(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}

/** \page gosip_plugin GOSIP plugin for DABC (libDabcGosip.so)
 *
 *  \subpage gosip_plugin_doc
 *
 *  \ingroup dabc_plugins
 *
 */


/** \page gosip_plugin_doc Short description of GOSIP plugin
 *
 * This should be description of GOSIP plugin for DABC.
 *
 */
