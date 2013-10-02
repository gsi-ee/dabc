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
#include "dabc/Port.h"
#include "dabc/Url.h"

#include "hadaq/HadaqTypeDefs.h"
#include "hadaq/HldInput.h"
#include "hadaq/HldOutput.h"
#include "hadaq/UdpTransport.h"
#include "hadaq/CombinerModule.h"
#include "hadaq/MbsTransmitterModule.h"
#include "hadaq/Observer.h"

dabc::FactoryPlugin hadaqfactory(new hadaq::Factory("hadaq"));


dabc::DataInput* hadaq::Factory::CreateDataInput(const std::string& typ)
{
   dabc::Url url(typ);
   if (url.GetProtocol()=="hld") {
      DOUT0("HLD input file name %s", url.GetFullName().c_str());

      return new hadaq::HldInput(url);
   }

   return 0;
}

dabc::DataOutput* hadaq::Factory::CreateDataOutput(const std::string& typ)
{
   dabc::Url url(typ);
   if (url.GetProtocol()=="hld") {
      DOUT0("HLD output file name %s", url.GetFullName().c_str());
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
      int rcvbuflen = url.GetOptionInt("buf", 1 << 20);

      nport = portref.Cfg(hadaq::xmlUdpPort, cmd).AsInt(nport);
      rcvbuflen = portref.Cfg(hadaq::xmlUdpBuffer, cmd).AsInt(rcvbuflen);
      int mtu = portref.Cfg(hadaq::xmlMTUsize, cmd).AsInt(63*1024);

      if (nport>0) {

         int fd = DataSocketAddon::OpenUdp(nport, rcvbuflen);

         if (fd>0) {
            DataSocketAddon* addon = new DataSocketAddon(fd, nport, mtu);

            return new hadaq::DataTransport(cmd, portref, addon);
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

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}


void hadaq::Factory::Initialize()
{
   hadaq::Observer* observ = new hadaq::Observer("/shm");
   dabc::WorkerRef w = observ;
   if (!observ->IsEnabled()) {
      w.Destroy();
   } else {
      DOUT0("Initialize SHM connected control");
      w.MakeThreadForWorker("ShmThread");
   }
}
