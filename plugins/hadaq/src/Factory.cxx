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

#include "hadaq/Factory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/SocketThread.h"
#include "dabc/MemoryPool.h"
#include "dabc/Configuration.h"
#include "dabc/Port.h"
#include "dabc/Url.h"

#include "hadaq/HadaqTypeDefs.h"
#include "hadaq/HldInput.h"
#include "hadaq/HldOutput.h"
#include "hadaq/UdpTransport.h"
#include "hadaq/CombinerModule.h"
#include "hadaq/MbsTransmitterModule.h"
#include "hadaq/Observer.h"
#include "hadaq/api.h"


dabc::FactoryPlugin hadaqfactory(new hadaq::Factory("hadaq"));


dabc::DataInput* hadaq::Factory::CreateDataInput(const std::string& typ)
{
   dabc::Url url(typ);
   if (url.GetProtocol()=="hld") {
      DOUT1("HLD input file name %s", url.GetFullName().c_str());

      return new hadaq::HldInput(url);
   }

   return 0;
}

dabc::DataOutput* hadaq::Factory::CreateDataOutput(const std::string& typ)
{
   dabc::Url url(typ);
   if (url.GetProtocol()=="hld") {
      DOUT1("HLD output file name %s", url.GetFullName().c_str());
      return new hadaq::HldOutput(url);
   }

   return 0;
}

dabc::Transport* hadaq::Factory::CreateTransport(const dabc::Reference& port, const std::string& typ, dabc::Command cmd)
{
   dabc::Url url(typ);

   dabc::PortRef portref = port;

   if (portref.IsInput() && (url.GetProtocol()=="hadaq") && !url.GetHostName().empty()) {

      int nport = url.GetPort();
      int rcvbuflen = url.GetOptionInt("udpbuf", 200000);
      int mtu = url.GetOptionInt("mtu", 64512);
      double flush = url.GetOptionDouble(dabc::xml_flush, 1.);
      bool observer = url.GetOptionBool("observer", false);
      bool debug = url.HasOption("debug");


      if (nport>0) {

         int fd = DataSocketAddon::OpenUdp(nport, rcvbuflen);

         if (fd>0) {
            DataSocketAddon* addon = new DataSocketAddon(fd, nport, mtu, flush, debug);

            return new hadaq::DataTransport(cmd, portref, addon, observer);
         }
      }
   }

   return dabc::Factory::CreateTransport(port, typ, cmd);
}


dabc::Module* hadaq::Factory::CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd)
{
   if (classname == "hadaq::CombinerModule")
      return new hadaq::CombinerModule(modulename, cmd);

   if (classname == "hadaq::MbsTransmitterModule")
      return new hadaq::MbsTransmitterModule(modulename, cmd);

   if (classname == "hadaq::ReadoutModule")
      return new hadaq::ReadoutModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}


void hadaq::Factory::Initialize()
{
   dabc::XMLNodePointer_t node = 0;

   while (dabc::mgr()->cfg()->NextCreationNode(node, "Observer", true)) {

      // const char* name = dabc::Xml::GetAttr(node, dabc::xmlNameAttr);

      const char* thrdname = dabc::Xml::GetAttr(node, dabc::xmlThreadAttr);
      if ((thrdname==0) || (*thrdname==0)) thrdname = "ShmThread";

      dabc::WorkerRef w = new hadaq::Observer("/shm");
      w.MakeThreadForWorker(thrdname);
   }
}
