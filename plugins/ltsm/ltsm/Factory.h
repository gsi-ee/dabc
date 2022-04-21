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

#ifndef LTSM_Factory
#define LTSM_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace ltsm {

   class Factory : public dabc::Factory {
      public:
         Factory(const std::string &name) : dabc::Factory(name) {}

         void *CreateAny(const std::string &classname, const std::string &objname, dabc::Command cmd) override;
   };

}

#endif
