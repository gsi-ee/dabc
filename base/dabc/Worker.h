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

#ifndef DABC_Worker
#define DABC_Worker

#ifndef DABC_Object
#include "dabc/Object.h"
#endif

#ifndef DABC_Thread
#include "dabc/Thread.h"
#endif

#ifndef DABC_Command
#include "dabc/Command.h"
#endif

#ifndef DABC_Parameter
#include "dabc/Parameter.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

namespace dabc {

   class ParameterEvent;

   class WorkerRef;
   class Worker;
   class WorkerAddonRef;

   /** \brief Generic addon for \ref dabc::Worker
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    */

   class WorkerAddon : public Object {

      friend class Thread;
      friend class Worker;
      friend class WorkerAddonRef;

      protected:
         Reference  fWorker;

         virtual void ObjectCleanup();

         virtual void ProcessEvent(const EventId&) {}

         /** This is way to delete worker with addon inclusive */
         void DeleteWorker();

         /** This is possibility to delete addon itself, invoking worker command */
         void DeleteAddonItself();

         void SubmitWorkerCmd(Command cmd);

         void FireWorkerEvent(unsigned evid);

         // method will be called by worker when thread was assigned
         virtual void OnThreadAssigned() {}

         // analog to Worker::ActiavteTimeout
         bool ActivateTimeout(double tmout_sec);

         // analog to Worker::ProcessTimeout
         virtual double ProcessTimeout(double last_diff) { return -1.; }

         /** Light-weight command interface, which can be used from worker */
         virtual long Notify(const std::string&, int) { return 0; }

      public:

         virtual const char* ClassName() const { return "WorkerAddon"; }

         virtual std::string RequiredThrdClass() const { return std::string(); }

         WorkerAddon(const std::string& name);
         virtual ~WorkerAddon();
   };

   // ________________________________________________________________

   /** \brief %Reference on \ref dabc::WorkerAddon object
    *
    * \ingroup dabc_all_classes
    */

   class WorkerAddonRef : public Reference {
      DABC_REFERENCE(WorkerAddonRef, Reference, WorkerAddon)

      long Notify(const std::string& cmd, int arg = 0)
         { return GetObject() ? GetObject()->Notify(cmd,arg) : 0; }
   };

   // _________________________________________________________________

   /** \brief Active object, which is working inside \ref dabc::Thread
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    */

   class Worker : public Object {

      enum EParsMasks {
            parsVisibilityMask = 0xFF,
            parsFixedMask      = 0x100,
            parsChangableMask  = 0x200,
            parsValidMask      = 0x400
      };

      friend class Thread;
      friend class Parameter;
      friend class ParameterContainer;
      friend class Command;
      friend class Object;
      friend class WorkerRef;
      friend class WorkerAddon;

      private:

         /** \brief Method to 'forget' thread reference */
         void ClearThreadRef();

         /** Method called from the thread to inform Worker when it is assigned to thread */
         void InformThreadAssigned()
         {
            OnThreadAssigned();
            if (!fAddon.null()) fAddon()->OnThreadAssigned();
         }

         double ProcessAddonTimeout(double last_diff)
         {
            return fAddon.null() ? -1 : fAddon()->ProcessTimeout(last_diff);
         }

      protected:
         ThreadRef        fThread;                     ///< reference on the thread, once assigned remain whole time
         WorkerAddonRef   fAddon;                      ///< extension of worker for some special events
         Reference        fPublisher;                  ///< reference on publisher, once found, remain until end of object live

         uint32_t         fWorkerId;
         int              fWorkerPriority;             ///< priority of events, submitted by worker to the thread

         Mutex*           fThreadMutex;                ///< pointer on main thread mutex

         // FIXME: most workers should analyze FireEvent to recognize moment when worker is going into halt mode
         bool             fWorkerActive;               ///< indicates if worker can submit events to the thread
         unsigned         fWorkerFiredEvents;          ///< indicate current balance between fired and processed events, used to correctly halt worker

         CommandsQueue    fWorkerCommands;             ///< all kinds of commands, processed by the worker

         int              fWorkerCommandsLevel;      /** Number of process commands recursion */

         Hierarchy        fWorkerHierarchy;            ///< place for publishing of worker parameters


         /** \brief Inherited method from Object, invoked at the moment when worker requested to be destroyed by its thread */
         virtual bool DestroyByOwnThread();

         /** \brief Central cleanup method for worker */
         virtual void ObjectCleanup();

