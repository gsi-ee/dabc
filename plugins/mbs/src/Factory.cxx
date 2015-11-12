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

#include "mbs/Factory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/SocketThread.h"
#include "dabc/MemoryPool.h"
#include "dabc/Url.h"
#include "dabc/DataTransport.h"

#include "mbs/LmdInput.h"
#include "mbs/LmdOutput.h"
#include "mbs/TextInput.h"
#include "mbs/GeneratorInput.h"
#include "mbs/ServerTransport.h"
#include "mbs/ClientTransport.h"
#include "mbs/CombinerModule.h"
#include "mbs/Monitor.h"
#include "mbs/api.h"

dabc::FactoryPlugin mbsfactory(new mbs::Factory("mbs"));


dabc::Reference mbs::Factory::CreateObject(const std::string& classname, const std::string& objname, dabc::Command cmd)
{
   if (classname=="mbs_iter")
      return new mbs::EventsIterator(objname);

   return dabc::Factory::CreateObject(classname, objname, cmd);
}

dabc::Module* mbs::Factory::CreateTransport(const dabc::Reference& port, const std::string& typ, dabc::Command cmd)
{
   dabc::Url url(typ);

   dabc::PortRef portref = port;

/*   if (portref.IsInput() && (url.GetProtocol()==mbs::protocolMbs) && !url.GetHostName().empty()) {

      dabc::DataInput* addon = CreateDataInput(typ);
      if (addon==0) return 0;

      dabc::InputTransport* tr = new dabc::InputTransport(cmd, portref, addon, true);

      tr->EnableReconnect(typ);

      return tr;
   }
*/

   if (portref.IsOutput() && (url.GetProtocol()==mbs::protocolMbs) && !url.GetHostName().empty()) {

      int kind = mbs::StreamServer;
      int portnum = 0;

      if (url.GetPort()>0) {
         portnum = url.GetPort();
         if (portnum==DefualtServerPort(mbs::TransportServer))
            kind = mbs::TransportServer;
      }

      if (!url.GetHostName().empty())
         kind = StrToServerKind(url.GetHostName().c_str());

      if (portnum==0) portnum = DefualtServerPort(kind);

      int fd = dabc::SocketThread::StartServer(portnum);

      if (fd<=0) {
         DOUT3("Fail assign MBS server to port:%d", portnum);
         return 0;
      }

      dabc::SocketServerAddon* addon = new dabc::SocketServerAddon(fd, portnum);

      return new mbs::ServerTransport(cmd, portref, kind, portnum, addon, url);
   }

   return dabc::Factory::CreateTransport(port, typ, cmd);
}


dabc::DataInput* mbs::Factory::CreateDataInput(const std::string& typ)
{
   DOUT2("Factory::CreateDataInput %s", typ.c_str());

   dabc::Url url(typ);
   if ((url.GetProtocol()==mbs::protocolLmd) && (url.GetFullName() == "Generator")) {
      DOUT0("Create LMD Generator input");
      return new mbs::GeneratorInput(url);
   } else
   if (url.GetProtocol()==mbs::protocolLmd) {
      DOUT0("LMD2 input file name %s", url.GetFullName().c_str());
      return new mbs::LmdInput(url);
   } else
   if (url.GetProtocol()=="lmdtxt") {
      DOUT0("TEXT LMD input file name %s", url.GetFullName().c_str());
      return new mbs::TextInput(url);
   } else
   if (((url.GetProtocol()==mbs::protocolMbs) && !url.GetHostName().empty()) ||
       (url.GetProtocol()=="mbss") || (url.GetProtocol()=="mbst")) {
      DOUT3("Try to create new MBS data input typ %s", typ.c_str());

      int kind = mbs::NoServer;
      if (url.GetProtocol()=="mbss") kind = mbs::StreamServer; else
      if (url.GetProtocol()=="mbst") kind = mbs::TransportServer;

      int portnum = 0;

      if ((kind == mbs::NoServer) && !url.GetFileName().empty())
         kind = StrToServerKind(url.GetFileName().c_str());

      if (url.GetPort()>0)
         portnum = url.GetPort();

      if (kind == mbs::NoServer) {
         if (portnum==DefualtServerPort(mbs::TransportServer)) kind = mbs::TransportServer; else
         if (portnum==DefualtServerPort(mbs::StreamServer)) kind = mbs::StreamServer;
      }

      if (portnum==0) portnum = DefualtServerPort(kind);

      if ((kind == mbs::NoServer) || (portnum==0)) {
         EOUT("MBS server in url %s not specified correctly", typ.c_str());
         return 0;
      }

      int fd = dabc::SocketThread::StartClient(url.GetHostName().c_str(), portnum);

      if (fd<=0) {
         DOUT3("Fail connecting to host:%s port:%d", url.GetHostName().c_str(), portnum);
         return 0;
      }

      DOUT0("Connect MBS %s server %s:%d", mbs::ServerKindToStr(kind), url.GetHostName().c_str(),  portnum);

      return new mbs::ClientTransport(fd, kind);
   }


   return 0;
}

dabc::DataOutput* mbs::Factory::CreateDataOutput(const std::string& typ)
{
   DOUT2("Factory::CreateDataOutput typ:%s", typ.c_str());

   dabc::Url url(typ);
   if (url.GetProtocol()==mbs::protocolLmd) {
      DOUT0("LMD output file name %s", url.GetFullName().c_str());
      return new mbs::LmdOutput(url);
   }

   return 0;
}

dabc::Module* mbs::Factory::CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd)
{
   if (classname == "mbs::CombinerModule")
      return new mbs::CombinerModule(modulename, cmd);

   if (classname == "mbs::Monitor")
      return new mbs::Monitor(modulename, cmd);

   if (classname == "mbs::ReadoutModule")
      return new mbs::ReadoutModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}

