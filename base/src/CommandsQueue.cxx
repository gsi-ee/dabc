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

      QueueRec rec;

      {
         LockGuard lock(m);

         if (fList.size()==0) return;

         rec = fList.front();
         fList.pop_front();
      }

      switch (rec.kind) {
         case kindNone:
            break;
         case kindPostponed:
         case kindSubmit:
            rec.cmd.Reply(res);
            break;
         case kindReply:
            // we should reply on the command without changing result
            rec.cmd.Reply();
            break;
         case kindAssign:
            rec.cmd.RemoveCaller(proc);
            break;
      }

   } while (true);
}


uint32_t dabc::CommandsQueue::Push(Command& cmd, EKind kind)
{
   if (kind == kindNone) kind = fKind;

   // exclude zero id
   do { fIdCounter++; } while (fIdCounter==0);

   fList.push_back(QueueRec(cmd, kind, fIdCounter));

   return fIdCounter;
}

uint32_t dabc::CommandsQueue::PushD(Command cmd)
{
   // exclude zero id
   do { fIdCounter++; } while (fIdCounter==0);

   fList.push_back(QueueRec(cmd, kindNone, fIdCounter));

   return fIdCounter;
}



dabc::Command dabc::CommandsQueue::Pop()
{
   if (fList.size()==0) return dabc::Command();

   QueueRec rec(fList.front());

   fList.pop_front();

   return rec.cmd;
}

dabc::Command dabc::CommandsQueue::PopWithKind(EKind kind)
{
   for (QueueRecsList::iterator iter = fList.begin(); iter != fList.end(); iter++) {
      if (iter->kind==kind) {
         dabc::Command cmd = iter->cmd;
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
         dabc::Command cmd = iter->cmd;
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
