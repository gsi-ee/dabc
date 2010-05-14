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
#ifndef DABC_WorkingProcessor
#define DABC_WorkingProcessor

#ifndef DABC_WorkingThread
#include "dabc/WorkingThread.h"
#endif

#ifndef DABC_Command
#include "dabc/Command.h"
#endif

namespace dabc {

   class Basic;
   class Folder;
   class Parameter;
   class CommandDefinition;

   class WorkingProcessor {

      enum EParsMasks {
            parsVisibilityMask = 0xFF,
            parsFixedMask = 0x100,
            parsChangableMask = 0x200,
            parsValidMask = 0x400
      };

      friend class WorkingThread;
      friend class Parameter;
      friend class Command;

      public:

         static int cmd_bool(bool v) { return v ? cmd_true : cmd_false; }

         enum EProcessorEventsCodes {
            evntFirstCore   = 1,    // events   1  .. 99 used only by Processor itself
            evntFirstSystem = 100,  // events 100 .. 999 used only inside DABC class
            evntFirstUser =   1000  // event from 1000 are available for users
         };

         enum EPriotiryLevels {
              priorityMaximum = 0,   // event queue with number 0 always has highest priority
              priorityMinimum = -1,  // this event will be submitted to queue with maximum number
              priorityDefault = -7,   // this code will be replaced with default priority for specified operation
              priorityMagic = -77     // this priority allows to submit commands even when processor is stopped
         };


         WorkingProcessor(Folder* parsholder = 0);
         virtual ~WorkingProcessor();

         /** Method returns name of required thread class for processor.
           * If returns 0 (default) any thread class is sufficient. */
         virtual const char* RequiredThrdClass() const { return 0; }

         bool AssignProcessorToThread(WorkingThread* thrd, bool sync = true);
         bool HaltProcessor();
         void RemoveProcessorFromThread(bool forget_thread);
         WorkingThread* ProcessorThread() const { return fProcessorThread; }
         const char* ProcessorThreadName() const { return fProcessorThread ? fProcessorThread->GetName() : 0; }

         // call this method when you want to be called after specified time period
         // 0 means to get called as soon as possible
         // <0 means cancel (deactivate) if possible (when it is not too late in time) previously scheduled timeout
         // >0 activate after specified interval
         void ActivateTimeout(double tmout_sec);

         void SetProcessorPriority(int nq) { fProcessorPriority = nq; }
         inline int ProcessorPriority() const { return fProcessorPriority; }

         uint32_t ProcessorId() const { return fProcessorId; }

         /** Synchronize processor with caller thread.
          *  Means let processor to execute all queued commands and process all queued events. */
         void SyncProcessor();

         /** Destroy processor asynchronously.
          * Can be called from any method of processor itself.
          * Replacement to operation delete this; */
         void DestroyProcessor();

         // this all about parameters list, which can be managed for any working processor

         Folder* GetTopParsFolder() { return fParsHolder; }

         Parameter* FindPar(const char* name) const;

         std::string GetParStr(const char* name, const std::string& defvalue = "") const;
         int GetParInt(const char* name, int defvalue = 0) const;
         double GetParDouble(const char* name, double defvalue = 0.) const;
         bool GetParBool(const char* name, bool defvalue = false) const;

         // these methods should be used either in constructor or in
         // dependent objects constructor (like Transport for Port)
         // Cfg means that parameter with given name will be created and its value
         // will be delivered back. Also will be checked if appropriate parameter
         // already exists in top configuration object (module, application)
         // If specified, command arguments list will be searched.
         // Priority: (max) Command, own parameter, external parameter, default value (min)
         std::string GetCfgStr(const char* name, const std::string& dfltvalue, Command* cmd = 0);
         int GetCfgInt(const char* name, int dfltvalue, Command* cmd = 0);
         double GetCfgDouble(const char* name, double dfltvalue, Command* cmd = 0);
         bool GetCfgBool(const char* name, bool dfltvalue, Command* cmd = 0);

         bool HasCfgPar(const char* name, Command* cmd = 0);

         static void SetGlobalParsVisibility(unsigned lvl = 1) { gParsVisibility = lvl; }
         static void SetParsCfgDefaults(unsigned flags) { gParsCfgDefaults = flags; }

         /** ! Assign command with processor before command be submitted to other processor
          * This produce ReplyCommand() call when command execution is finished
          * Pointer on command itself returned if operation was successful, otherwise 0 */
         Command* Assign(Command* cmd);

         /** Submit command for execution in the processor */
         bool Submit(Command* cmd, int priority = priorityDefault);

         /** Execute command in the processor. Event loop of caller thread is kept running */
         int Execute(Command* cmd, double tmout = -1., int priority = priorityDefault);

