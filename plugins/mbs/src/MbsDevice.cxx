#include "mbs/MbsDevice.h"

#include "dabc/Manager.h"
#include "dabc/Command.h"
#include "dabc/Port.h"
#include "dabc/SocketThread.h"

#include "mbs/MbsTransport.h"


mbs::MbsDevice::MbsDevice(Basic* parent, const char* name) :
   dabc::Device(parent, name)
{
   DOUT5(("Start MbsDevice constructor")); 
   
   dabc::Manager::Instance()->MakeThreadFor(this, "MBSThread");

   DOUT5(("Did MbsDevice constructor")); 
}
         
int mbs::MbsDevice::CreateTransport(dabc::Command* cmd, dabc::Port* port)
{
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
   
   MbsServerTransport* tr = new MbsServerTransport(this, port, kind, servfd, portnum, maxbufsize);
   
   // connect to port immidiately, even before client(s) really connected
   // This will fill output queue of the transport, but buffers should not be lost
   // Connection/disconnection of client will be invisible for the module
    
   port->AssignTransport(tr);

   return true;
}
