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
#ifndef DABC_CommandsSet
#define DABC_CommandsSet

#ifndef DABC_WorkingProcessor
#include "dabc/WorkingProcessor.h"
#endif

namespace dabc {

   /** CommandsSet - class provides possibility to submit/control execution of commands sets
     * It's main idea - several commands can be submitted in parallel or sequential and
     * only when all submitted commands are completed, master command is replied with true.
     * One also can specify timeout interval, after which master command will be replied with false.
     * There are two ways to assign command to the set -
     * 1. Use CommandSet as client. It means, command submitted immediately and
     *    CommandSet is only analyzes reply of this command to see if result is ok.
     * 2. Use CommandSet::Add method. Then all these commands will be submitted
     *    to specified receiver after CommandSet::Completed call.
     * In all cases one should use CommandSet::Completed to identify that set
     * is configured and no more commands will be added.
     */

   class CommandsSet : protected WorkingProcessor {
      public:
         CommandsSet(WorkingThread* thrd = 0, bool parallel = true);
         virtual ~CommandsSet();

         /** Set default receiver for commands in the set.
          * By default, manager is receiver of commands */
         void SetReceiver(WorkingProcessor* proc);

         /** Set how commands from set executed - parallel (simultaneously) or sequential */
         void SetParallelExe(bool on = true) { fParallelExe = on; }

         /** Add new command to the set.
          * Command will be submitted to specified receiver during Run
          * If receiver not specified, default receiver will be used
          * User can submit commands itself after he add command to the set (do_submit should be false),
          * but commands set should be created with thread parameter specified */
         void Add(Command* cmd, WorkingProcessor* recv = 0, bool do_submit = true);

         /** Runs commands set synchronously with caller.
          * Execute all commands and returns cmd_true only when commands are executed
          * Result of each command execution can be accessed via GetCmdResult() method */
         int ExecuteSet(double tmout = 0.);

         /** Submit commands set for asynchronous execution.
          * Set will be assigned to current thread.
          * When all commands are executed, optional rescmd can be completed.
          * If tmout is specified (more than 0.), commands set will be terminated. */
         bool SubmitSet(Command* rescmd = 0, double tmout = 0.);

         /** Return total number of commands in the set */
         unsigned GetNumber() const;

         /** Return command from commands set.
          * If command was not completed, return 0 */
         dabc::Command* GetCommand(unsigned n);

         /** Return command result
          * If command was not completed, returns cmd_timedout */
         int GetCmdResult(unsigned n);

         void Cleanup();

      protected:

         virtual int ExecuteCommand(Command* cmd);

         virtual bool ReplyCommand(Command* cmd);

         virtual double ProcessTimeout(double last_diff);

         /** Method called when execution of set is finished. Can be reimplemented by user */
         virtual void SetCompleted(int res) {}

         /** Method called when execution of set is finished. Can be reimplemented by user */
         virtual void SetCommandCompleted(Command* cmd) {}


         struct CmdRec {
            dabc::Command*    cmd;
            WorkingProcessor* recv;
            int               state; // 0 - init, 1 - submitted, 2 - replied
            bool CanFree() { return state != 1; }
         };

         void CleanupCmds();

         bool SubmitNextCommand();

         int CheckExecutionResult();


         /** Actual receiver of commands, default is manager */
         WorkingProcessor  *fReceiver;

         /** List of commands, which should be submitted to receiver */
         Queue<CmdRec>      fCmds;

         /** Is execution of command done parallel - all they submitted immediately */
         bool               fParallelExe;

         /** Confirmation command */
         Command           *fConfirm;

         /** Execution timeout for command */
         double             fTimeout;

         /** Indicates execution mode */
         bool               fSyncMode;

         /** Indicates if set execution is completed */
         bool               fCompleted;

   };

}

#endif
