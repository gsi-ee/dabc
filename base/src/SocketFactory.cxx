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

#include <sys/types.h>
#include <unistd.h>

#include "dabc/SocketDevice.h"
#include "dabc/SocketTransport.h"
#include "dabc/SocketCommandChannel.h"
#include "dabc/Url.h"

// as long as sockets integrated into libDabcBase, SocketFactory should be created directly by manager
// dabc::FactoryPlugin socketfactory(new dabc::SocketFactory("sockets"));


void dabc::SocketFactory::Initialize()
{
}


dabc::Reference dabc::SocketFactory::CreateObject(const std::string &classname, const std::string &objname, Command cmd)
{
   if (classname == "SocketCommandChannel") {

      dabc::SocketServerAddon* addon = nullptr;

      if (cmd.GetBool("WithServer", true)) {
         std::string host = cmd.GetStr("ServerHost");
         int nport = cmd.GetInt("ServerPort");
         if (nport <= 0) nport = dabc::defaultDabcPort;

         addon = dabc::SocketThread::CreateServerAddon(host, nport);
         if (!addon) {
            EOUT("Cannot open cmd socket on port %d", nport);
            return nullptr;
         }
         addon->SetDeliverEventsToWorker(true);

         cmd.SetStr("localaddr", addon->ServerId());
         DOUT0("Start DABC server on %s", addon->ServerId().c_str());
      }

      return new SocketCommandChannel(objname, addon, cmd);
   }

   return nullptr;
}


dabc::Device* dabc::SocketFactory::CreateDevice(const std::string &classname,
                                                const std::string &devname, Command cmd)
{
   if (classname == dabc::typeSocketDevice)
      return new SocketDevice(devname, cmd);

   return nullptr;
}

dabc::Reference dabc::SocketFactory::CreateThread(Reference parent, const std::string &classname, const std::string &thrdname, const std::string &thrddev, Command cmd)
{
   (void) thrddev;
   dabc::Thread* thrd = nullptr;

   if (classname == typeSocketThread)
      thrd = new SocketThread(parent, thrdname, cmd);

   return Reference(thrd);
}

dabc::Module* dabc::SocketFactory::CreateTransport(const Reference& ref, const std::string &typ, Command cmd)
{
   dabc::PortRef port = ref;

   dabc::Url url(typ);
   if (url.IsValid() && (url.GetProtocol()=="udp") && !port.null()) {

      int fhandle = dabc::SocketThread::CreateUdp();
      if (fhandle<0) return nullptr;

      SocketNetworkInetrface* addon = nullptr;

      if (port.IsOutput()) {

         addon = new SocketNetworkInetrface(fhandle, true);

         addon->SetSendAddr(url.GetHostName(), url.GetPort());

      } else {

         int udp_port = dabc::SocketThread::BindUdp(fhandle, url.GetPort());

         if (udp_port <= 0) {
            dabc::SocketThread::CloseUdp(fhandle);
            return nullptr;
         }

         bool mcast = url.HasOption("mcast");

         if (mcast && !dabc::SocketThread::AttachMulticast(fhandle, url.GetHostName())) {
            dabc::SocketThread::CloseUdp(fhandle);
            return nullptr;
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