         /* Wrap of previous method, provided for convenience */
         int Execute(const char* cmdname, double tmout = -1., int priority = priorityDefault);

         int ExecuteInt(const char* cmdname, const char* intresname, double timeout_sec = -1.);

         std::string ExecuteStr(const char* cmdname, const char* strresname, double timeout_sec = -1.);


      protected:

         /** This method called before command will be executed.
          *  Only if cmd_ignore is returned, ExecuteCommand will be called for this command
          *  Otherwise command is replied with returned value */
         virtual int PreviewCommand(Command* cmd);

         /** Main method were command is executed */
         virtual int ExecuteCommand(Command* cmd) { return cmd_false; }

         /** Reimplement this method to react on command reply
          * Return true if command can be destroyed by framework*/
         virtual bool ReplyCommand(Command* cmd) { return true; }


         // Method is called when requested time point is reached
         // Rewrite method in derived class to react on this event
         // return value specifies time interval to the next timeout
         // Argument identifies time distance to previous timeout
         // Return value: <0 - no new timeout is required
         //               =0 - provide timeout as soon as possible
         //               >0 - activate timeout after this interval

         virtual double ProcessTimeout(double last_diff) { return -1.; }

         inline bool _IsFireEvent() const
         {
            return fProcessorThread && (fProcessorId>0) && !fProcessorStopped;
         }

         inline void _FireEvent(uint16_t evid)
         {
            if (_IsFireEvent())
               fProcessorThread->_Fire(CodeEvent(evid, fProcessorId), fProcessorPriority);
         }

         inline void FireEvent(uint16_t evid)
         {
            LockGuard lock(fProcessorMainMutex);
            _FireEvent(evid);
         }

         inline void _FireEvent(uint16_t evid, uint32_t arg, int pri = -1)
         {
            if (_IsFireEvent())
               fProcessorThread->_Fire(CodeEvent(evid, fProcessorId, arg), pri < 0 ? fProcessorPriority : pri);
         }

         inline void FireEvent(uint16_t evid, uint32_t arg, int pri = -1)
         {
            LockGuard lock(fProcessorMainMutex);
            _FireEvent(evid, arg, pri);
         }

         inline void _FireDoNothingEvent()
         {
            if (_IsFireEvent())
               fProcessorThread->_Fire(CodeEvent(WorkingThread::evntDoNothing), -1);
         }

         virtual void ProcessEvent(EventId);

         bool ActivateMainLoop();
         void ExitMainLoop();
         void SingleLoop(double tmout) { ProcessorThread()->SingleLoop(this, tmout); }

         void ProcessorSleep(double tmout);
         inline bool IsProcessorDestroyment() const { return fProcessorDestroyment; }
         inline bool IsProcessorHalted() const { return fProcessorHalted; }
         inline bool IsProcessorStopped() const { return fProcessorStopped; }

         int ExecuteIn(WorkingProcessor* dest, Command* cmd, double tmout = -1., int priority = priorityDefault);
         int ExecuteIn(WorkingProcessor* dest, const char* cmdname, double tmout = -1., int priority = priorityDefault);

         void CancelCommands();


         virtual void DoProcessorMainLoop() {}
         virtual void DoProcessorAfterMainLoop() {}

         // method called immediately after processor was assigned to thread
         // called comes from the thread context
         virtual void OnThreadAssigned() {}

         void DestroyAllPars();
         void DestroyPar(const char* name) { DestroyPar(FindPar(name)); }
         bool DestroyPar(Parameter* par);
         bool InvokeParChange(Parameter* par, const char* value, Command* cmd);

         bool SetParStr(const char* name, const char* value);
         bool SetParStr(const char* name, const std::string& value) { return SetParStr(name, value.c_str()); }
         bool SetParInt(const char* name, int value);
         bool SetParDouble(const char* name, double value);
         bool SetParBool(const char* name, bool value);

         bool SetParFixed(const char* name, bool on = true);
         void LockUnlockPars(bool on);

         Folder* MakeFolderForParam(const char* parname);

         static unsigned MakeParsFlags(unsigned visibility = 1, bool fixed = false, bool changable = true);
         unsigned SetParDflts(unsigned visibility = 1, bool fixed = false, bool changable = true);
         bool GetParDfltsVisible();
         bool GetParDfltsFixed();
         bool GetParDfltsChangable();

         virtual WorkingProcessor* GetCfgMaster() { return 0; }

         CommandDefinition* NewCmdDef(const char* cmdname);
         bool DeleteCmdDef(const char* cmdname);

