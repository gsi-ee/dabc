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

#ifndef VERBS_Factory
#define VERBS_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

/** \brief Support of InfiniBand %verbs */

namespace verbs {

   /** \brief %Factory for VERBS classes */

   class Factory : public dabc::Factory {
      public:
         Factory(const std::string &name) : dabc::Factory(name) {}

         dabc::Reference CreateObject(const std::string &classname, const std::string &objname, dabc::Command cmd) override;

         dabc::Device* CreateDevice(const std::string &classname,
                                    const std::string &devname,
                                    dabc::Command cmd) override;

         dabc::Reference CreateThread(dabc::Reference parent, const std::string &classname, const std::string &thrdname, const std::string &thrddev, dabc::Command cmd) override;
   };
}

#endif
