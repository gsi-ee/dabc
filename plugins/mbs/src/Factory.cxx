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
#include "dabc/Port.h"
#include "dabc/Url.h"

#include "mbs/LmdInput.h"
#include "mbs/LmdOutput.h"
#include "mbs/TextInput.h"
#include "mbs/ServerTransport.h"
#include "mbs/ClientTransport.h"
#include "mbs/GeneratorModule.h"
#include "mbs/CombinerModule.h"

#ifdef MBSEVAPI
#include "mbs/EventAPI.h"
#endif

dabc::FactoryPlugin mbsfactory(new mbs::Factory("mbs"));

dabc::Transport* mbs::Factory::CreateTransport(dabc::Reference ref, const char* typ, dabc::Command cmd)
{
   dabc::PortRef portref = ref;
   if (portref.null()) {
      EOUT(("Port not specified"));
      return 0;
   }

   uint32_t maxbufsize(1024);
   int scale(1), kind(0), portnum(0), maxclients(0);
   std::string kindstr, hostname;
   bool isserver(true);
   double tmout(1.);
   dabc::Url kindurl;

   if (strcmp(typ, mbs::typeServerTransport)==0)
      isserver = true;
   else
   if (strcmp(typ, mbs::typeClientTransport)==0)
      isserver = false;
   else
      return dabc::Factory::CreateTransport(portref, typ, cmd);

   kindstr = portref.Cfg(xmlServerKind, cmd).AsStdStr();
   kindurl.SetUrl(kindstr, false);

   kind = mbs::StreamServer;

   if (!kindstr.empty()) {

      DOUT2(("KIND STR = %s", kindstr.c_str()));

      kind = StrToServerKind(kindurl.GetFullName().c_str());

      if (kind == mbs::NoServer) {
         EOUT(("Wrong configured server type %s, use stream server", kindstr.c_str()));
         kind = mbs::StreamServer;
      }

      portnum = kindurl.GetPort();
      if (portnum>0)
         DOUT0(("MBS port %d", portnum));
   }

   tmout = portref.Cfg(dabc::xmlConnTimeout, cmd).AsDouble(0.);

   if (isserver) {
      maxbufsize = portref.Cfg(dabc::xmlBufferSize, cmd).AsUInt(0);

      if (maxbufsize==0) {
         dabc::MemoryPoolRef pool = portref.GetPool();
         maxbufsize = pool.GetMaxBufSize();
      }

      if (maxbufsize==0) maxbufsize = 65536;
      if (maxbufsize<32) maxbufsize = 32;

      scale = portref.Cfg(xmlServerScale, cmd).AsInt(1);
      if (scale<1) scale = 1;

      maxclients = portref.Cfg(xmlServerLimit, cmd).AsInt(0);
      if (maxclients<0) maxclients = 0;

   } else {
      hostname = portref.Cfg(xmlServerName, cmd).AsStdStr("localhost");
      DOUT1(("Creating mbs client for host:%s port:%d tmout:%3.1f", hostname.c_str(), portnum, tmout));

      dabc::Url hosturl;
      hosturl.SetUrl(hostname, false);
      if (hosturl.GetPort() > 0) {
         portnum = hosturl.GetPort();
         hostname = hosturl.GetFullName();
         DOUT0(("Use custom port %d for client connection to %s host", portnum, hostname.c_str()));
      }
   }

   if (portnum==0)
      portnum = portref.Cfg(xmlServerPort, cmd).AsInt(DefualtServerPort(kind));

   dabc::TimeStamp tm(dabc::Now());
   bool firsttime = true;

   do {

      if (firsttime) firsttime = false;
                else dabc::mgr.Sleep(0.01);

      if (isserver) {
         int servfd = dabc::SocketThread::StartServer(portnum);

         if (servfd>0) {

            DOUT1(("!!!!! Starts MBS server kind:%s on port %d maxbufsize %u scale %d maxclients %d", mbs::ServerKindToStr(kind), portnum, maxbufsize, scale, maxclients));

            // connect to port immediately, even before client(s) really connected
            // This will fill output queue of the transport, but buffers should not be lost
            // Connection/disconnection of client will be invisible for the module
            return new ServerTransport(portref, kind, servfd, portnum, maxbufsize, scale, maxclients);
         }

      } else {

         int fd = dabc::SocketThread::StartClient(hostname.c_str(), portnum);

         if (fd>0) {

            DOUT1(("Creating client kind = %s  host %s port %d", mbs::ServerKindToStr(kind), hostname.c_str(), portnum));

            return new ClientTransport(portref, kind, fd);
         }
      }

   } while (!tm.Expired(tmout));

   return 0;
}

