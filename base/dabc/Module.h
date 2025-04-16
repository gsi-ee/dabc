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

#ifndef DABC_Port
#include "dabc/Port.h"
#endif

namespace dabc {

   class Manager;
   class ModuleSync;
   class ModuleAsync;
   class ModuleRef;

   /** \brief Base for dabc::ModuleSync and dabc::ModuleAsync classes.
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    */

   class Module : public Worker {

      friend class Manager;
      friend class ModuleItem;
      friend class Timer;
      friend class ConnTimer;
      friend class ModuleSync;
      friend class ModuleAsync;
      friend class ModuleRef;

      protected:

         bool                       fRunState{false};      ///< true if module in the running state
         std::vector<ModuleItem*>   fItems;                ///< map for fast search of module items
         std::vector<InputPort*>    fInputs;               ///< array of input ports
         std::vector<OutputPort*>   fOutputs;              ///< array of output ports
         std::vector<PoolHandle*>   fPools;                ///< array of pools
         std::vector<Timer*>        fTimers;               ///< array of timers
         std::vector<ModuleItem*>   fUsers;                ///< array of user items
         unsigned                   fSysTimerIndex{0};     ///< index of timer, which will be used with module itself
         bool                       fAutoStop{false};      ///< module will automatically stop when all i/o ports will be disconnected
         dabc::Reference            fDfltPool;             ///< direct reference on memory pool, used when no pool handles are not created
         std::string                fInfoParName;          ///< full name of parameter, used as info
         std::string                fPublishPars;          ///< path where module pars will be published

      private:

         Module(const std::string &name, Command cmd);
         virtual ~Module();

         void AddModuleItem(ModuleItem* item);
         void RemoveModuleItem(ModuleItem* item);

         Buffer TakeDfltBuffer();

         inline PoolHandle* Pool(unsigned n = 0) const { return n < fPools.size() ? fPools[n] : nullptr; }
         inline OutputPort* Output(unsigned n = 0) const { return n < fOutputs.size() ? fOutputs[n] : nullptr; }
         inline InputPort* Input(unsigned n = 0) const { return n < fInputs.size() ? fInputs[n] : nullptr; }


      protected:

         /** \brief Inherited method, called during module destroy.
          *
          * Used to stop module if it is still running. */
         void ObjectCleanup() override;

         virtual bool DoStop();
         virtual bool DoStart();

         void OnThreadAssigned() override;
         int PreviewCommand(Command cmd) override;

         virtual void SetModulePriority(int pri = -1);

         void ProcessEvent(const EventId&) override;

         double ProcessTimeout(double last_diff) override;

         bool Find(ConfigIO &) override;

         void BuildFieldsMap(RecordFieldsMap *) override;

         /** \brief Starts execution of the module code */
         bool Start();

         /** \brief Returns true if module if running */
         inline bool IsRunning() const { return fRunState; }

         /** \brief Stops execution of the module code */
         bool Stop();

         /** If set, module will be automatically stopped when all i/o ports are disconnected */
         void SetAutoStop(bool on = true) { fAutoStop = on; }


         /** Creates handle for memory pool, which preserves reference on memory pool
          * and provides fast access to memory pool functionality.
          * Memory pool should exist before handle can be created
          * Returns index of pool handle, which can be used later with operation like TakeBuffer */
         unsigned CreatePoolHandle(const std::string &poolname, unsigned queue = 10);

         unsigned CreateInput(const std::string &name, unsigned queue = 10);
         unsigned CreateOutput(const std::string &name, unsigned queue = 10);

         unsigned CreateTimer(const std::string &name, double period_sec = -1., bool synchron = false);

         unsigned CreateUserItem(const std::string &name);


         /** Method ensure that at least specified number of input and output ports will be created. */
         void EnsurePorts(unsigned numinp = 0, unsigned numout = 0, const std::string &poolname = "");

         /** Bind input and output ports that both will share same connection.
          * In local case ports will be connected to appropriate pair of bind ports from other module */
         bool BindPorts(const std::string &inpname, const std::string &outname);


