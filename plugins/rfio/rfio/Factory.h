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

#ifndef RFIO_Factory
#define RFIO_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace rfio {

   class Factory : public dabc::Factory {
      public:
         Factory(const std::string& name) : dabc::Factory(name) {}

   };

}

#endif
