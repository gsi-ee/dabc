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
#include "dabc/collections.h"

#include <stdlib.h>
#include <string.h>

#include "dabc/threads.h"
#include "dabc/Command.h"
#include "dabc/Buffer.h"

// ________________________________________________________________

void dabc::BuffersQueue::Cleanup(Mutex* mutex)
{
   dabc::Buffer* buf = 0;
   
   do {
      dabc::Buffer::Release(buf);
      
      dabc::LockGuard lock(mutex);
      
      buf = Size()>0 ? Pop() : 0;
       
   } while (buf!=0); 
}

// ____________________________________________________________________


dabc::CommandsQueue::CommandsQueue(bool replyqueue, bool withmutex) :
   fQueue(16, true),
   fKind(replyqueue ? kindReply : kindSubmit),
   fCmdsMutex(0),
   fIdCounter(0)
{
   if (withmutex) fCmdsMutex = new Mutex;
}

dabc::CommandsQueue::CommandsQueue(EKind kind) :
   fQueue(16, true),
   fKind(kind),
   fCmdsMutex(0),
   fIdCounter(0)
{

}


dabc::CommandsQueue::~CommandsQueue()
{
   Cleanup();

   if (fCmdsMutex) {   
      delete fCmdsMutex; 
      fCmdsMutex = 0;
   }
}

void dabc::CommandsQueue::Cleanup()
{
   Command* cmd = 0;
   
   while ((cmd = Pop()) != 0) {
      DOUT5(("CommandsQueue::Cleanup command %s", cmd->GetName())); 
      if (fKind == kindReply)
         dabc::Command::Finalise(cmd);  
      else   
         dabc::Command::Reply(cmd, false);
   }
}
         
uint32_t dabc::CommandsQueue::Push(Command* cmd)
{ 
   LockGuard guard(fCmdsMutex);
   fQueue.Push(cmd);

   if ((fKind == kindReply) || (fKind == kindSubmit)) {
      do { fIdCounter++; } while (fIdCounter==0);
      cmd->fCmdId = fIdCounter;
      return fIdCounter;
   }
   return 0;
}
         
dabc::Command* dabc::CommandsQueue::Pop() 
{
   LockGuard guard(fCmdsMutex); 
   return fQueue.Size()>0 ? fQueue.Pop() : 0;
}

dabc::Command* dabc::CommandsQueue::PopWithId(uint32_t id)
{
   LockGuard guard(fCmdsMutex);

   if ((fKind == kindReply) || (fKind == kindSubmit))
      for (unsigned n=0;n<fQueue.Size();n++) {
         dabc::Command* cmd = fQueue.Item(n);
         if (cmd->fCmdId == id) {
            fQueue.RemoveItem(n);
            return cmd;
         }
      }

   return 0;

}

bool dabc::CommandsQueue::HasCommand(Command* cmd)
{
   if (cmd==0) return false;

   LockGuard guard(fCmdsMutex);

   for (unsigned n=0;n<fQueue.Size();n++)
      if (fQueue.Item(n) == cmd) return true;

   return false;
}

bool dabc::CommandsQueue::RemoveCommand(Command* cmd)
{
   if (cmd==0) return false;

   LockGuard guard(fCmdsMutex);

   for (unsigned n=0;n<fQueue.Size();n++)
      if (fQueue.Item(n) == cmd) {
         fQueue.RemoveItem(n);
         return true;
      }

   return false;
}



dabc::Command* dabc::CommandsQueue::Front()
{
   LockGuard guard(fCmdsMutex); 
   return fQueue.Size()>0 ? fQueue.Front() : 0; 
}

unsigned dabc::CommandsQueue::Size()
{
   LockGuard guard(fCmdsMutex);  
   return fQueue.Size();
}
