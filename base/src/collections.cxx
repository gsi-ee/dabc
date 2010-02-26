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
   fReplyQueue(replyqueue),
   fCmdsMutex(0)
{
   if (withmutex) fCmdsMutex = new Mutex;
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
      if (fReplyQueue)
         dabc::Command::Finalise(cmd);  
      else   
         dabc::Command::Reply(cmd, false);
   }
}
         
void dabc::CommandsQueue::Push(Command* cmd) 
{ 
   LockGuard guard(fCmdsMutex); 
   fQueue.Push(cmd); 
}
         
dabc::Command* dabc::CommandsQueue::Pop() 
{
   LockGuard guard(fCmdsMutex); 
   return fQueue.Size()>0 ? fQueue.Pop() : 0;
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