         /** \brief Method to clear object reference, will be called from thread context (when possible) */
         virtual void ObjectDestroyed(Object*) {}

         /** \brief Method indicates if worker is running in the thread and accepts normal events.
          * All events accepted by the worker will be delivered and processed.
          * If destruction or halt of the worker starts, no new events can be submitted (execpt command with magic priority)
          * In this phase IsWorkerActive() will already return false  */
         inline bool IsWorkerActive() const { return fWorkerActive; }

         /** \brief Special constructor, designed for inherited classes */
         Worker(const ConstructorPair& pair);

      public:

         static int cmd_bool(bool v) { return v ? cmd_true : cmd_false; }

         enum EWorkerEventsCodes {
            evntFirstCore   = 1,    // events   1  .. 99 used only by Worker itself
            evntFirstAddOn  = 100,  // events 100 .. 199 are dedicated for specific add-ons
            evntFirstSystem = 200,  // events 200 .. 999 used only inside DABC class
            evntFirstUser =   1000  // event from 1000 are available for users
         };

         enum EPriotiryLevels {
              priorityMaximum = 0,   // event queue with number 0 always has highest priority
              priorityMinimum = -1,  // this event will be submitted to queue with maximum number
              priorityDefault = -7,  // this code will be replaced with default priority for specified operation
              priorityMagic = -77    // this priority allows to submit commands even when processor is stopped
         };

         Worker(Reference parent, const std::string& name);
         virtual ~Worker();

         virtual const char* ClassName() const { return "Worker"; }

         /** Method returns name of required thread class for processor.
           * If returns empty string, any thread class is sufficient. */
         virtual std::string RequiredThrdClass() const
         { return fAddon.null() ? std::string() : fAddon()->RequiredThrdClass(); }

         /** \brief Indicates if pointer on thread is not zero; thread-safe */
         bool HasThread() const;

         /** \brief Retutns true if called from thread. If thread not assigned, also returns true */
         bool IsOwnThread() const;

         /** \brief Assign worker to thread, worker becomes active immediately */
         bool AssignToThread(ThreadRef thrd, bool sync = true);

         /** \brief Creates appropriate thread for worker and assign worker to the thread */
         bool MakeThreadForWorker(const std::string& thrdname = "");

         /** \brief Detach worker from the thread, later worker can be assigned to some other thread
          * Method especially useful to normally finish object recursion (if it possible).
          * If successful, object can be assigned to new thread again or destroyed.
          * Worker halt (recursion break) is also performed during object destroyment,
          * therefore it is not necessary to call this method before destroy of the worker */
         bool DettachFromThread();

         /** \brief Return reference on the worker thread; thread-safe */
         ThreadRef thread();

         /** \brief Returns name of the worker thread; thread-safe  */
         std::string ThreadName() const;

         /** \brief Method used to produce timeout events in the worker
          *
          *  After specified time interval ProcessTimeout() method of worker will be called.
          *
          *  \param[in] tmout_sec has following meaning:
          *     = 0 ProcessTimeout() will be called as soon as possible
          *     < 0 deactivate if possible previously scheduled timeout
          *     > 0 activate after specified interval
          *  \returns false if timeout cannot be configured (when thread is not assigned or not active) */
         bool ActivateTimeout(double tmout_sec);

         void SetWorkerPriority(int nq) { fWorkerPriority = nq; }
         inline int WorkerPriority() const { return fWorkerPriority; }

         uint32_t WorkerId() const { return fWorkerId; }

         // this all about parameters list, which can be managed for any working processor

         /** \brief Returns reference on worker parameter object */
         Parameter Par(const std::string& name) const;

         /** \brief Returns configuration field of specified name
          * Configuration value of specified name searched in following places:
          * 1. As field in command
          * 2. As parameter value in the worker itself
          * 3. As value in xml file
          * 4. As parameter value of all parents
          * */
         RecordField Cfg(const std::string& name, Command cmd = 0) const;

         /** ! Assign command with processor before command be submitted to other processor
          * This produce ReplyCommand() call when command execution is finished
          * Command itself returned if operation was successful, otherwise null() command */
         Command Assign(Command cmd);

         /** Returns true if command can be submitted to worker */
         bool CanSubmitCommand() const;

         /** Submit command for execution in the processor */
         bool Submit(Command cmd);

         /** Execute command in the processor. Event loop of caller thread is kept running */
         bool Execute(Command cmd, double tmout = -1.);

