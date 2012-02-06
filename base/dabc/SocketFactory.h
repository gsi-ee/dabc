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

   class SocketFactory : public Factory {
      public:
         SocketFactory(const char* name) : Factory(name) {}

         virtual Reference CreateObject(const char* classname, const char* objname, Command cmd);

         virtual Device* CreateDevice(const char* classname,
                                      const char* devname,
                                      Command);

         virtual Reference CreateThread(Reference parent, const char* classname, const char* thrdname, const char* thrddev, Command cmd);
   };

}

#endif
