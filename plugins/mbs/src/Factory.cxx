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
#include "mbs/ServerTransport.h"
#include "mbs/ClientTransport.h"
#include "mbs/GeneratorModule.h"
#include "mbs/CombinerModule.h"

dabc::FactoryPlugin mbsfactory(new mbs::Factory("mbs"));


dabc::Transport* mbs::Factory::CreateTransport(const dabc::Reference& port, const std::string& typ, dabc::Command cmd)
{
   dabc::Url url(typ);

   dabc::PortRef portref = port;

   if (portref.IsInput() && (url.GetProtocol()==mbs::protocolMbs) && !url.GetHostName().empty()) {

      DOUT0("Try to create new MBS input transport typ %s file %s", typ.c_str(), url.GetFileName().c_str());

      int kind = mbs::NoServer;
      int portnum = 0;

      if (!url.GetFileName().empty())
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

      dabc::TimeStamp tm(dabc::Now());
      bool firsttime = true;

      // TODO: configure timeout via url parameter
      // TODO: make connection via special addon (not blocking thread here)
      double tmout(3.);

      int fd(-1);

      do {

         if (firsttime) firsttime = false;
                   else dabc::mgr.Sleep(0.01);

         fd = dabc::SocketThread::StartClient(url.GetHostName().c_str(), portnum);
         if (fd>0) break;

      } while (!tm.Expired(tmout));

      if (fd<=0) {
         DOUT0("Fail connecting to host:%s port:%d", url.GetHostName().c_str(), portnum);
         return 0;
      }

      DOUT0("Try to establish connection with host:%s kind:%s port:%d", url.GetHostName().c_str(), mbs::ServerKindToStr(kind), portnum);

      ClientTransport* tr = new ClientTransport(fd, kind);

      return new dabc::InputTransport(cmd, portref, tr, false, tr);
   }


   if (portref.IsOutput() && (url.GetProtocol()==mbs::protocolMbs) && !url.GetHostName().empty()) {

      DOUT0("Try to create new MBS output transport type %s", typ.c_str());

      int kind = mbs::StreamServer;
      int portnum = 0;
      int connlimit = 0;

      if (url.GetPort()>0) {
         portnum = url.GetPort();
         if (portnum==DefualtServerPort(mbs::TransportServer))
            kind = mbs::TransportServer;
      }

      if (!url.GetHostName().empty())
         kind = StrToServerKind(url.GetHostName().c_str());

      if (url.HasOption("limit"))
         connlimit = url.GetOptionInt("limit", connlimit);

      if (portnum==0) portnum = DefualtServerPort(kind);

      int fd = dabc::SocketThread::StartServer(portnum);

      if (fd<=0) {
         DOUT0("Fail assign MBS server to port:%d", portnum);
         return 0;
      }

      DOUT0("Create MBS server fd:%d kind:%s port:%d limit:%d", fd, mbs::ServerKindToStr(kind), portnum, connlimit);

      dabc::SocketServerAddon* addon = new dabc::SocketServerAddon(fd, portnum);

      return new mbs::ServerTransport(cmd, portref, kind, addon, connlimit);
   }


   return dabc::Factory::CreateTransport(port, typ, cmd);
}


dabc::DataInput* mbs::Factory::CreateDataInput(const std::string& typ)
{
   DOUT2("Factory::CreateDataInput %s", typ.c_str());

   dabc::Url url(typ);
   if (url.GetProtocol()==mbs::protocolLmd) {
      DOUT0("LMD input file name %s", url.GetFullName().c_str());

      return new mbs::LmdInput(url);
   } else
   if (url.GetProtocol()=="lmdtxt") {
      DOUT0("TEXT LMD input file name %s", url.GetFullName().c_str());
      return new mbs::TextInput(url);
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
   if (classname == "mbs::GeneratorModule")
      return new mbs::GeneratorModule(modulename, cmd);

   if (classname == "mbs::CombinerModule")
      return new mbs::CombinerModule(modulename, cmd);

   if (classname == "mbs::ReadoutModule")
      return new mbs::ReadoutModule(modulename, cmd);

   if (classname == "mbs::TransmitterModule")
      return new mbs::TransmitterModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}

