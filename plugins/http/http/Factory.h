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

#ifndef MBS_Factory
#define MBS_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

/** \brief Support of HTTP in DABC */

namespace http {

   /** \brief %Factory for HTTP-related classes in DABC */

   class Factory : public dabc::Factory {
      public:
         Factory(const std::string& name) : dabc::Factory(name) {}

         virtual void Initialize();

      protected:

   };

}

#endif
