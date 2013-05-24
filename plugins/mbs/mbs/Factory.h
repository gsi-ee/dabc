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

/** \brief Support for MBS - standard GSI DAQ */

namespace mbs {

   /** \brief %Factory of MBS classes */

   class Factory : public dabc::Factory {
      public:
         Factory(const std::string& name) : dabc::Factory(name) {}

         virtual dabc::Transport* CreateTransport(const dabc::Reference& port, const std::string& typ, dabc::Command cmd);

         virtual dabc::DataInput* CreateDataInput(const std::string& typ);

         virtual dabc::DataOutput* CreateDataOutput(const std::string& typ);

         virtual dabc::Module* CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd);

      protected:

   };

}

#endif
