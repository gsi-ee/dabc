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

#ifndef DABC_Application
#define DABC_Application

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#ifndef DABC_Queue
#include "dabc/Queue.h"
#endif

namespace dabc {

   /** Command used to activate application - all necessary transitions will be
    * performed to bring application in working state. Used by main dabc runnable */
   class CmdInvokeAppRun : public Command {
      DABC_COMMAND(CmdInvokeAppRun, "InvokeAppRun");
   };

   /** Command used to finish application work - it should destroy all components and
    * finally declare application as ready - IsDone() should return true at the end  */
   class CmdInvokeAppFinish : public Command {
      DABC_COMMAND(CmdInvokeAppFinish, "InvokeAppFinish");
   };

   class CmdInvokeTransition : public Command {
      DABC_COMMAND(CmdInvokeTransition, "InvokeTransition");

      void SetTransition(const std::string& cmd) { Field("Transition").SetStr(cmd); }
      std::string GetTransition() const { return Field("Transition").AsStdStr(); }
   };


   class Manager;
   class ApplicationRef;

   /** \brief Base application class
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    * Defines main methods and minimal set of state-machine states and commands.
    */

   class ApplicationBase : public Worker {

      friend class Manager;
      friend class ApplicationRef;

      public:

         typedef void* ExternalFunction();

         // these are three states in default state machine
         static const char* stHalted() { return "Halted"; }
         static const char* stRunning() { return "Running"; }
         static const char* stFailure() { return "Failure"; }

         // these are only two command in default state machine
         static const char* stcmdDoStart() { return "DoStart"; }
         static const char* stcmdDoStop() { return "DoStop"; }

      protected:

         ExternalFunction*  fInitFunc;

         bool               fWasRunning; ///< indicate if application was running at least once

         virtual int ExecuteCommand(Command cmd);

         /** \brief Cleanup application */
         virtual void ObjectCleanup();

         /** Method should bring application in to running state */
         virtual bool PerformApplicationRun();

         /** Method brings application in finish state, when application can be destroyed */
         virtual bool PerformApplicationFinish();

         /** Directly changes value of the state parameter */
         void SetState(const std::string& name) { Par(StateParName()).SetStr(name); }

         /** Set external function, which creates all necessary components of the application */
         void SetInitFunc(ExternalFunction* initfunc);

         /** Default method to automatically create all components like pools, modules, connections, ...
          * Returns true if specified components were created successfully */
         bool DefaultInitFunc();

         /** Return true if all application-relevant modules are running */
         virtual bool IsModulesRunning();

         /** Start all application modules */
         virtual bool StartModules();

         /** Stop all application modules */
         virtual bool StopModules();

         /** Delete all components created in application, excluding state parameter */
         virtual bool CleanupApplication();

         /** Should return true if specified transition allowed */
         virtual bool IsTransitionAllowed(const std::string&);

         /** Do action, required by transition, called in context of application thread */
         virtual bool DoStateTransition(const std::string&);

         /** Method should return true when application is did the job and can be finished.
          * By default it is the case, when all modules stopped during run state */
         virtual bool IsWorkDone();

         /** Returns true if application finished its work */
         virtual bool IsFinished();

         /** Default state machine command timeout */
         virtual int SMCommandTimeout() const { return 10; }

         /** \brief Fill application objects hierarchy */
         virtual void BuildWorkerHierarchy(HierarchyContainer* cont);

      public:

         ApplicationBase();
         virtual ~ApplicationBase();

         static const char* StateParName() { return "State"; }

         std::string GetState() const { return Par(StateParName()).AsStdStr(); }

         /** Execute transition command */
         bool ExecuteStateTransition(const std::string& cmd, double tmout = -1.);

         /** Invoke (make asynchron) state transition */
         bool InvokeStateTransition(const std::string& cmd);

         /** Call this method that application should check if job is finished*
          * Application will be automatically stopped when it happens */
         bool CheckWorkDone();

         virtual bool Find(ConfigIO &cfg);

         virtual const char* ClassName() const { return typeApplication; }
   };

   // =====================================================================================

   /** \brief Base class for user-specific applications.
    *
    * \ingroup dabc_user_classes
    * \ingroup dabc_all_classes
    *
    * Main aim of writing user-specific application is creation and management of
    * different application components like modules, transports, devices, which cannot
    * be handled by standard DABC components.
    *
    */


   class Application : public ApplicationBase {

      friend class ApplicationRef;

      public:

         struct NodeStateRec {
            bool active;
         };

         static const char* stNull() { return "Null"; }
         static const char* stConfigured() { return "Configured"; }
         static const char* stReady() { return "Ready"; }
         static const char* stError() { return "Error"; }

         static const char* stcmdDoConfigure() { return "DoConfigure"; }
         static const char* stcmdDoEnable() { return "DoEnable"; }
         static const char* stcmdDoError() { return "DoError"; }
         static const char* stcmdDoHalt() { return "DoHalt"; }

      private:

         void GetFirstNodesConfig();

      protected:

         friend class Manager;

         std::string        fAppClass;
         PointersVector     fNodes;

         virtual int ExecuteCommand(Command cmd);

         /** Method should bring application in to running state.
          * In current implementation it is done via few steps - Configure, Enable, Run */
         virtual bool PerformApplicationRun();

         virtual bool PerformApplicationFinish();

         /** \brief Remember current state of all nodes seen by the application
          * All changes notifications will be done relative to this stored version
          * This call explicitly done after application constructor */

         bool MakeSystemSnapshot(double tmout = 10.);

         NodeStateRec* NodeRec(unsigned n) const { return n < NumNodes() ? (NodeStateRec*) fNodes[n] : 0; }

         /** Returns true if application life-cycle is finished */
         virtual bool IsFinished();

      public:

          Application(const char* classname = 0);

          virtual ~Application();

          /** Should return true if specified transition allowed */
          virtual bool IsTransitionAllowed(const std::string& trans_cmd);

          /** Method could be reimplemented to perform state transition */
          virtual bool DoStateTransition(const std::string& trans_cmd);

          // These methods are called in the state transition process,
          // but always from application thread (via command channel)
          // Implement them to create modules, test if connection is done and
          // do specific actions before modules stars, after modules stopped and before module destroyed

          // FIXME: these are methods from dabc v1, one could remove them at all

          virtual bool CreateAppModules();
          virtual bool BeforeAppModulesStarted() { return true; }
          virtual bool AfterAppModulesStopped() { return true; }
          virtual bool BeforeAppModulesDestroyed() { return true; }

          virtual int SMCommandTimeout() const { return 10; }

          virtual const char* ClassName() const { return fAppClass.c_str(); }

          // ___________ these are new methods ______________

          unsigned NumNodes() const { return fNodes.size(); }

          bool NodeActive(unsigned n) const { return NodeRec(n) ? NodeRec(n)->active : false; }
   };

   // ________________________________________________________________________________


   /** \brief %Reference on \ref dabc::Application class
    *
    * \ingroup dabc_all_classes
    */

   class ApplicationRef : public WorkerRef {

      DABC_REFERENCE(ApplicationRef, WorkerRef, Application)

      public:

         /** \brief Verifies that all modules finish there job. If yes, complete process will be stopped. */
         void CheckWorkDone();

         /** \brief Return true if application normally finished its work (module is stopped) */
         bool IsWorkDone();

         /** \brief Returns true when application is cleaned up */
         bool IsFinished();
   };



}

#endif
