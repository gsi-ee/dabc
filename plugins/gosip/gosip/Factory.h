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

#ifndef GOSIP_Factory
#define GOSIP_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

/** \brief Support for GOSIP - communication protocol for different software kinds */

namespace gosip {

   /** \brief %Factory of MBS classes */

   class Factory : public dabc::Factory {
      public:
         Factory(const std::string& name) : dabc::Factory(name) {}

         virtual dabc::Module* CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd);

      protected:

   };

}

#endif
