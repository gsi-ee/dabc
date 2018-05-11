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

#ifndef AQUA_Factory
#define AQUA_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

/** \brief Support for AQUA - A1 DAQ system */

namespace aqua {

   /** \brief %Factory of MBS classes */

   class Factory : public dabc::Factory {
      public:
         Factory(const std::string &name) : dabc::Factory(name) {}

         /** Factory method to create data output */
         virtual dabc::DataOutput* CreateDataOutput(const std::string &typ);
   };

}

#endif