         // these are new methods, which should be protected
         unsigned NumOutputs() const { return fOutputs.size(); }
         bool IsValidOutput(unsigned indx = 0) const { return indx < fOutputs.size(); }
         bool IsOutputConnected(unsigned indx = 0) const { return indx < fOutputs.size() ? fOutputs[indx]->IsConnected() : false; }
         unsigned OutputQueueCapacity(unsigned indx = 0) const { return indx < fOutputs.size() ? fOutputs[indx]->QueueCapacity() : 0; }
         unsigned FindOutput(const std::string &name) const;
         std::string OutputName(unsigned indx = 0, bool fullname = false) const;

         unsigned NumInputs() const { return fInputs.size(); }
         bool IsValidInput(unsigned indx = 0) const { return indx < fInputs.size(); }
         bool IsInputConnected(unsigned indx = 0) const { return indx < fInputs.size() ? fInputs[indx]->IsConnected() : false; }
         bool InputQueueFull(unsigned indx = 0) const { return indx < fInputs.size() ? fInputs[indx]->QueueFull() : false; }
         unsigned InputQueueCapacity(unsigned indx = 0) const { return indx < fInputs.size() ? fInputs[indx]->QueueCapacity() : 0; }
         unsigned FindInput(const std::string &name) const;
         std::string InputName(unsigned indx = 0, bool fullname = false) const;

         unsigned NumPools() const { return fPools.size(); }
         bool IsValidPool(unsigned indx = 0) const { return indx < fPools.size(); }
         bool IsPoolConnected(unsigned indx = 0) const { return indx < fPools.size() ? fPools[indx]->IsConnected() : false; }
         unsigned FindPool(const std::string &name) const;
         std::string PoolName(unsigned indx = 0, bool fullname = false) const;
         /** Returns true when handle automatically delivers buffers via the connection */
         bool IsAutoPool(unsigned indx = 0) const { return indx<fPools.size() ? fPools[indx]->QueueCapacity() > 0 : false; }


         PortRef FindPort(const std::string &name) const;
         bool IsPortConnected(const std::string &name) const;

         /** Disconnect port from transport. Should be called only from Module thread */
         bool DisconnectPort(const std::string &name, bool witherr = false);

         /** Method disconnects all module ports, should be called only from Module thread */
         void DisconnectAllPorts(bool witherr = false);

         /** Submits command to transport, assigned with the port */
         bool SubmitCommandToTransport(const std::string &portname, Command cmd);

         unsigned PortQueueCapacity(const std::string &name) const { return FindPort(name).QueueCapacity(); }
         bool SetPortSignaling(const std::string &name, Port::EventsProducing signal);
         bool SetPortRatemeter(const std::string &name, const Parameter& ref);
         bool SetPortLoopLength(const std::string &name, unsigned cnt);
         unsigned GetPortLoopLength(const std::string &name);

         unsigned FindTimer(const std::string &name);
         bool IsValidTimer(unsigned indx) const { return indx < fTimers.size(); }
         unsigned NumTimers() const { return fTimers.size(); }
         std::string TimerName(unsigned n = 0, bool fullname = false) const;

         void ShootTimer(unsigned indx, double delay_sec = 0.)
         {
            if (indx < fTimers.size()) {
               if (indx == fSysTimerIndex) ActivateTimeout(delay_sec);
                                      else fTimers[indx]->SingleShoot(delay_sec);
            }
         }

         void ShootTimer(const std::string &name, double delay_sec = 0.)
           {  ShootTimer(FindTimer(name), delay_sec); }

         ModuleItem* GetItem(unsigned id) const { return id < fItems.size() ? fItems[id] : nullptr; }

         unsigned FindUserItem(const std::string &name);
         bool IsValidUserItem(unsigned indx) const { return indx < fUsers.size(); }
         std::string UserItemName(unsigned indx = 0, bool fullname = false) const;
         EventId ConstructUserItemEvent(unsigned indx = 0)
         { return EventId(evntUser, 0, indx < fUsers.size() ? fUsers[indx]->ItemId() : 0); }


