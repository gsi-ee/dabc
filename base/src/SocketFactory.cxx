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
#include "dabc/SocketTransport.h"
#include "dabc/SocketCommandChannel.h"
#include "dabc/NetworkTransport.h"
#include "dabc/Port.h"
#include "dabc/Url.h"

// as long as sockets integrated in libDabcBase, SocketFactory should be created directly by manager
// dabc::FactoryPlugin socketfactory(new dabc::SocketFactory("sockets"));


dabc::Reference dabc::SocketFactory::CreateObject(const std::string& classname, const std::string& objname, Command cmd)
{
   if (classname == "SocketCommandChannel") {

      dabc::SocketServerAddon* addon = 0;

      if (cmd.GetBool("WithServer", true)) {
         int nport = cmd.GetInt("ServerPort", 1237);
         int fd = dabc::SocketThread::StartServer(nport);
         if (fd<=0) {
            EOUT("Cannot open cmd socket on port %d", nport);
            return 0;
         }
         DOUT1("Start CMD SOCKET port:%d", nport);
         addon = new dabc::SocketServerAddon(fd, nport);
         addon->SetDeliverEventsToWorker(true);
      }

      return new SocketCommandChannel(objname, addon);
   }

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

dabc::Transport* dabc::SocketFactory::CreateTransport(const Reference& ref, const std::string& typ, Command cmd)
{
   dabc::PortRef port = ref;

   dabc::Url url(typ);
   if (url.IsValid() && (url.GetProtocol()=="udp") && !port.null()) {

      int fhandle = dabc::SocketThread::CreateUdp();
      if (fhandle<0) return 0;

      SocketNetworkInetrface* addon = 0;

      if (port.IsOutput()) {

         addon = new SocketNetworkInetrface(fhandle, true);

         addon->SetSendAddr(url.GetHostName(), url.GetPort());

      } else {

         int udp_port = dabc::SocketThread::BindUdp(fhandle, url.GetPort());

         if (udp_port<=0) {
            dabc::SocketThread::CloseUdp(fhandle);
            return 0;
         }

         bool mcast = url.HasOption("mcast");

         if (mcast && !dabc::SocketThread::AttachMulticast(fhandle, url.GetHostName())) {
            dabc::SocketThread::CloseUdp(fhandle);
            return 0;
         }

         addon = new SocketNetworkInetrface(fhandle, true);

         if (mcast) addon->SetMCastAddr(url.GetHostName());
      }


      PortRef inpport, outport;

      if (port.IsOutput()) {
         outport << port;
      } else {
         inpport << port;
      }

      return new dabc::NetworkTransport(dabc::Command(), inpport, outport, false, addon);
   }

   return dabc::Factory::CreateTransport(port, typ, cmd);
}


