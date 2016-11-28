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
   std::string prot = url.GetProtocol(), host = url.GetHostName();
   int kind = 0, portnum = url.GetPort();
   if (prot == mbs::protocolMbss) kind = mbs::StreamServer; else
   if (prot == mbs::protocolMbst) kind = mbs::TransportServer; else
   if (prot == mbs::protocolMbs) {
      kind = mbs::StreamServer;
      if (portnum==DefualtServerPort(mbs::TransportServer))
         kind = mbs::TransportServer;
      if (StrToServerKind(host) != NoServer) {
         kind = StrToServerKind(host);
         host = "";
      }
   }

   if (portref.IsOutput() && (kind>0)) {

      if (portnum==0) portnum = DefualtServerPort(kind);

      dabc::SocketServerAddon* addon = dabc::SocketThread::CreateServerAddon(host, portnum);

      if (addon==0) {
         DOUT3("Fail assign MBS server to port:%d", portnum);
         return 0;
      }

      return new mbs::ServerTransport(cmd, portref, kind, portnum, addon, url);
   }

   return dabc::Factory::CreateTransport(port, typ, cmd);
}


dabc::DataInput* mbs::Factory::CreateDataInput(const std::string& typ)
{
   dabc::Url url(typ);
   if ((url.GetProtocol()==mbs::protocolLmd) && (url.GetFullName() == "Generator")) {
      return new mbs::GeneratorInput(url);
   } else
   if (url.GetProtocol()==mbs::protocolLmd) {
      return new mbs::LmdInput(url);
   } else
   if (url.GetProtocol()=="lmdtxt") {
      return new mbs::TextInput(url);
   } else
   if (((url.GetProtocol() == mbs::protocolMbs) && !url.GetHostName().empty()) ||
        (url.GetProtocol() == mbs::protocolMbss) || (url.GetProtocol() == mbs::protocolMbst)) {
      DOUT3("Try to create new MBS data input typ %s", typ.c_str());

      int kind = mbs::NoServer;
      if (url.GetProtocol()==mbs::protocolMbss) kind = mbs::StreamServer; else
      if (url.GetProtocol()==mbs::protocolMbst) kind = mbs::TransportServer;

      int portnum = url.GetPort();

      if ((kind == mbs::NoServer) && !url.GetFileName().empty())
         kind = StrToServerKind(url.GetFileName());

      if (kind == mbs::NoServer) {
         if (portnum==DefualtServerPort(mbs::TransportServer)) kind = mbs::TransportServer; else
         if (portnum==DefualtServerPort(mbs::StreamServer)) kind = mbs::StreamServer;
      }

      if (portnum<=0) portnum = DefualtServerPort(kind);

      if ((kind == mbs::NoServer) || (portnum==0)) {
         EOUT("MBS server in url %s not specified correctly", typ.c_str());
         return 0;
      }

      int fd = dabc::SocketThread::StartClient(url.GetHostName(), portnum);

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

