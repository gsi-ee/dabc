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

#ifndef FESA_Factory
#define FESA_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

/** \brief Support for FESA - the control system of GSI/FAIR accelerator */

namespace fesa {

   /** \brief %Factory of MBS classes */

   class Factory : public dabc::Factory {
      public:
         Factory(const std::string &name) : dabc::Factory(name) {}

         dabc::Module *CreateModule(const std::string &classname, const std::string &modulename, dabc::Command cmd) override;

   };

}

#endif
