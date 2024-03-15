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

#include "dogma/Factory.h"

#include "dogma/TerminalModule.h"
#include "dogma/UdpTransport.h"
#include "dogma/Iterator.h"
#include "dogma/api.h"

dabc::FactoryPlugin dogmafactory(new dogma::Factory("dogma"));

dabc::Reference dogma::Factory::CreateObject(const std::string &classname, const std::string &objname, dabc::Command cmd)
{
   if (classname == "dogma_iter")
      return new dogma::RawIterator(objname);

   return dabc::Factory::CreateObject(classname, objname, cmd);
}

dabc::Module* dogma::Factory::CreateModule(const std::string &classname, const std::string &modulename, dabc::Command cmd)
{
   if (classname == "dogma::TerminalModule")
      return new dogma::TerminalModule(modulename, cmd);

  if (classname == "dogma::ReadoutModule")
      return new dogma::ReadoutModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}

dabc::Module* dogma::Factory::CreateTransport(const dabc::Reference& port, const std::string &typ, dabc::Command cmd)
{
   dabc::Url url(typ);

   dabc::PortRef portref = port;

   if (!portref.IsInput() || url.GetFullName().empty() || (url.GetProtocol() != "dogma"))
      return dabc::Factory::CreateTransport(port, typ, cmd);

   std::string portname = portref.GetName();

   int nport = url.GetPort();
   if (nport <= 0) {
      EOUT("Port not specified");
      return nullptr;
   }

   int rcvbuflen = url.GetOptionInt("udpbuf", 200000);
   int fd = dogma::UdpAddon::OpenUdp(url.GetHostName(), nport, rcvbuflen);
   if (fd<=0) {
      EOUT("Cannot open UDP socket for %s", url.GetHostNameWithPort().c_str());
      return nullptr;
   }

   int mtu = url.GetOptionInt("mtu", 64512);
   int maxloop = url.GetOptionInt("maxloop", 100);
   double flush = url.GetOptionDouble(dabc::xml_flush, 1.);
   double reduce = url.GetOptionDouble("reduce", 1.);
   double lost_rate = url.GetOptionDouble("lost", 0);
   bool debug = url.HasOption("debug");
   bool print = url.HasOption("print");
   int udp_queue = url.GetOptionInt("upd_queue", 0);
   double heartbeat = url.GetOptionDouble("heartbeat", -1.);

   if (udp_queue>0) cmd.SetInt("TransportQueue", udp_queue);

   DOUT0("Start DOGMA UDP transport on %s", url.GetHostNameWithPort().c_str());

   auto addon = new dogma::UdpAddon(fd, nport, mtu, debug, print, maxloop, reduce, lost_rate);
	return new dogma::UdpTransport(cmd, portref, addon, flush, heartbeat);
}
