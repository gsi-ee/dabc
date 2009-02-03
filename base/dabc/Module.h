#ifndef DABC_Module
#define DABC_Module

#ifndef DABC_Folder
#include "dabc/Folder.h"
#endif

#ifndef DABC_WorkingProcessor
#include "dabc/WorkingProcessor.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

#ifndef DABC_Command
#include "dabc/Command.h"
#endif

#ifndef DABC_CommandClient
#include "dabc/CommandClient.h"
#endif

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

#ifndef DABC_Parameter
#include "dabc/Parameter.h"
#endif

namespace dabc {

   class PoolHandle;
   class ModuleItem;
   class Manager;
   class Port;
   class Timer;
   class Transport;

   class Module : public Folder,
                  public WorkingProcessor,
                  public CommandClientBase {

      friend class Manager;
      friend class ModuleItem;
      friend class Port;

      enum  { evntReinjectlost = evntFirstUser, evntReplyCommand };

      protected:
         typedef std::vector<unsigned> ItemsIndexVector;

         enum EModuleRunState { msHalted, msStopped, msRunning };

         EModuleRunState          fRunState;    // module running state
         PointersVector           fItems;       // map for fast search of module items
         ItemsIndexVector         fInputPorts;  // map for fast access to input ports
         ItemsIndexVector         fOutputPorts; // map for fast access to output ports
         ItemsIndexVector         fPorts;       // map for fast access to i/o ports
         ItemsIndexVector         fPoolHandels; // map for fast access to memory pools handles
         CommandsQueue            fReplyes;     // reply queue
         Queue<EventId>           fLostEvents;  // events, coming while module is sleeping

      protected:

         Module(const char* name = "module", Command* cmd = 0);

      public:
         virtual ~Module();

         // this methods can be used from outside of module

         void Start();
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

         Folder* GetTimersFolder(bool force = false);

         ModuleItem* GetItem(unsigned id) const { return id<fItems.size() ? (ModuleItem*) fItems.at(id) : 0; }

         inline bool IsRunning() const { return fRunState == msRunning; }
         inline bool IsStopped() const { return fRunState == msStopped; }
         inline bool IsHalted() const { return fRunState == msHalted; }

         virtual CommandReceiver* GetCmdReceiver() { return this; }

         // generic event processing function
         virtual void ProcessUserEvent(ModuleItem* item, uint16_t evid) {}

         // some useful routines for I/O handling
         bool CanSendToAllOutputs() const;
         void SendToAllOutputs(Buffer* buf);

         virtual const char* MasterClassName() const { return "Module"; }
         virtual const char* ClassName() const { return "Module"; }
         virtual bool UseMasterClassName() const { return true; }

      protected:

         // these two methods called before start and after stop of module
         // disregard of module has its own mainloop or not
         virtual void BeforeModuleStart() {}
         virtual void AfterModuleStop() {}

         // user must redefine method when it want to execute commands.
         // If method return true or false (cmd_true or cmd_false),
         // command considered as executed and will be replyed
         // Any other arguments (cmd_postponed) means that execution
         // will be postponed in the module itself
         // User must call Module::ExecuteCommand() to enable processing of standard commands
         virtual int ExecuteCommand(Command* cmd);

         // This is place of user code when command completion is arrived
         // User can implement any analysis of command data in this method
         // If returns true, command object will be delete automatically,
         // otherwise ownership delegated to user.
         virtual bool ReplyCommand(Command* cmd) { return true; }

         // =======================================================

         PoolHandle* CreatePoolHandle(const char* poolname, BufferSize_t size = 0, BufferNum_t number = 0, BufferNum_t increment = 0);

         Port* CreateInput(const char* name, PoolHandle* pool = 0, unsigned queue = 10, BufferSize_t headersize = 0)
           { return CreateIOPort(name, pool, queue, 0, headersize); }
         Port* CreateOutput(const char* name, PoolHandle* pool = 0, unsigned queue = 10, BufferSize_t headersize = 0)
           { return CreateIOPort(name, pool, 0, queue, headersize); }
         Port* CreateIOPort(const char* name, PoolHandle* pool = 0, unsigned recvqueue = 10, unsigned sendqueue = 10, BufferSize_t headersize = 0);

         RateParameter* CreateRateParameter(const char* name, bool sync = true, double interval = 1., const char* inpportname = 0, const char* outportname = 0);

         Parameter* CreatePoolUsageParameter(const char* name, double interval = 1., const char* poolname = 0);

         Timer* CreateTimer(const char* name, double period_sec = 1., bool synchron = false);
         Timer* FindTimer(const char* name);
         bool ShootTimer(const char* name, double delay_sec = 0.);

         Port* GetPortItem(unsigned id) const;

         virtual bool _ProcessReply(Command* cmd);

         TimeStamp_t ThrdTimeStamp() { return ProcessorThread() ? ProcessorThread()->ThrdTimeStamp() : NullTimeStamp; }
         TimeSource* GetThrdTimeSource() { return ProcessorThread() ? ProcessorThread()->ThrdTimeSource() : 0; }

         void GetUserEvent(ModuleItem* item, uint16_t evid);

         virtual void ProcessEvent(EventId evid);

         bool DoStop();
         bool DoStart();

         virtual void OnThreadAssigned();
         virtual int PreviewCommand(Command* cmd);

         virtual WorkingProcessor* GetCfgMaster();

       private:

         void ItemCreated(ModuleItem* item);
         void ItemDestroyed(ModuleItem* item);
   };

};

#endif
