#include "mbs/Factory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/SocketThread.h"
#include "dabc/Port.h"

#include "mbs/EventAPI.h"
#include "mbs/LmdInput.h"
#include "mbs/LmdOutput.h"
#include "mbs/ServerTransport.h"
#include "mbs/ClientTransport.h"

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
      return 0;

   std::string kindstr = port->GetCfgStr(xmlServerKind, ServerKindToStr(mbs::TransportServer), cmd);

   int kind = StrToServerKind(kindstr.c_str());
   if (kind == mbs::NoServer) {
      EOUT(("Wrong configured server type %s, use transport", kindstr.c_str()));
      kind = mbs::TransportServer;
   }

   int portnum = port->GetCfgInt(xmlServerPort, DefualtServerPort(kind), cmd);

   dabc::Device* dev = (dabc::Device*) dabc::mgr()->FindLocalDevice();
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

      int fd = dabc::SocketThread::StartClient(hostname.c_str(), portnum);

      if (fd<=0) return 0;

      DOUT1(("Creating client kind = %s  host %s port %d thrd %s", kindstr.c_str(), hostname.c_str(), portnum, newthrdname.c_str()));

      ClientTransport* tr = new ClientTransport(dev, port, kind, fd, newthrdname);

      return tr;
   }

   uint32_t maxbufsize = port->GetCfgInt(dabc::xmlBufferSize, 16*1024, cmd);

   int servfd = dabc::SocketThread::StartServer(portnum);

   if (servfd<0) return 0;

   DOUT1(("!!!!! Starts MBS server kind:%s on port %d maxbufsize %u", kindstr.c_str(), portnum, maxbufsize));

   ServerTransport* tr = new ServerTransport(dev, port, kind, servfd, newthrdname, portnum, maxbufsize);

   // connect to port immediately, even before client(s) really connected
   // This will fill output queue of the transport, but buffers should not be lost
   // Connection/disconnection of client will be invisible for the module

   return tr;
}


dabc::DataInput* mbs::Factory::CreateDataInput(const char* typ, const char* name, dabc::Command* cmd)
{
   if ((typ==0) || (strlen(typ)==0)) return 0;

   DOUT3(("Factory::CreateDataInput %s", typ));

   if (strcmp(typ, typeLmdInput)==0) {
      return new mbs::LmdInput();
   } else
   if (strcmp(typ, xmlEvapiType) == 0) {
      return new mbs::EvapiInput(xmlEvapiFile, name);
   } else
   if (strcmp(typ, xmlEvapiFile) == 0) {
      return new mbs::EvapiInput(xmlEvapiFile, name);
   } else
   if (strcmp(typ, xmlEvapiRFIOFile) == 0) {
      return new mbs::EvapiInput(xmlEvapiRFIOFile, name);
   } else
   if (strcmp(typ, xmlEvapiTransportServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiTransportServer, name);
   } else
   if (strcmp(typ, xmlEvapiStreamServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiStreamServer, name);
   } else
   if (strcmp(typ, xmlEvapiEventServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiEventServer, name);
   } else
   if (strcmp(typ, xmlEvapiRemoteEventServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiRemoteEventServer, name);
   }

   return 0;
}

dabc::DataOutput* mbs::Factory::CreateDataOutput(const char* typ, const char* name, dabc::Command* cmd)
{

   if ((typ==0) || (strlen(typ)==0)) return 0;

   DOUT3(("Factory::CreateDataOutput typ:%s", typ));

   if (strcmp(typ, typeLmdOutput)==0) {
      return new mbs::LmdOutput();
   } else
   if (strcmp(typ, xmlEvapiOutFile) == 0) {
      return new mbs::EvapiOutput(name);
   }

   return 0;
}
