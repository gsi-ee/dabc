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

#ifndef DABC_SocketFactory
#define DABC_SocketFactory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace dabc {

   /** \brief Factory for socket classes
    *
    * \ingroup dabc_all_classes
    */

   class SocketFactory : public Factory {
      public:
         SocketFactory(const std::string& name) : Factory(name) {}

         virtual Reference CreateObject(const std::string& classname, const std::string& objname, Command cmd);

         virtual Device* CreateDevice(const std::string& classname, const std::string& devname, Command cmd);

         virtual Reference CreateThread(Reference parent, const std::string& classname, const std::string& thrdname, const std::string& thrddev, Command cmd);
   };

}

#endif