         bool Execute(const std::string& cmd, double tmout = -1.) { return Execute(Command(cmd), tmout); }

         /** \brief Return address, which can be used to submit command to the worker
          * If full specified, command can be submitted from any node, which has connection to this node */
         std::string WorkerAddress(bool full = true);

         /** Assigns addon to the worker
          * Should be called before worker assigned to the thread */
         void AssignAddon(WorkerAddon* addon);

      protected:

         /** This method called before command will be executed.
          *  Only if cmd_ignore is returned, ExecuteCommand will be called for this command
          *  Otherwise command is replied with returned value
          *  Contrary to ExecuteCommand, PreviewCommand used by dabc classes itself.
          *  Therefore, if method redefined in inherited class,
          *  one should always call PreviewCommand of base class first. */
         virtual int PreviewCommand(Command cmd);

         /** Main method where commands are executed */
         virtual int ExecuteCommand(Command cmd);

         /** Reimplement this method to react on command reply
          * Return true if command can be destroyed by framework*/
         virtual bool ReplyCommand(Command cmd);

         // Method is called when requested time point is reached
         // Rewrite method in derived class to react on this event
         // return value specifies time interval to the next timeout
         // Argument last_diff identifies time distance to previous timeout
         // Return value: <0 - no new timeout is required
         //               =0 - provide timeout as soon as possible
         //               >0 - activate timeout after this interval
         virtual double ProcessTimeout(double last_diff) { return -1.; }

         inline bool _IsFireEvent() const
         {
            return fThread() && fWorkerActive;
         }

         inline bool _FireEvent(uint16_t evid)
         {
            if (!_IsFireEvent()) return false;
            fThread()->_Fire(EventId(evid, fWorkerId), fWorkerPriority);
            fWorkerFiredEvents++;
            return true;
         }

         inline bool FireEvent(uint16_t evid)
         {
            LockGuard lock(fThreadMutex);
            return _FireEvent(evid);
         }

         inline bool _FireEvent(uint16_t evid, uint32_t arg, int pri = -1)
         {
            if (!_IsFireEvent()) return false;
            fThread()->_Fire(EventId(evid, fWorkerId, arg), pri < 0 ? fWorkerPriority : pri);
            fWorkerFiredEvents++;
            return true;
         }

         inline bool FireEvent(uint16_t evid, uint32_t arg, int pri = -1)
         {
            LockGuard lock(fThreadMutex);
            return _FireEvent(evid, arg, pri);
         }


         inline bool _FireDoNothingEvent()
         {
            if (!_IsFireEvent()) return false;
            fThread()->_Fire(EventId(Thread::evntDoNothing), -1);
            return true;
         }

         virtual void ProcessEvent(const EventId&);

         bool ActivateMainLoop();

         bool SingleLoop(double tmout) { return fThread()->SingleLoop(fWorkerId, tmout); }

         void WorkerSleep(double tmout);

         /** Executes command in specified worker. Call allowed only from worker thred (therefore method protected).
          * Makes it easy to recognise caller thread and keep its event loop running.
          * Equivalent to dest->Execute(cmd). */
         bool ExecuteIn(Worker* dest, Command cmd);

         void CancelCommands();

         /** Internal - entrance function for main loop execution. */
         virtual void DoWorkerMainLoop() {}

         /** Internal - function executed after leaving main loop. */
         virtual void DoWorkerAfterMainLoop() {}

         // method called immediately after processor was assigned to thread
         // called comes from the thread context
         virtual void OnThreadAssigned() {}

         virtual bool Find(ConfigIO &cfg);

         virtual Parameter CreatePar(const std::string& name, const std::string& kind = "");

         CommandDefinition CreateCmdDef(const std::string& name);

         /** \brief Method must be used if worker wants to destroy parameter */
         bool DestroyPar(const std::string& name);

         /** Subscribe to parameter events from local or remote node */
         bool RegisterForParameterEvent(const std::string& mask, bool onlychangeevent = true);

         /** Unsubscribe to parameter events from local or remote node */
         bool UnregisterForParameterEvent(const std::string& mask);

         /** Interface method to retrieve subscribed parameter events */
         virtual void ProcessParameterEvent(const ParameterEvent& evnt) {}

         /** \brief Return reference on publisher.
          * First time publisher is searched in objects hierarchy and reference
          * stored in the internal structures.
          * User can call CleanupPublisher to clean reference and unsubscribe/unpublish
          * all registered structures.    */
         Reference GetPublisher();

