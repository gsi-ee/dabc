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

#ifndef DABC_string
#include "dabc/string.h"
#endif

#include <vector>

namespace dabc {

   class CmdStateTransition : public Command {
      DABC_COMMAND(CmdStateTransition, "CmdStateTransition");

      CmdStateTransition(const std::string& state) :
         dabc::Command(CmdName())
      {
         SetStr("State", state);
      }
   };


   class Manager;
   class ApplicationRef;

   /** \brief Base class for user-specific applications.
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    * Defines main methods and minimal set of state-machine states and commands.
    * Main aim of writing user-specific application is creation and management of
    * different application components like modules, transports, devices, which cannot
    * be handled by standard DABC methods.
    * For instance, complex connection scheme or modules creation,
    * dependent from application parameters.
    *
    * Following states are defined in the state-machine:
    *  'Halted'  - initial state, no any object existed
    *  'Ready'   - all objects are created and ready for start
    *  'Running' - modules are running
    *  'Failure' - error state
    *
    * Several commands are provided to switch between states:
    *  'DoConfigure' - switch to 'Ready' state
    *  'DoStart'     - switch to 'Running' state
    *  'DoStop'      - switch to 'Ready' state from 'Running'
    *  'DoHalt'      - switch to 'Halted' state
    *
    *  Any command can be executed at any time - state machine will try to bring
    *  system to specifies state.
    *
    *  Commands can be triggered via web interface.
    *
    *  By default, application will be brought to the 'Running' state by the dabc.
    *  Only if self='false' specified in <Context> node attributes, application will be created and remain as is
    *
    */

   class Application : public Worker {

      friend class Manager;
      friend class ApplicationRef;

      public:

         typedef void* ExternalFunction();

         // these are states in default state machine
         static const char* stHalted()  { return "Halted"; }
         static const char* stReady()   { return "Ready"; }
         static const char* stRunning() { return "Running"; }
         static const char* stFailure() { return "Failure"; }
         static const char* stTransition() { return "Transition"; }

         // these are commands provided by state machine
         static const char* stcmdDoConfigure() { return "DoConfigure"; }
         static const char* stcmdDoStart() { return "DoStart"; }
         static const char* stcmdDoStop()  { return "DoStop"; }
         static const char* stcmdDoHalt()  { return "DoHalt"; }

      protected:

         std::string        fAppClass;

         ExternalFunction*  fInitFunc;

         bool               fAnyModuleWasRunning; ///< indicate when any module was running, than once can automatically stop application

         bool               fSelfControl;   ///< when true, application itself decide when stop main loop

         std::vector<std::string> fAppDevices;   ///< list of devices, created by application
         std::vector<std::string> fAppPools;     ///< list of pools, created by application
         std::vector<std::string> fAppModules;   ///< list of modules, created by application

         virtual int ExecuteCommand(Command cmd);

         /** \brief Method called at thread assignment - application may switch into running state */
         virtual void OnThreadAssigned();

         /** \brief Cleanup application */
         virtual void ObjectCleanup();

         /** Directly changes value of the state parameter */
         void SetAppState(const std::string& name);

         /** Set external function, which creates all necessary components of the application */
         void SetInitFunc(ExternalFunction* initfunc);

         /** Default method to automatically create all components like pools, modules, connections, ...
          * Returns true if specified components were created successfully */
         bool DefaultInitFunc();

         /** Return true if all application-relevant modules are running */
         virtual bool IsModulesRunning();

         /** Create application modules */
         virtual bool CreateAppModules();

         /** Start all application modules */
         virtual bool StartModules();

         /** Stop all application modules */
         virtual bool StopModules();

         /** Delete all components created in application, excluding state parameter */
         virtual bool CleanupApplication();

         /** Do action, required to make transition into specified state */
         virtual bool DoTransition(const std::string& state);

         /** Default state machine command timeout */
         virtual int SMCommandTimeout() const { return 10; }

         virtual void BuildFieldsMap(RecordFieldsMap* cont);

         /** Timeout used by application to control stop state of modules and brake application */
         virtual double ProcessTimeout(double);

      public:

         Application(const char* classname = 0);
         virtual ~Application();

         static const char* StateParName() { return "State"; }

         std::string GetState() const { return Par(StateParName()).Value().AsStr(); }

         virtual bool Find(ConfigIO &cfg);

         virtual const char* ClassName() const { return fAppClass.c_str(); }
   };

   // ________________________________________________________________________________


   /** \brief %Reference on \ref dabc::Application class
    *
    * \ingroup dabc_all_classes
    */

   class ApplicationRef : public WorkerRef {

      DABC_REFERENCE(ApplicationRef, WorkerRef, Application)

      bool ChangeState(const std::string& state)
      { return Execute(CmdStateTransition(state)); }

      bool StartAllModules()
      { return Execute("StartAllModules"); }

      bool StopAllModules()
      { return Execute("StopAllModules"); }

      /** Adds object into application list
       *  List used when objects must be destroyed or application start/stop should be executed
       * @param kind - 'device', 'pool', module'
       * @param name - object name
       * @return true if successes  */
      bool AddObject(const std::string& kind, const std::string& name);

   };



}

#endif
