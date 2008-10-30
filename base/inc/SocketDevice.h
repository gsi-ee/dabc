#ifndef DABC_SocketDevice
#define DABC_SocketDevice

#ifndef DABC_GenericDevice
#include "dabc/Device.h"
#endif

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

namespace dabc {
    
   class NewConnectRec;
   class SocketProtocolProcessor;

   class SocketDevice : public Device {
       
      friend class SocketProtocolProcessor; 
      
      public:
         SocketDevice(Basic* parent, const char* name);
         virtual ~SocketDevice();
         
         virtual const char* RequiredThrdClass() const { return "SocketThread"; }
         
         virtual bool ServerConnect(Command* cmd, Port* port, const char* portname);
         virtual bool ClientConnect(Command* cmd, Port* port, const char* portname);
         virtual bool StartServerThread(Command* cmd, String& servid, const char* cmdchannel = 0);
         virtual bool SubmitRemoteCommand(const char* servid, const char* channelid, Command* cmd);

         virtual int CreateTransport(Command* cmd, Port* port);
         
         static void SetLocalHostIP(const char* ip);
         static const char* GetLocalHost();

      protected: 
      
         void NewClientRequest(int connfd);
         
         virtual double ProcessTimeout(double last_diff);
         
         virtual int ExecuteCommand(dabc::Command* cmd);
         
         bool CleanupRecs(double tmout);
         
         void AddRec(NewConnectRec* rec);
         
         void DestroyRec(NewConnectRec* rec, bool res);
         
         NewConnectRec* _FindRec(const char* connid);
         
         void ServerProtocolRequest(SocketProtocolProcessor* proc, const char* inmsg, char* outmsg);
         
         void ProtocolCompleted(SocketProtocolProcessor* proc, const char* inmsg);
         
         bool SubmitCommandFromRemote(SocketProtocolProcessor* proc, const char* cmd);

         bool RemoteCommandReplyed(SocketProtocolProcessor* proc, const char* cmd);
         
         void DestroyProcessor(SocketProtocolProcessor* proc, bool res);
         
         virtual const char* ClassName() const { return "SocketDevice"; }
         
         SocketServerProcessor* fServer;
         String                 fServerCmdChannel; // name (pass) of command channel
         PointersVector         fConnRecs; // list of connections recs
         PointersVector         fProtocols; // list of protocol start processors
         long                   fConnCounter;
         
         static char            fLocalHostIP[100];
   };
   
}

#endif
