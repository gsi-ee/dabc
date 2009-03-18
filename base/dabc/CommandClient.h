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
#ifndef DABC_CommandClient
#define DABC_CommandClient

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

#ifndef DABC_string
#include "dabc/string.h"
#endif

namespace dabc {

   class Command;
   class Mutex;
   class Condition;
   class Manager;

   class CommandClientBase {
      friend class Command;

      public:
         CommandClientBase();
         virtual ~CommandClientBase();

         Command* Assign(Command* cmd);

         int CancelCommands();

      protected:
         class stl_commands_list;

         Command* _Assign(Command* cmd);
         void _Forget(Command* cmd);
         bool _CommandReplyed(Command* cmd, bool res);

         virtual bool _ProcessReply(Command* cmd) = 0;
         unsigned _NumSubmCmds();

         Mutex                *fCmdsMutex;
         stl_commands_list    *fSubmCmds;
   };

   // ___________________________________________________________________

  /**
    * Base class for processing of commands.
    */


   class CommandReceiver {
      public:
         enum CommandRes {
            cmd_false = false,
            cmd_true = true,
            cmd_ignore = 3333,
            cmd_postponed = 7777
         };

      static int cmd_bool(bool res) { return res ? cmd_true : cmd_false; }

      protected:
         // this method is called when command should be executed
         int ProcessCommand(Command* cmd);
         virtual int PreviewCommand(Command* cmd) { return cmd_ignore; }
         virtual int ExecuteCommand(Command* cmd);

         virtual bool IsExecutionThread() { return true; }

         virtual ~CommandReceiver() {}

      public:
         virtual bool Submit(Command* cmd);

         bool Execute(Command* cmd, double timeout_sec = -1.);
         bool Execute(const char* cmdname, double timeout_sec = -1.);

         int ExecuteInt(const char* cmdname, const char* intresname, double timeout_sec = -1.);
         std::string ExecuteStr(const char* cmdname, const char* strresname, double timeout_sec = -1.);
   };

   // ___________________________________________________________________

   class CommandClient : public CommandClientBase {
      public:
         CommandClient(bool keep_cmds = false);
         virtual ~CommandClient();

         bool WaitCommands(double timeout_sec = -1.);

         bool IsKeepCommands() const { return fKeepCmds; }

         CommandsQueue& ReplyedCmds() { return fReplyedCmds; }

         void Reset();

      protected:

         virtual bool _ProcessReply(Command* cmd);

         bool                  fKeepCmds;
         Condition            *fReplyCond;
         CommandsQueue         fReplyedCmds;
         bool                  fReplyedRes;

   };

}

#endif
