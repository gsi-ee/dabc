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


dabc::Reference dabc::SocketFactory::CreateObject(const char* classname, const char* objname, Command cmd)
{
   if (strcmp(classname, "SocketCommandChannel")==0)
      return dabc::SocketCommandChannel::CreateChannel(objname);

   return 0;
}


dabc::Device* dabc::SocketFactory::CreateDevice(const char* classname,
                                                const char* devname, Command)
{
   if (strcmp(classname, dabc::typeSocketDevice)==0)
      return new SocketDevice(devname);

   return 0;
}

dabc::Reference dabc::SocketFactory::CreateThread(Reference parent, const char* classname, const char* thrdname, const char* thrddev, Command cmd)
{
   dabc::Thread* thrd = 0;

   if ((classname!=0) && strcmp(classname, typeSocketThread)==0)
      thrd = new SocketThread(parent, thrdname, cmd.GetInt("NumQueues", 3));

   return Reference(thrd);
}

dabc::FactoryPlugin socketfactory(new dabc::SocketFactory("sockets"));
