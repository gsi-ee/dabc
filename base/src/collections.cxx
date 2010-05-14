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

dabc::CommandsQueue::CommandsQueue(EKind kind) :
   fQueue(16, true),
   fKind(kind),
   fIdCounter(0)
{
}

dabc::CommandsQueue::CommandsQueue(bool isreply, bool) :
  fQueue(16, true),
  fKind(isreply ? kindReply : kindSubmit),
  fIdCounter(0)
{
}



dabc::CommandsQueue::~CommandsQueue()
{
   Cleanup();
}

void dabc::CommandsQueue::Cleanup(Mutex* m, WorkingProcessor* proc)
{
   Command* cmd = 0;
   

   while ((cmd = Pop(m)) != 0) {

      switch (fKind) {
         case kindNone:
            break;
         case kindSubmit:
            dabc::Command::Reply(cmd, false);
            break;
         case kindReply:
            dabc::Command::Finalise(cmd);
            break;
         case kindAssign:
            if (proc) cmd->RemoveCaller(proc);
            break;
      }
   }
}
         
uint32_t dabc::CommandsQueue::Push(Command* cmd)
{ 
   fQueue.Push(cmd);

   if ((fKind != kindReply) && (fKind != kindSubmit)) return 0;

   do { fIdCounter++; } while (fIdCounter==0);
   cmd->fCmdId = fIdCounter;
   return fIdCounter;
}
         
dabc::Command* dabc::CommandsQueue::Pop(Mutex* m)
{
   LockGuard lock(m);
   return Pop();
}

dabc::Command* dabc::CommandsQueue::Pop() 
{
   return fQueue.Size()>0 ? fQueue.Pop() : 0;
}

dabc::Command* dabc::CommandsQueue::PopWithId(uint32_t id)
{
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

   for (unsigned n=0;n<fQueue.Size();n++)
      if (fQueue.Item(n) == cmd) return true;

   return false;
}

bool dabc::CommandsQueue::RemoveCommand(Command* cmd)
{
   if ((cmd==0) || (fQueue.Size()==0)) return false;

   for (unsigned n=fQueue.Size();n>0;n--)
      if (fQueue.Item(n-1) == cmd) {
         fQueue.RemoveItem(n-1);
         return true;
      }

   return false;
}

dabc::Command* dabc::CommandsQueue::Front()
{
   return fQueue.Size()>0 ? fQueue.Front() : 0; 
}

unsigned dabc::CommandsQueue::Size()
{
   return fQueue.Size();
}

void dabc::CommandsQueue::ReplyAll(int res)
{
   Command* cmd = 0;
   while ((cmd = Pop()) != 0)
     dabc::Command::Reply(cmd, res);
}