         virtual bool Publish(const Hierarchy& h, const std::string& path);

         virtual bool PublishPars(const std::string& path);

         virtual bool Unpublish(const Hierarchy& h, const std::string& path);

         virtual bool Subscribe(const std::string& path);

         virtual bool Unsubscribe(const std::string& path);

         /** \brief Release reference on publisher and unsubscribe/unpublish all registered entries */
         void CleanupPublisher(bool sync = true);

         /** \brief Method called before publisher makes next snapshot of hierarchy.
          *  Worker is able to make any kind of changes   */
         virtual void BeforeHierarchyScan(Hierarchy& h) {}

      private:

         enum { evntSubmitCommand = evntFirstCore,
                evntReplyCommand };

         void ProcessCoreEvent(EventId);

         int ProcessCommand(dabc::Command cmd);
         bool GetCommandReply(dabc::Command& cmd, bool* exe_ready);

         /** Method called by parameter object which is belong to the worker.
          * Method is called from the thread where parameter is changing - it could be not a worker thread.
          * Means one should use protected by the mutex worker fields.
          * If monitoring field for parameter is specified, ParameterEvent will be to the worker thread */
         void WorkerParameterChanged(bool forcecall, ParameterContainer* par, const std::string& value);

         /** Method to process parameter recording in worker thread */
         void ProcessParameterRecording(ParameterContainer* par);
   };

   // __________________________________________________________________________

   /** \brief %Reference on \ref dabc::Worker
    *
    * \ingroup dabc_all_classes
    */

   class WorkerRef : public Reference {

      DABC_REFERENCE(WorkerRef, Reference, Worker)

      public:

         bool Submit(Command cmd);

         bool Execute(Command cmd, double tmout = -1.);

         bool Execute(const std::string& cmd, double tmout = -1.);

         /** \brief Returns reference on parameter */
         Parameter Par(const std::string& name) const;

         /** \brief Returns configuration record of specified name */
         RecordField Cfg(const std::string& name, Command cmd = 0) const
         { return GetObject() ? GetObject()->Cfg(name, cmd) : cmd.GetField(name); }

         /** \brief Returns true when thread is assigned to the worker */
         bool HasThread() const;

         /** \brief Returns thread name of worker assigned */
         std::string ThreadName() const
           { return GetObject() ? GetObject()->ThreadName() : std::string(); }

         ThreadRef thread()
            { return GetObject() ? GetObject()->thread() : ThreadRef(); }

         /** \brief Returns true if two workers share same thread */
         bool IsSameThread(const WorkerRef& ref);

         bool MakeThreadForWorker(const std::string& thrdname = "")
           { return GetObject() ? GetObject()->MakeThreadForWorker(thrdname) : false; }

         /** \brief Returns true if command can be submitted to the worker */
         bool CanSubmitCommand() const;

         /** Synchronize worker with caller thread.
          *  We let worker to execute all queued commands and process all queued events. */
         bool SyncWorker(double tmout = -1.);

         bool FireEvent(uint16_t evid, uint32_t arg) { return GetObject() ? GetObject()->FireEvent(evid, arg) : false; }

         bool FireEvent(const EventId& ev) { return GetObject() ? GetObject()->FireEvent(ev.GetCode(), ev.GetArg()) : false; }
   };

   // _____________________________________________________________________________


   /** This command used to distribute parameter event to receivers */
   class CmdParameterEvent : public Command {

      DABC_COMMAND(CmdParameterEvent, "CmdParameterEvent")


      CmdParameterEvent(const std::string& parname, const std::string& parvalue, int evid, bool attrmodified = false) :
         dabc::Command(CmdName())
      {
         SetStr("ParName", parname);
         if (!parvalue.empty()) SetStr("ParValue", parvalue);
         if (evid!=parModified) SetInt("Event", evid);
         if (attrmodified) SetBool("AttrMod", true);
      }
   };

   // _____________________________________________________________________

   class ParameterEvent : protected CmdParameterEvent {
      friend class Worker;

      protected:
         ParameterEvent() : CmdParameterEvent() {}

         ParameterEvent(const Command& cmd) : CmdParameterEvent(cmd) {}

      public:

         std::string ParName() const { return GetStr("ParName"); }
         std::string ParValue() const { return GetStr("ParValue"); }
         int EventId() const { return GetInt("Event", parModified); }
         bool AttrModified() const { return GetBool("AttrMod", false); }
   };

}

#endif
