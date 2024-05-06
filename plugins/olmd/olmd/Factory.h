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

#ifndef OLMD_Factory
#define OLMD_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

/** \brief Support for original LMD files by MBS (OLMD) -  */

namespace olmd {

   /** \brief %Factory of MBS classes */

   class Factory : public dabc::Factory {
      public:
         Factory(const std::string& name) : dabc::Factory(name) {}


         virtual dabc::DataInput* CreateDataInput(const std::string& typ) override;


      protected:

   };


   extern const char* protocolOlmd;


}

#endif
