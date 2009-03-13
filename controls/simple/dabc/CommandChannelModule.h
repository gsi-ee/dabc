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
#ifndef DABC_CommandChannelModule
#define DABC_CommandChannelModule

#include "dabc/ModuleAsync.h"

#include "dabc/Command.h"

namespace dabc {

   class MemoryPool;
   class CommandsQueue;

   class CommandChannelModule : public ModuleAsync {
      protected:
         bool           fIsMaster;
         CommandsQueue* fCmdOutQueue;
      public:
         CommandChannelModule(int numnodes);
         virtual ~CommandChannelModule();

         static const char* CmdPoolName() { return "CommandChannelPool"; }
         static int CmdBufSize() { return 16*1024 - 16; }

         virtual WorkingProcessor* GetCfgMaster() { return 0; }

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