         Parameter CreatePar(const std::string &name, const std::string &kind = "") override;

         void SetInfoParName(const std::string &name);


         // TODO: move to respective module classes
         bool CanSendToAllOutputs(bool exclude_disconnected = true) const;
         void SendToAllOutputs(Buffer& buf);


         void ProduceInputEvent(unsigned indx = 0, unsigned cnt = 1);
         void ProduceOutputEvent(unsigned indx = 0, unsigned cnt = 1);
         void ProducePoolEvent(unsigned indx = 0, unsigned cnt = 1);
         void ProduceUserItemEvent(unsigned indx = 0, unsigned cnt = 1);


         // =================== can be reimplemented in derived classes ===============


         // user must redefine method when it want to execute commands.
         // If method return true or false (cmd_true or cmd_false),
         // command considered as executed and will be replied
         // Any other arguments (cmd_postponed) means that execution
         // will be postponed in the module itself
         // User must call Module::ExecuteCommand() to enable processing of standard commands
         int ExecuteCommand(Command cmd)  override { return dabc::Worker::ExecuteCommand(cmd); }


         // This is place of user code when command completion is arrived
         // User can implement any analysis of command data in this method
         // If returns true, command object will be delete automatically,
         // otherwise ownership delegated to user.
         // In that case command must be at some point replied - means user should call cmd.Reply(res) method.
         bool ReplyCommand(Command cmd)  override { return dabc::Worker::ReplyCommand(cmd); }


         // these two methods called before start and after stop of module
         // disregard of module has its own mainloop or not
         virtual void BeforeModuleStart() {}
         virtual void AfterModuleStop() {}
         /** Method, which can be reimplemented by user and should cleanup all references on buffers
          * and other objects */
         virtual void ModuleCleanup() {}

         // generic users event processing function
         virtual void ProcessItemEvent(ModuleItem * /* item */, uint16_t /* evid */) {}

         virtual void ProcessConnectionActivated(const std::string & /* name */, bool /* on */) {}

      public:

         const char *ClassName() const override { return "Module"; }

         std::string GetInfoParName() const;

   };

   // ___________________________________________________________________


   /** \brief %Reference on \ref dabc::Module class
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    * This is main interface for user to access module functionality from outside
    */

   class ModuleRef : public WorkerRef {

      DABC_REFERENCE(ModuleRef, WorkerRef, Module)

      friend class Manager;

      public:

         bool IsRunning() const { return GetObject() ? GetObject()->IsRunning() : false; }

         bool Start() { return Execute("StartModule"); }

         bool Stop() { return Execute("StopModule"); }

         bool CheckConnected() { return Execute("CheckConnected"); }

         /** Returns true if specified input is connected */
         bool IsInputConnected(unsigned ninp);

         /** Returns true if specified output is connected */
         bool IsOutputConnected(unsigned ninp);

         /** Returns number of inputs in the module */
         unsigned NumInputs();

         /** Returns number of outputs in the module */
         unsigned NumOutputs();

         /** Return reference on the port */
         PortRef FindPort(const std::string &name);

         /** Returns true if port with specified name is connected - thread safe */
         bool IsPortConnected(const std::string &name);

         /** Return item name of the input, can be used in connect command */
         std::string InputName(unsigned n = 0, bool itemname = true);

         /** Return item name of the output, can be used in connect command */
         std::string OutputName(unsigned n = 0, bool itemname = true);

         /** Method called by manager to establish connection to pools
          * TODO: while used from devices, made public. Should be protected later again */
         bool ConnectPoolHandles();

         /** \brief Returns info parameter name,
          *  used to provide different kind of log/debug information */
         std::string InfoParName() const
            {  return null() ? std::string() : GetObject()->GetInfoParName(); }

   };

};

#endif
