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

#ifndef ROOT_Factory
#define ROOT_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace root {

   class Factory : public dabc::Factory {
      public:
         Factory(const std::string &name) : dabc::Factory(name) {}

         void Initialize() override;

         dabc::Reference CreateObject(const std::string &classname, const std::string &objname, dabc::Command cmd) override;

         dabc::DataInput *CreateDataInput(const std::string &) override;

         dabc::DataOutput* CreateDataOutput(const std::string &) override;
   };

}

#endif
