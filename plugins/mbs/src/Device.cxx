#include "mbs/Device.h"

#include "dabc/Manager.h"
#include "dabc/Command.h"
#include "dabc/Port.h"
#include "dabc/SocketThread.h"

#include "mbs/ServerTransport.h"
#include "mbs/ClientTransport.h"


mbs::Device::Device(Basic* parent, const char* name) :
   dabc::Device(parent, name)
{
   DOUT5(("Start mbs::Device constructor"));

   dabc::Manager::Instance()->MakeThreadFor(this, "MBSThread");

   DOUT5(("Did mbs::Device constructor"));
}

int mbs::Device::CreateTransport(dabc::Command* cmd, dabc::Port* port)
{
   std::string kindstr = port->GetCfgStr(xmlServerKind, ServerKindToStr(mbs::TransportServer), cmd);

   int kind = StrToServerKind(kindstr.c_str());
   if ( kind == mbs::NoServer) {
      EOUT(("Wrong configured server type %s, use transport", kindstr.c_str()));
      kind = mbs::TransportServer;
   }

   int portnum = port->GetCfgInt(xmlServerPort, DefualtServerPort(kind), cmd);

   if (cmd->GetBool(xmlIsClient, false)) {

      std::string hostname = port->GetCfgStr(xmlServerName, "localhost", cmd);

      int fd = dabc::SocketThread::StartClient(hostname.c_str(), portnum);

      if (fd<=0) return false;

      DOUT1(("Creating client kind = %s  host %s port %d", kindstr.c_str(), hostname.c_str(), portnum));

      ClientTransport* tr = new ClientTransport(this, port, kind);

      tr->SetSocket(fd);

      port->AssignTransport(tr);

      return true;
   }

   uint32_t maxbufsize = port->GetCfgInt(dabc::xmlBufferSize, 16*1024, cmd);

   int servfd = dabc::SocketThread::StartServer(portnum);

   if (servfd<0) return cmd_false;

   DOUT1(("!!!!! Starts MBS server kind:%s on port %d", kindstr.c_str(), portnum));

   ServerTransport* tr = new ServerTransport(this, port, kind, servfd, portnum, maxbufsize);

   // connect to port immediately, even before client(s) really connected
   // This will fill output queue of the transport, but buffers should not be lost
   // Connection/disconnection of client will be invisible for the module

   port->AssignTransport(tr);

   return cmd_true;
}
