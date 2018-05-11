// $Id: Factory.h 2190 2014-03-27 15:34:14Z linev $

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

#ifndef SAFT_FACTORY_H
#define SAFT_FACTORY_H

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

/** \brief GSI Simplified API for Timing Library (saftlib) plug-in for dabc*/

namespace saftdabc {

   /** \brief %Factory for saftlib plugin classes */

   class Factory: public dabc::Factory {

      public:

         Factory(const std::string &name);

         dabc::Device* CreateDevice (const std::string &classname, const std::string &devname,
             dabc::Command cmd);

   };

}

#endif