dabc::DataInput* mbs::Factory::CreateDataInput(const char* typ)
{
   if ((typ==0) || (strlen(typ)==0)) return 0;

   DOUT3(("Factory::CreateDataInput %s", typ));

   if (strcmp(typ, mbs::typeLmdInput)==0) {
      return new mbs::LmdInput();
   } else
   if (strcmp(typ, mbs::typeTextInput)==0) {
      return new mbs::TextInput();
   }
#ifdef MBSEVAPI

   else
   if (strcmp(typ, xmlEvapiFile) == 0) {
      return new mbs::EvapiInput(xmlEvapiFile);
   } else
   if (strcmp(typ, xmlEvapiRFIOFile) == 0) {
      return new mbs::EvapiInput(xmlEvapiRFIOFile);
   } else
   if (strcmp(typ, xmlEvapiTransportServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiTransportServer);
   } else
   if (strcmp(typ, xmlEvapiStreamServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiStreamServer);
   } else
   if (strcmp(typ, xmlEvapiEventServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiEventServer);
   } else
   if (strcmp(typ, xmlEvapiRemoteEventServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiRemoteEventServer);
   }
#endif

   return 0;
}

dabc::DataOutput* mbs::Factory::CreateDataOutput(const char* typ)
{
   if ((typ==0) || (strlen(typ)==0)) return 0;

   DOUT3(("Factory::CreateDataOutput typ:%s", typ));

   if (strcmp(typ, typeLmdOutput)==0) {
      return new mbs::LmdOutput();
   }
#ifdef MBSEVAPI
   else
   if (strcmp(typ, xmlEvapiOutFile) == 0) {
      return new mbs::EvapiOutput();
   }
#endif

   return 0;
}

dabc::Module* mbs::Factory::CreateModule(const char* classname, const char* modulename, dabc::Command cmd)
{

   if (strcmp(classname, "mbs::GeneratorModule")==0)
      return new mbs::GeneratorModule(modulename, cmd);
   else
   if (strcmp(classname, "mbs::CombinerModule")==0)
      return new mbs::CombinerModule(modulename, cmd);
   else
   if (strcmp(classname, "mbs::ReadoutModule")==0)
      return new mbs::ReadoutModule(modulename, cmd);
   else
   if (strcmp(classname, "mbs::TransmitterModule")==0)
      return new mbs::TransmitterModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}


// FIXME: implement CreateTransport for all classes, based on URL

/*

dabc::Transport* mbs::Factory::CreateTransportNew(dabc::Port* port, dabc::Command cmd)
{

   bool isinput = port->InputQueueCapacity() > 0;
   bool isoutput = (port->InputQueueCapacity() > 0) && !isinput;

   std::string proto = port->Cfg(dabc::xmlProtocol, cmd).AsStdStr();

   int kind = mbs::NoServer;

   int portnum = 0;

   if (proto.compare(mbs::protocolMbsTransport) == 0) kind = mbs::TransportServer; else
   if (proto.compare(mbs::protocolMbsStream) == 0) kind = mbs::StreamServer; else
   if (proto.compare(mbs::protocolMbs) == 0) {

      kind = StrToServerKind(port->Cfg("kind", cmd).AsStr());

      if (kind==mbs::NoServer) {
         portnum = port->Cfg(dabc::xmlUrlPort,cmd).AsInt();

         kind = mbs::TransportServer;
         if (port->Cfg(dabc::xmlUrlPort,cmd).AsInt() == DefualtServerPort(mbs::StreamServer))
            kind = mbs::StreamServer;
      }
   }

   if (kind == mbs::NoServer) return 0;

   if (portnum<=0) portnum = port->Cfg(dabc::xmlUrlPort,cmd).AsInt();

   if (portnum<=0) portnum = DefualtServerPort(kind);

   std::string thrdname = port->Cfg(dabc::xmlTrThread, cmd).AsStdStr();
   if (thrdname.length()==0) thrdname = port->ThreadName();

   if (isinput) {

      std::string hostname = port->Cfg(dabc::xmlHostName, cmd).AsStdStr("localhost");

      DOUT1(("Creating mbs client for host:%s port:%d", hostname.c_str(), portnum));

      int fd = dabc::SocketThread::StartClient(hostname.c_str(), portnum);

      if (fd<=0) return 0;

      DOUT1(("Creating client kind = %s  host %s port %d", ServerKindToStr(kind), hostname.c_str(), portnum));

      return new mbs::ClientTransport(port, kind, fd, thrdname);
   }

   int servfd = dabc::SocketThread::StartServer(portnum);
   if (servfd<0) return 0;

   uint32_t maxbufsize = port->Cfg(dabc::xmlBufferSize, cmd).AsInt(16*1024);
   int scale = port->Cfg(xmlServerScale, cmd).AsInt(1);

   DOUT1(("!!!!! Starts MBS server kind:%s on port %d maxbufsize %u scale %d", ServerKindToStr(kind), portnum, maxbufsize, scale));

   // connect to port immediately, even before client(s) really connected
   // This will fill output queue of the transport, but buffers should not be lost
   // Connection/disconnection of client will be invisible for the module

   return new mbs::ServerTransport(port, thrdname, kind, servfd, portnum, maxbufsize, scale);
}

*/
