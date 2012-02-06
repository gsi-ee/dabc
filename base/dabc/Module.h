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

#ifndef DABC_Module
#define DABC_Module

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

#ifndef DABC_Command
#include "dabc/Command.h"
#endif

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

namespace dabc {

   class PoolHandle;
   class ModuleItem;
   class Manager;
   class Port;
   class Timer;
   class Transport;

   class Module : public Worker {

      friend class Manager;
      friend class ModuleItem;
      friend class Port;

      enum  { evntReinjectlost = evntFirstSystem };

      protected:
         typedef std::vector<unsigned> ItemsIndexVector;

         bool                       fRunState;    //!< module is in running state
         PointersVector             fItems;       // map for fast search of module items
         ItemsIndexVector           fInputPorts;  // map for fast access to input ports
         ItemsIndexVector           fOutputPorts; // map for fast access to output ports
         ItemsIndexVector           fPorts;       // map for fast access to i/o ports
         ItemsIndexVector           fPoolHandels; // map for fast access to memory pools handles
         Queue<EventId, true>       fLostEvents;  // events, coming while module is sleeping

      protected:

         Module(const char* name = "module", Command cmd = 0);

         Port* GetPortItem(unsigned id) const;

      public:
         virtual ~Module();

         // this methods can be used from outside of module

         /** Starts execution of the module code */
         void Start();

         /** Stops execution of the module code */
         void Stop();

         // end of public methods, rest later will be moved to protected area

         PoolHandle* Pool(unsigned n = 0) const;

         unsigned NumInputs() const { return fInputPorts.size(); }
         unsigned NumOutputs() const { return fOutputPorts.size(); }
         unsigned NumIOPorts() const { return fPorts.size(); }

         Port* Input(unsigned n = 0) const { return n < fInputPorts.size() ? GetPortItem(fInputPorts[n]) : 0; }
         Port* Output(unsigned n = 0) const { return n < fOutputPorts.size() ? GetPortItem(fOutputPorts[n]) : 0; }
         Port* IOPort(unsigned n = 0) const { return n < fPorts.size() ? GetPortItem(fPorts[n]) : 0; }

         Port* FindPort(const char* name);
         PoolHandle* FindPool(const char* name = 0);

         unsigned InputNumber(Port* port);
         unsigned OutputNumber(Port* port);
         unsigned IOPortNumber(Port* port);

         Reference GetTimersFolder(bool force = false);

         ModuleItem* GetItem(unsigned id) const { return id<fItems.size() ? (ModuleItem*) fItems.at(id) : 0; }

         inline bool IsRunning() const { return fRunState; }

         // generic users event processing function
         virtual void ProcessUserEvent(ModuleItem* item, uint16_t evid) {}

         // some useful routines for I/O handling
         bool CanSendToAllOutputs() const;
         void SendToAllOutputs(const Buffer& buf);

         virtual const char* ClassName() const { return "Module"; }

      protected:

         // these two methods called before start and after stop of module
         // disregard of module has its own mainloop or not
         virtual void BeforeModuleStart() {}
         virtual void AfterModuleStop() {}

         // user must redefine method when it want to execute commands.
         // If method return true or false (cmd_true or cmd_false),
         // command considered as executed and will be replied
         // Any other arguments (cmd_postponed) means that execution
         // will be postponed in the module itself
         // User must call Module::ExecuteCommand() to enable processing of standard commands
         virtual int ExecuteCommand(Command cmd) { return dabc::Worker::ExecuteCommand(cmd); }

         // This is place of user code when command completion is arrived
         // User can implement any analysis of command data in this method
         // If returns true, command object will be delete automatically,
         // otherwise ownership delegated to user.
         // In that case command must be at some point replied - means user should call cmd.Reply(res) method.
         virtual bool ReplyCommand(Command cmd) { return dabc::Worker::ReplyCommand(cmd); }

         // =======================================================

         /** Creates handle for memory pool, which preserves reference on memory pool
          * and provides fast access to memory pool functionality.
          * Memory pool should exist before handle can be created
          */
         PoolHandle* CreatePoolHandle(const char* poolname);

         Port* CreateIOPort(const char* name, PoolHandle* pool = 0, unsigned recvqueue = 10, unsigned sendqueue = 10);

         Port* CreateInput(const char* name, PoolHandle* pool = 0, unsigned queue = 10)
           { return CreateIOPort(name, pool, queue, 0); }
         Port* CreateOutput(const char* name, PoolHandle* pool = 0, unsigned queue = 10)
           { return CreateIOPort(name, pool, 0, queue); }

         bool CreatePoolUsageParameter(const char* name, double interval = 1., const char* poolname = 0);

         Timer* CreateTimer(const char* name, double period_sec = 1., bool synchron = false);
         Timer* FindTimer(const char* name);
         bool ShootTimer(const char* name, double delay_sec = 0.);

         void GetUserEvent(ModuleItem* item, uint16_t evid);

         virtual void ProcessEvent(const EventId&);

         /** \brief Inherited method, called during module cleanup.
          * Used to stop module if it is still running */
         virtual void ObjectCleanup();

         /** Method, which can be reimplemented by user and should cleanup all references on buffers
          * and other objects */
         virtual void ModuleCleanup() {}

         bool DoStop();
         bool DoStart();

         virtual void OnThreadAssigned();
         virtual int PreviewCommand(Command cmd);

         virtual void PoolHandleCleaned(PoolHandle* pool) {}

       private:

         void ItemCleaned(ModuleItem* item);
         void ItemCreated(ModuleItem* item);
         void ItemDestroyed(ModuleItem* item);
   };


   class ModuleRef : public WorkerRef {

      DABC_REFERENCE(ModuleRef, WorkerRef, Module)

      public:

         bool IsRunning() const { return GetObject() ? GetObject()->IsRunning() : false; }

         bool Start() { return Execute("StartModule"); }

         bool Stop() { return Execute("StopModule"); }

         bool CheckConnected() { return Execute("CheckConnected"); }

         /** Returns true if specified input is connected */
         bool IsInputConnected(unsigned ninp);

         /** Returns true if specified output is connected */
         bool IsOutputConnected(unsigned ninp);

   };

};

#endif
