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
   if (classname == "SocketCommandChannel")
      return dabc::SocketCommandChannel::CreateChannel(objname);

   if (classname == "SocketCommandChannelNew")
      return dabc::SocketCommandChannelNew::CreateChannel(objname);


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

      int udp_port = url.GetPort();
      if (udp_port==0) {
         udp_port = 4567;
         DOUT0("Multicast port not specified - use default %d", udp_port);
      }

      bool isrecv = port.IsInput();

      SocketNetworkInetrface* addon = 0;

      if (url.HasOption("mcast")) {

         int fhandle = -1;

         if (isrecv) {
            fhandle = dabc::SocketThread::StartMulticast(url.GetHostName(), udp_port, isrecv);
         } else {
            fhandle = dabc::SocketThread::CreateUdp();
         }

         DOUT0("MULTICAST handle:%d", fhandle);

         if (fhandle<0) return 0;

         addon = new SocketNetworkInetrface(fhandle, true);

         if (isrecv) {
            addon->SetMCastAddr(url.GetHostName(), udp_port, isrecv);
            addon->SetLogging(true);
         } else {
            addon->SetSendAddr(url.GetHostName(), udp_port);

         }
      } else {

         int fhandle = -1;

         fhandle = dabc::SocketThread::StartUdp(udp_port);

         DOUT0("Start UDP server at port %d  fd:%d", udp_port, fhandle);

         fhandle = dabc::SocketThread::ConnectUdp(fhandle, url.GetHostName(), udp_port);

         DOUT0("Connect UDP to remote host %s port %d  fd:%d", url.GetHostName().c_str(), udp_port, fhandle);

         if (fhandle<=0) return 0;

         addon = new SocketNetworkInetrface(fhandle, true);
      }

/*
      if (isrecv) {
         fhandle = dabc::SocketThread::CreateUdp();
         fhandle = dabc::SocketThread::ConnectUdp(fhandle, url.GetHostName(), udp_port);
      } else {
         fhandle = dabc::SocketThread::StartUdp(udp_port);
      }
*/


      PortRef inpport, outport;

      if (isrecv) {
        inpport << port;
      } else {
        outport << port;
      }

      return new dabc::NetworkTransport(dabc::Command(), inpport, outport, false, addon);
   }

   return dabc::Factory::CreateTransport(port, typ, cmd);
}


