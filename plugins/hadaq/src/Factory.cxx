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


dabc::FactoryPlugin hadaqfactory(new hadaq::Factory("hadaq"));

dabc::Transport* hadaq::Factory::CreateTransport(dabc::Reference ref, const char* typ, dabc::Command cmd)
{
   dabc::PortRef portref = ref;
   if (portref.null()) {
      EOUT(("Port not specified"));
      return 0;
   }
   if (strcmp(typ, hadaq::typeUdpInput)==0)
      {
       // FIXME!
        return (new hadaq::UdpDataSocket(portref));
      }
   else
      return dabc::Factory::CreateTransport(portref, typ, cmd);


   return 0;
}

dabc::DataInput* hadaq::Factory::CreateDataInput(const char* typ)
{
   if ((typ==0) || (strlen(typ)==0)) return 0;

   DOUT1(("Factory::CreateDataInput %s", typ));

   if (strcmp(typ, hadaq::typeHldInput)==0) {
      DOUT1(("HADAQ Factory creating HldInput"));
      return new hadaq::HldInput();
   }

//   else
//   if (strcmp(typ, hadaq::typeTextInput)==0) {
//      return new hadaq::TextInput();
//   }

   return 0;
}

dabc::DataOutput* hadaq::Factory::CreateDataOutput(const char* typ)
{
   if ((typ==0) || (strlen(typ)==0)) return 0;

   DOUT3(("Factory::CreateDataOutput typ:%s", typ));

   if (strcmp(typ, typeHldOutput)==0) {
      return new hadaq::HldOutput();
   }

   return 0;
}

dabc::Module* hadaq::Factory::CreateModule(const char* classname, const char* modulename, dabc::Command cmd)
{


   if (strcmp(classname, "hadaq::CombinerModule")==0)
      return new hadaq::CombinerModule(modulename, cmd);
   else
   if (strcmp(classname, "hadaq::MbsTransmitterModule")==0)
      return new hadaq::MbsTransmitterModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}


//dabc::Device* hadaq::Factory::CreateDevice(const char* classname, const char* devname, dabc::Command cmd)
//{
//   const char* thrdname = cmd.Field("Thread").AsStr();
//
//   if (strcmp(classname, hadaq::typeUdpDevice)==0) {
//      DOUT2(("hadaq::Factory::CreateDevice - Creating Hadaq Netmem UdpDevice %s ...", devname));
//
//      hadaq::UdpDevice* dev = new hadaq::UdpDevice(devname, thrdname, cmd);
//
//      if (!dev->IsConnected()) {
//         dabc::Object::Destroy(dev);
//         return 0;
//      }
//
//      return dev;
//   }
//
//   return 0;
//}

