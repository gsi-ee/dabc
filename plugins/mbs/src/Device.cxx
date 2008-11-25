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
   if (cmd->GetBool("IsClient", false)) {
      int kind = cmd->GetInt("ServerKind", -1);
      int portnum = cmd->GetInt("PortNum",-1);
      const char* hostname = cmd->GetStr("Host", "localhost");

      if (portnum<=0)
         if (kind == TransportServer) portnum = 6000; else
         if (kind == StreamServer) portnum = 6002;

      int fd = dabc::SocketThread::StartClient(hostname, portnum);

      if (fd<=0) return false;

      DOUT1(("Creating client kind = %d  host %s port %d", kind, hostname, portnum));

      ClientTransport* tr = new ClientTransport(this, port, kind);

      tr->SetSocket(fd);

      port->AssignTransport(tr);

      return true;
   }

   int portnum = cmd->GetInt("PortNum",-1);

   int kind = cmd->GetInt("ServerKind", -1);
   uint32_t maxbufsize = cmd->GetUInt("BufferSize", 16*1024);
   int portmin = cmd->GetInt("PortMin",-1);
   int portmax = cmd->GetInt("PortMax",-1);

   if (kind == TransportServer) {
      portmin = 6000;
      portmax = 6000;
   } else
   if (kind == StreamServer) {
      portmin = 6002;
      portmax = 6002;
   } else {
      kind = TransportServer;
   }

   int servfd = dabc::SocketThread::StartServer(portnum, portmin, portmax);

   if (servfd<0) return false;

   DOUT1(("!!!!! Starts MBS server on port %d", portnum));

   ServerTransport* tr = new ServerTransport(this, port, kind, servfd, portnum, maxbufsize);

   // connect to port immediately, even before client(s) really connected
   // This will fill output queue of the transport, but buffers should not be lost
   // Connection/disconnection of client will be invisible for the module

   port->AssignTransport(tr);

   return true;
}
