/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#ifndef DABC_ROOT_Factory
#define DABC_ROOT_Factory

#ifndef __CINT__

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace mbs_root {

   class Factory : public dabc::Factory {
      public:
         Factory(const char* name) : dabc::Factory(name) {}

         virtual dabc::DataInput* CreateDataInput(const char* typ);

         virtual dabc::DataOutput* CreateDataOutput(const char* typ);
   };

}
#endif
#endif
