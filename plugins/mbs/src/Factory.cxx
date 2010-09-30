/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "mbs/Factory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/SocketThread.h"
#include "dabc/Port.h"


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

mbs::Factory mbsfactory("mbs");

dabc::Transport* mbs::Factory::CreateTransport(dabc::Port* port, const char* typ, const char* thrdname, dabc::Command* cmd)
{
   bool isserver = true;

   if (strcmp(typ, mbs::typeServerTransport)==0)
      isserver = true;
   else
   if (strcmp(typ, mbs::typeClientTransport)==0)
      isserver = false;
   else
      return dabc::Factory::CreateTransport(port, typ, thrdname, cmd);

   std::string kindstr = port->GetCfgStr(xmlServerKind, ServerKindToStr(mbs::TransportServer), cmd);

   int kind = StrToServerKind(kindstr.c_str());
   if (kind == mbs::NoServer) {
      EOUT(("Wrong configured server type %s, use transport", kindstr.c_str()));
      kind = mbs::TransportServer;
   }

   int portnum = port->GetCfgInt(xmlServerPort, DefualtServerPort(kind), cmd);

   dabc::Device* dev = dabc::mgr()->FindLocalDevice();
   if (dev==0) {
      EOUT(("Local device not found"));
      return 0;
   }

   std::string newthrdname;
   if (thrdname==0)
      newthrdname = dabc::mgr()->MakeThreadName();
   else
      newthrdname = thrdname;


   if (!isserver) {

      std::string hostname = port->GetCfgStr(xmlServerName, "localhost", cmd);

      DOUT1(("Creating mbs client for host:%s port:%d", hostname.c_str(), portnum));

      int fd = dabc::SocketThread::StartClient(hostname.c_str(), portnum);

      if (fd<=0) return 0;

      DOUT1(("Creating client kind = %s  host %s port %d", kindstr.c_str(), hostname.c_str(), portnum));

      return new ClientTransport(port, kind, fd, newthrdname);
   }

   uint32_t maxbufsize = port->GetCfgInt(dabc::xmlBufferSize, 16*1024, cmd);
   int scale = port->GetCfgInt(xmlServerScale, 1, cmd);

   int servfd = dabc::SocketThread::StartServer(portnum);

   if (servfd<0) return 0;

   DOUT1(("!!!!! Starts MBS server kind:%s on port %d maxbufsize %u scale %d", kindstr.c_str(), portnum, maxbufsize, scale));

   // connect to port immediately, even before client(s) really connected
   // This will fill output queue of the transport, but buffers should not be lost
   // Connection/disconnection of client will be invisible for the module

   return new ServerTransport(port, newthrdname, kind, servfd, portnum, maxbufsize, scale);
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

dabc::Module* mbs::Factory::CreateModule(const char* classname, const char* modulename, dabc::Command* cmd)
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


dabc::Transport* mbs::Factory::CreateTransportNew(dabc::Port* port, dabc::Command* cmd)
{

   dabc::ConfigSource cfg(cmd, port);

   bool isinput = port->InputQueueCapacity() > 0;
   bool isoutput = (port->InputQueueCapacity() > 0) && !isinput;

   std::string proto = cfg.GetCfgStr(dabc::xmlProtocol);

   if (proto.compare(mbs::protocolLmd)==0) {

      return CreateIOTransport(port, cmd,
            isinput ? new mbs::LmdInput() : 0,
            isoutput ? new mbs::LmdOutput() : 0);
   }

   int kind = mbs::NoServer;

   int portnum = 0;

   if (proto.compare(mbs::protocolMbsTransport) == 0) kind = mbs::TransportServer; else
   if (proto.compare(mbs::protocolMbsStream) == 0) kind = mbs::StreamServer; else
   if (proto.compare(mbs::protocolMbs) == 0) {

      kind = StrToServerKind(cfg.GetCfgStr("kind").c_str());

      if (kind==mbs::NoServer) {
         portnum = cfg.GetCfgInt(dabc::xmlUrlPort);

         kind = mbs::TransportServer;
         if (cfg.GetCfgInt(dabc::xmlUrlPort) == DefualtServerPort(mbs::StreamServer))
            kind = mbs::StreamServer;
      }
   }

   if (kind == mbs::NoServer) return 0;

   if (portnum<=0) portnum = cfg.GetCfgInt(dabc::xmlUrlPort);

   if (portnum<=0) portnum = DefualtServerPort(kind);

   std::string thrdname = cfg.GetCfgStr(dabc::xmlTrThread);
   if (thrdname.length()==0) thrdname = port->ProcessorThreadName();

   if (isinput) {

      std::string hostname = cfg.GetCfgStr(dabc::xmlHostName, "localhost");

      DOUT1(("Creating mbs client for host:%s port:%d", hostname.c_str(), portnum));

      int fd = dabc::SocketThread::StartClient(hostname.c_str(), portnum);

      if (fd<=0) return 0;

      DOUT1(("Creating client kind = %s  host %s port %d", ServerKindToStr(kind), hostname.c_str(), portnum));

      return new mbs::ClientTransport(port, kind, fd, thrdname);
   }

   int servfd = dabc::SocketThread::StartServer(portnum);
   if (servfd<0) return 0;

   uint32_t maxbufsize = cfg.GetCfgInt(dabc::xmlBufferSize, 16*1024);
   int scale = cfg.GetCfgInt(xmlServerScale, 1);

   DOUT1(("!!!!! Starts MBS server kind:%s on port %d maxbufsize %u scale %d", ServerKindToStr(kind), portnum, maxbufsize, scale));

   // connect to port immediately, even before client(s) really connected
   // This will fill output queue of the transport, but buffers should not be lost
   // Connection/disconnection of client will be invisible for the module

   return new ServerTransport(port, thrdname, kind, servfd, portnum, maxbufsize, scale);
}

