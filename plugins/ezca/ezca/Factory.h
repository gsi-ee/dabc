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

#ifndef EZCA_FACTORY_H
#define EZCA_FACTORY_H

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

/** \brief EPICS E-Z (easy) channel access */

namespace ezca {

   /** \brief %Factory for epics eazy channel access classes */

   class Factory: public dabc::Factory {

      public:

         Factory(const std::string &name);

         virtual dabc::DataInput* CreateDataInput(const std::string &typ);

         virtual dabc::Module* CreateModule(const std::string &classname, const std::string &modulename, dabc::Command cmd);
   };

}

#endif

