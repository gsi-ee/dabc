// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#ifndef DABC_CommandsQueue
#define DABC_CommandsQueue

#ifndef DABC_Command
#include "dabc/Command.h"
#endif

#include <list>

#include <stdint.h>

namespace dabc {

   class Worker;

   class CommandsQueue {

      public:

         enum EKind {
            kindNone,
            kindSubmit,    //!< command submitted for execution
            kindReply,     //!< command replied to the worker
            kindAssign,    //!< command assign with the worker, but not yet replied
            kindPostponed  //!< command submitted but postponed to not increase execution recursion
         };

      protected:

         struct QueueRec {
            Command     cmd;
            EKind       kind;
            uint32_t    id;

            QueueRec() : cmd(), kind(kindNone), id(0) {}
            QueueRec(const Command& _cmd, EKind _kind, uint32_t _id) : cmd(_cmd), kind(_kind), id(_id) {}
            QueueRec(const QueueRec& src) : cmd(src.cmd), kind(src.kind), id(src.id) {}
            QueueRec& operator=(const QueueRec& src)
            {
               cmd = src.cmd;
               kind = src.kind;
               id = src.id;
               return *this;
            }
         };

         typedef std::list<QueueRec> QueueRecsList;

         QueueRecsList     fList;
         EKind             fKind;
         uint32_t          fIdCounter;

      public:
         /** Normal constructor */
         CommandsQueue(EKind kind = kindNone);
         virtual ~CommandsQueue();

         /** Add reference on the command in the queue.
          * Kind of command can be specified, if not - default queue kind is used
          * Returns unique id for the command inside this queue */
         uint32_t Push(Command& cmd, EKind kind = kindNone);

         /** FIXME: is command should be supplied with reference ??? */
         uint32_t PushD(Command cmd);

         /** Change kind of the entry for specified command.
          * If entry not exists, command will be add to the queue.
          * Returns unique id */
         uint32_t ChangeKind(Command& cmd, EKind kind);

         Command  Pop();
         Command  PopWithId(uint32_t id);
         Command  PopWithKind(EKind kind);
         Command  Front();
         EKind    FrontKind();

         unsigned Size() const { return fList.size(); }

         void ReplyAll(int res);

         void Cleanup(Mutex* m = 0, Worker* proc = 0, int res = cmd_false);
   };

}

#endif
