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

#ifndef DOGMA_Factory
#define DOGMA_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

/** \brief Support for DOGMA  */

namespace dogma {

   /** \brief %Factory for HADAQ classes  */

   class Factory : public dabc::Factory {
      public:
         Factory(const std::string &name) : dabc::Factory(name) {}

         dabc::Reference CreateObject(const std::string &classname, const std::string &objname, dabc::Command cmd) override;

         dabc::Module *CreateModule(const std::string &classname, const std::string &modulename, dabc::Command cmd) override;

         dabc::Module *CreateTransport(const dabc::Reference& port, const std::string &typ, dabc::Command cmd) override;
   };

}

#endif
