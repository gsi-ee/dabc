#ifndef DABC_CommandChannelModule
#define DABC_CommandChannelModule

#include "dabc/ModuleAsync.h"

#include "dabc/Command.h"

namespace dabc {
    
   class MemoryPool; 
   class CommandsQueue;
   
   class CommandChannelModule : public ModuleAsync {
      protected:
         PoolHandle*  fPool;
         bool           fIsMaster;
         CommandsQueue* fCmdOutQueue;
      public:
         CommandChannelModule(Manager* mgr, int numnodes);
         virtual ~CommandChannelModule();
         
         MemoryPool* GetPool();
         
         virtual int ExecuteCommand(Command* cmd);
         virtual void ProcessInputEvent(Port* port);
         virtual void ProcessOutputEvent(Port* port);
         virtual void ProcessPoolEvent(PoolHandle* pool);
         virtual void ProcessConnectEvent(Port* port);
         virtual void ProcessDisconnectEvent(Port* port);

         
      protected:  
         void SendSubmittedCommands();
         
         void ConnDissPort(Port* port, bool on);
         
   };   
}

#endif
