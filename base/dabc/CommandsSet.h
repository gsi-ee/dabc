#ifndef DABC_CommandsSet
#define DABC_CommandsSet

#ifndef DABC_CommandClient
#include "dabc/CommandClient.h"
#endif

#ifndef DABC_WorkingProcessor
#include "dabc/WorkingProcessor.h"
#endif

namespace dabc {

   /** CommandsSet - class provides possibility to submit/control execution of commands sets
     * It's main idea - several commands can be submitted in parallel or sequential and
     * only when all of them is completed, master command is replyed with true argument
     * One also can specify timeout interval, after which master command will be replyed with false.
     * There are two ways to assign command to the set -
     * 1. Use CommandSet as client. It means, command submitted immediately and
     *    CommandSet is only analyzes reply of this command to see if result is ok.
     * 2. Use CommandSet::Add method. Then all these commands will be submitted
     *    to specified receiver after CommandSet::Completed call.
     * In all cases one should use CommandSet::Completed to identify that set
     * is configured and no more commands will be added.
     */

   class CommandsSet : public CommandClientBase,
                       public WorkingProcessor {

      public:
         CommandsSet(Command* main_cmd, bool parallel_exe = true);
         virtual ~CommandsSet();

         // Set receiver of commands, when set is activated
         // By default, manager is receiver of commands
         void SetReceiver(CommandReceiver* rcv);

         // add new command to the set
         void Add(Command* cmd);

         // call this method after all commands is submitted to set
         // if execution is completed at that time, set will be safely deleted
         // no action with set variable is not allowed after this call
         static void Completed(dabc::CommandsSet* set, double timeout_sec = -1);

         void Reset(bool mainres = false);

         // redefine this method to react on the reply of slave command
         // return true, if command object is reused in the method
         // false mean that user has nothing to do with the command
         virtual bool ProcessCommandReply(Command*) { return false; }

      protected:

         virtual bool _ProcessReply(Command* cmd);

         bool _SubmitNextCommands();

         virtual double ProcessTimeout(double);

         Manager           *fMgr;         // pointer on manager
         CommandReceiver   *fReceiver;    // actual receiver of set command
         Command           *fMain;        // master command
         bool               fParallelExe; // parallel execution of all commands
         CommandsQueue      fCmdsSet;     // set of all commands, submitted to the set
         bool               fMainRes;
         bool               fCompleted;
         bool               fNeedDestroy; // true if object must be destroyed
   };
}

#endif
