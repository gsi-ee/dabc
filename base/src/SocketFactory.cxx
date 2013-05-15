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

#include "dabc/SocketFactory.h"

#include "dabc/SocketDevice.h"
#include "dabc/SocketThread.h"
#include "dabc/SocketCommandChannel.h"

// as long as sockets integrated in libDabcBase, SocketFactory should be created directly by manager
// dabc::FactoryPlugin socketfactory(new dabc::SocketFactory("sockets"));


dabc::Reference dabc::SocketFactory::CreateObject(const std::string& classname, const std::string& objname, Command cmd)
{
   if (classname == "SocketCommandChannel")
      return dabc::SocketCommandChannel::CreateChannel(objname);

   return 0;
}


dabc::Device* dabc::SocketFactory::CreateDevice(const std::string& classname,
                                                const std::string& devname, Command)
{
   if (classname == dabc::typeSocketDevice)
      return new SocketDevice(devname);

   return 0;
}

dabc::Reference dabc::SocketFactory::CreateThread(Reference parent, const std::string& classname, const std::string& thrdname, const std::string& thrddev, Command cmd)
{
   dabc::Thread* thrd = 0;

   if (classname == typeSocketThread)
      thrd = new SocketThread(parent, thrdname, cmd);

   return Reference(thrd);
}