         // methods for parameters creation
         Parameter* CreatePar(int kind, const char* name, const char* initvalue = 0, unsigned flags = 0);
         Parameter* CreateParStr(const char* name, const char* initvalue = 0, unsigned flags = 0);
         Parameter* CreateParInt(const char* name, int initvalue = 0, unsigned flags = 0);
         Parameter* CreateParDouble(const char* name, double initvalue = 0., unsigned flags = 0);
         Parameter* CreateParBool(const char* name, bool initvalue = false, unsigned flags = 0);
         Parameter* CreateParInfo(const char* name, int level = 0, const char* color = 0);

         // this method is called after parameter is changed
         // user may add its reaction on this event, but cannot
         // refuse parameter changing
         virtual void ParameterChanged(Parameter* par) {}

         WorkingThread*   fProcessorThread;
         uint32_t         fProcessorId;
         int              fProcessorPriority;

         Mutex*           fProcessorMainMutex;         // pointer on main thread mutex

         CommandsQueue    fProcessorSubmCommands;      /** list of submitted commands, protected via main thread mutex */
         CommandsQueue    fProcessorReplyCommands;     /** list of reply commands, protected via main thread mutex */
         CommandsQueue    fProcessorAssignCommands;    /** list of reply commands, protected via main thread mutex */

         CommandsQueue    fProcessorCommands;          // list of commands, which must be processed later
         int              fProcessorCommandsLevel;     /** Number of process commands recursion */

         Folder*          fParsHolder;

         unsigned         fParsDefaults;

         static unsigned  gParsVisibility; /** system-wide level for visibility of created parameters */
         static unsigned  gParsCfgDefaults; /** system-wide defaults for config parameters */

      private:
         enum { evntSubmitCommand = evntFirstCore,
                evntReplyCommand,
                evntPostCommand };


         bool TakeActivateData(TimeStamp_t& mark, double& interval);
         void ProcessCoreEvent(EventId);


         int ProcessCommand(dabc::Command* cmd);
         bool GetCommandReply(dabc::Command* cmd, bool* exe_ready);

         inline void CheckHaltCmds(int lvl = 1)
         {
            if (fProcessorRecursion==lvl)
               if (fProcessorHaltCommands.Size()>0) {
                  {
                     LockGuard lock(fProcessorMainMutex);
                     fProcessorHalted = true;
                  }
                  fProcessorHaltCommands.ReplyAll(cmd_true);
               }
         }


         bool             fProcessorActivateTmout; // used in activate to deliver timestamp to thread, locked by mutex
         TimeStamp_t      fProcessorActivateMark; // used in activate to deliver timestamp to thread, locked by mutex
         double           fProcessorActivateInterv; // used in activate to deliver timestamp to thread, locked by mutex

         TimeStamp_t      fProcessorPrevFire;     /** used in thread for timeout handling */
         TimeStamp_t      fProcessorNextFire;     /** used in thread for timeout handling */
         int              fProcessorRecursion;    /** counts how many recursive calls of ProcessEvent */
         bool             fProcessorDestroyment;  /** indicates if we start destroying of processor */
         bool             fProcessorHalted;       /** indicates if we doing processor halt */
         CommandsQueue    fProcessorHaltCommands; /** list of halt commands, which should be replied when processing is really halted */
         bool             fProcessorStopped;      /** indicate processor stop, no any events can be submitted */
   };

   class ConfigSource {
      public:
         ConfigSource(Command* cmd = 0, WorkingProcessor* proc = 0) : fCmd(cmd), fProc(proc) {}

         std::string GetCfgStr(const char* name, const std::string& dfltvalue)
         {
            if (fProc) return fProc->GetCfgStr(name, dfltvalue, fCmd);
            if (fCmd) return fCmd->GetStr(name, dfltvalue.c_str());
            return dfltvalue;
         }

         int GetCfgInt(const char* name, int dfltvalue)
         {
            if (fProc) return fProc->GetCfgInt(name, dfltvalue, fCmd);
            if (fCmd) return fCmd->GetInt(name, dfltvalue);
            return dfltvalue;
         }

         double GetCfgDouble(const char* name, double dfltvalue)
         {
            if (fProc) return fProc->GetCfgDouble(name, dfltvalue, fCmd);
            if (fCmd) return fCmd->GetDouble(name, dfltvalue);
            return dfltvalue;

         }

         bool GetCfgBool(const char* name, bool dfltvalue)
         {
            if (fProc) return fProc->GetCfgBool(name, dfltvalue, fCmd);
            if (fCmd) return fCmd->GetBool(name, dfltvalue);
            return dfltvalue;
         }

      protected:
         Command* fCmd;
         WorkingProcessor*  fProc;
   };



}

#endif
