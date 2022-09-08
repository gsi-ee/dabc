// $Id$

#include "dabc/CommandsQueue.h"

#include "dabc/Worker.h"

dabc::CommandsQueue::CommandsQueue(EKind kind) :
   fList(),
   fKind(kind),
   fIdCounter(0)
{
}

dabc::CommandsQueue::~CommandsQueue()
{
   Cleanup();
}

void dabc::CommandsQueue::Cleanup(Mutex* m, Worker* proc, int res)
{
   do {

      Command cmd;
      int kind;

      {
         LockGuard lock(m);

         if (fList.size()==0) return;

         cmd << fList.front().cmd;
         kind = fList.front().kind;
         fList.pop_front();
      }

      switch (kind) {
         case kindNone:
            break;
         case kindPostponed:
         case kindSubmit:
            cmd.Reply(res);
            break;
         case kindReply:
            // we should reply on the command without changing result
            cmd.Reply();
            break;
         case kindAssign:
            cmd.RemoveCaller(proc);
            break;
      }
      cmd.Release();
   } while (true);
}


void dabc::CommandsQueue::ReplyTimedout()
{
   QueueRecsList::iterator iter = fList.begin();
   while (iter != fList.end()) {

      if (!iter->cmd.IsTimedout()) { iter++; continue; }

      iter->cmd.Reply(dabc::cmd_timedout);

      QueueRecsList::iterator curr = iter++;
      fList.erase(curr);
   }
}


uint32_t dabc::CommandsQueue::Push(Command cmd, EKind kind)
{
   if (kind == kindNone) kind = fKind;

   // exclude zero id
   do { fIdCounter++; } while (fIdCounter==0);

   fList.emplace_back(QueueRec());

   fList.back().cmd << cmd;
   fList.back().kind = kind;
   fList.back().id = fIdCounter;

   return fIdCounter;
}

dabc::Command dabc::CommandsQueue::Pop()
{
   if (fList.size()==0) return dabc::Command();

   dabc::Command cmd;

   cmd << fList.front().cmd;

   fList.pop_front();

   return cmd;
}

dabc::Command dabc::CommandsQueue::PopWithKind(EKind kind)
{
   for (QueueRecsList::iterator iter = fList.begin(); iter != fList.end(); iter++) {
      if (iter->kind==kind) {
         dabc::Command cmd;
         cmd << iter->cmd;
         fList.erase(iter);
         return cmd;
      }
   }

   return dabc::Command();
}

dabc::Command dabc::CommandsQueue::PopWithId(uint32_t id)
{
   for (QueueRecsList::iterator iter = fList.begin(); iter != fList.end(); iter++) {
      if (iter->id==id) {
         dabc::Command cmd;
         cmd << iter->cmd;
         fList.erase(iter);
         return cmd;
      }
   }

   return dabc::Command();
}

dabc::Command dabc::CommandsQueue::Front()
{
   return fList.size()>0 ? fList.front().cmd : dabc::Command();
}

dabc::CommandsQueue::EKind dabc::CommandsQueue::FrontKind()
{
   return fList.size()>0 ? fList.front().kind : kindNone;
}

uint32_t dabc::CommandsQueue::ChangeKind(Command& cmd, EKind kind)
{
   for (QueueRecsList::iterator iter = fList.begin(); iter != fList.end(); iter++) {
      if (iter->cmd==cmd) {
         iter->kind = kind;
         return iter->id;
      }
   }

   return Push(cmd, kind);
}

void dabc::CommandsQueue::ReplyAll(int res)
{
   Cleanup(0, 0, res);
}
