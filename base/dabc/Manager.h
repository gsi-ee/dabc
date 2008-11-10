#ifndef DABC_Manager
#define DABC_Manager

#ifndef DABC_Folder
#include "dabc/Folder.h"
#endif

#ifndef DABC_CommandClient
#include "dabc/CommandClient.h"
#endif

#ifndef DABC_WorkingProcessor
#include "dabc/WorkingProcessor.h"
#endif

#ifndef DABC_Command
#include "dabc/Command.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

#ifndef DABC_Parameter
#include "dabc/Parameter.h"
#endif


namespace dabc {

   class Mutex;
   class Module;
   class WorkingThread;
   class SocketThread;
   class Port;
   class MemoryPool;
   class Manager;
   class Device;
   class Parameter;
   class CommandDefinition;
   class Application;
   class Factory;
   class DependPairList;
   class LocalDevice;
   class DataInput;
   class DataOutput;
   class FileIO;
   class StateMachineModule;

   class CommandCreateModule : public Command {
      public:
         static const char* CmdName() { return "CreateModule"; }

         CommandCreateModule(const char* classname, const char* modulename, const char* thrdname = 0) :
            Command(CmdName())
            {
               SetArguments(this, classname, modulename, thrdname);
            }

         static void SetArguments(Command* cmd, const char* classname, const char* modulename, const char* thrdname = 0)
         {
            cmd->SetStr("Class", classname);
            cmd->SetStr("Name", modulename);
            cmd->SetStr("Thread", thrdname);
         }
   };

   class CmdCreatePool : public Command {
      public:
         static const char* CmdName() { return "CreatePool"; }

         CmdCreatePool(const char* name, unsigned totalsize, unsigned buffersize, unsigned headersize = 0) :
            Command(CmdName())
         {
            SetPar("PoolName", name);
            SetUInt("NumBuffers", buffersize>0 ? totalsize / buffersize : 1);
            SetUInt("BufferSize", buffersize);
            SetUInt("HeaderSize", headersize);
         }
   };

   class CmdDeletePool : public Command {
      public:
         static const char* CmdName() { return "DeletePool"; }
         CmdDeletePool(const char* name) :
            Command(CmdName())
         {
            SetStr("PoolName", name);
         }
   };

   class CommandStartModule : public Command {
      public:
         CommandStartModule(const char* modulename) :
            Command("StartModule")
         {
            SetPar("Module", modulename);
         }
   };

   class CommandStopModule : public Command {
      public:
         CommandStopModule(const char* modulename) :
            Command("StopModule")
         {
            SetPar("Module", modulename);
         }
   };

   class CommandDeleteModule : public Command {
      public:
         CommandDeleteModule(const char* modulename) :
            Command("DeleteModule")
         {
            SetPar("Module", modulename);
         }
   };

   class CmdStartAllModules : public Command {
      public:
         CmdStartAllModules(int appid = 0) :
            Command("StartAllModules")
         {
            SetInt("AppId", appid);
         }
   };

   class CmdStopAllModules : public Command {
      public:
         CmdStopAllModules(int appid = 0) :
            Command("StopAllModules")
         {
            SetInt("AppId", appid);
         }
   };

   class CmdCleanupManager : public Command {
      public:
         CmdCleanupManager(int appid = 0) :
            Command("CleanupManager")
         {
            SetInt("AppId", appid);
         }
   };

   class CmdCreateApplication : public Command {
      public:
         static const char* CmdName() { return "CreateApplication"; }

         CmdCreateApplication(const char* appclass, const char* appname = 0, const char* appthrd = 0) :
            Command(CmdName())
         {
            SetArguments(this, appclass, appname, appthrd);
         }

         static void SetArguments(Command* cmd, const char* appclass, const char* appname, const char* appthrd)
         {
            cmd->SetStr("AppClass", appclass);
            cmd->SetStr("AppName", appname);
            cmd->SetStr("AppThrd", appthrd);
         }
     };


   class CmdCreateDevice : public Command {
      public:
         static const char* CmdName() { return "CreateDevice"; }

         CmdCreateDevice(const char* devclass, const char* devname) :
            Command(CmdName())
         {
            SetArguments(this, devclass, devname);
         }

         static void SetArguments(Command* cmd, const char* devclass, const char* devname)
         {
            cmd->SetStr("DevClass", devclass);
            cmd->SetStr("DevName", devname);
         }
   };

   class CmdCreateThread : public Command {
      public:
         static const char* CmdName() { return "CreateThread"; }

         CmdCreateThread(const char* thrdname, const char* thrdclass = 0,  const char* devname = 0) :
            Command(CmdName())
         {
            SetArguments(this, thrdname, thrdclass, devname);
         }

         static void SetArguments(Command* cmd, const char* thrdname, const char* thrdclass = 0, const char* devname = 0)
         {
            cmd->SetStr("ThrdName", thrdname);
            cmd->SetStr("ThrdClass", thrdclass);
            cmd->SetStr("ThrdDev", devname);
         }
   };

   class CmdCreateTransport : public Command {
      public:
         static const char* CmdName() { return "CreateTransport"; }

         CmdCreateTransport() : Command(CmdName()) {}

         CmdCreateTransport(const char* portname) :
            Command(CmdName())
         {
            SetArguments(this, portname);
         }

         static void SetArguments(Command* cmd, const char* portname)
         {
            cmd->SetPar("PortName", portname);
         }
   };

   class CommandPortConnect: public Command {
      public:
         static const char* CmdName() { return "PortConnect"; }

         CommandPortConnect(const char* port1fullname,
                            const char* port2fullname,
                            const char* device = 0,
                            const char* trthread = 0) :
            Command(CmdName())
            {
               SetPar("Port1Name", port1fullname);
               SetPar("Port2Name", port2fullname);
               SetPar("Device", device);
               SetPar("TrThread", trthread);
            }
   };

   class CommandDirectConnect : public CmdCreateTransport {
      public:
         CommandDirectConnect(bool isserver, const char* portname, bool immidiate_reply = false) :
            CmdCreateTransport(portname)
            {
               SetBool("IsServer", isserver);
               SetBool("ImmidiateReply", immidiate_reply);
            }
   };

   class CommandSetParameter : public Command {
      public:
         static const char* CmdName() { return "SetParameter"; }


         CommandSetParameter(const char* parname, const char* value) :
            Command(CmdName())
            {
                SetPar("ParName", parname);
                SetPar("ParValue", value);
            }
         CommandSetParameter(const char* parname, int value) :
            Command(CmdName())
            {
                SetPar("ParName", parname);
                SetInt("ParValue", value);
            }
   };

   class CmdCreateDataTransport : public Command {
      public:
         static const char* CmdName() { return "CreateDataTransport"; }

         CmdCreateDataTransport() : Command(CmdName()) {}

         CmdCreateDataTransport(const char* portname,
                                const char* thrdname = 0) :
            Command(CmdName())
         {
            SetArgs(this, portname, thrdname);
         }

         static void SetArgs(Command* cmd,
                             const char* portname,
                             const char* thrdname = 0)
         {
            cmd->SetPar("PortName", portname);
            cmd->SetPar("ThrdName", thrdname);
         }

         static void SetIOTyp(Command* cmd,
                              const char* iotyp)
         {
            cmd->SetPar("IOTyp", iotyp);
         }

         static const char* GetIOTyp(Command* cmd)
         {
            return cmd->GetPar("IOTyp");
         }

         static void SetArgsInp(Command* cmd,
                                const char* inp_typ,
                                const char* inp_name)
         {
            cmd->SetPar("InpType", inp_typ);
            cmd->SetPar("InpName", inp_name);
         }

         static void SetArgsOut(Command* cmd,
                                const char* out_typ,
                                const char* out_name)
         {
            cmd->SetPar("OutType", out_typ);
            cmd->SetPar("OutName", out_name);
         }
   };

   class CommandStateTransition : public Command {
      public:
         static const char* CmdName() { return "StateTransition"; }

         CommandStateTransition(const char* state_transition_cmd) :
            Command(CmdName())
            {
               SetStr("Cmd", state_transition_cmd);
            }
   };

   template<class T>
   class CleanupEnvelope : public Basic {
      protected:
         T* fObj;
      public:
         CleanupEnvelope(T* obj) : Basic(0, "noname"), fObj(obj) {}
         virtual ~CleanupEnvelope() { delete fObj; }
   };


   class Manager : public Folder,
                   public WorkingProcessor,
                   public CommandClientBase {

      friend class Basic;
      friend class Factory;
      friend class Parameter;
      friend class CommandDefinition;

      protected:

         void ObjectDestroyed(Basic* obj);

         const char* ExtractManagerName(const char* fullname, std::string& managername);

         void ChangeManagerName(const char* newname);

         enum MgrEvents { evntDestroyObj = evntFirstUser, evntManagerReply };

      public:

         Manager(const char* managername, bool usecurrentprocess = false);
         virtual ~Manager();

         static Manager* Instance() { return fInstance; }

          // candidates for protected

         /** Delete all modules and stop manager thread.
           * Normally, last command before exit from main program.
           * Automatically called from destructor */
         void HaltManager();

         /** Check if transition allowed */
         bool IsStateTransitionAllowed(const char* state_transition_cmd, bool errout = false);

         /** Perform action to makes required state transition
           * Should not be called from manager thread while
           * it is synchron and returns only when transition is completed (true) or
           * error is detected (false) */
         bool DoStateTransition(const char* state_transition_cmd);


         // ------------------------- State machine constants and methods ----------------------

         static const char* stParName; // name of manager parameter, where current state is stored

         static const char* stNull;       // no connection to state machine
         static const char* stHalted;
         static const char* stConfigured;
         static const char* stReady;
         static const char* stRunning;
         static const char* stFailure; // failure during state transition
         static const char* stError;   // error after state transition is completed

         static const char* stcmdDoConfigure;
         static const char* stcmdDoEnable;
         static const char* stcmdDoStart;
         static const char* stcmdDoStop;
         static const char* stcmdDoError;
         static const char* stcmdDoHalt;

         static const char* TargetStateName(const char* stcmd);

         /** Invoke state transition of manager.
           * Must be overwritten in derived class.
           * This MUST be asynhron functions means calling thread should not be blocked.
           * Actual state transition will be performed in state-machine thread.
           * If command object is specified, it will be replyed when state transition is
           * completed or when transition is failed */
         virtual bool InvokeStateTransition(const char* state_transition_name, Command* cmd = 0);

         /** Returns curren state name */
         const char* CurrentState() const;


         // -------------- generic folder structure of manager

         static const char* ThreadsFolderName() { return "Threads"; }
         static const char* ModulesFolderName() { return "Modules"; }
         static const char* DevicesFolderName() { return "Devices"; }
         static const char* PluginFolderName()  { return "Plugin"; }
         static const char* FactoriesFolderName() { return "Factories"; }
         static const char* PoolsFolderName()   { return "Pools"; }
         static const char* LocalDeviceName()   { return "local"; }

         static const char* MgrThrdName()       { return "ManagerThrd"; }

         Folder* GetFactoriesFolder(bool force = false) { return GetFolder(FactoriesFolderName(), force, false); }
         Folder* GetAppFolder(bool force = false) { return GetFolder(PluginFolderName(), force, true); }
         Folder* GetDevicesFolder(bool force = false) { return GetFolder(DevicesFolderName(), force, true); }
         Folder* GetThreadsFolder(bool force = false) { return GetFolder(ThreadsFolderName(), force, true); }
         Folder* GetModulesFolder(bool force = false) { return GetFolder(ModulesFolderName(), force, true); }
         Folder* GetPoolsFolder(bool force = false) { return GetFolder(PoolsFolderName(), force, true); }

         Module* FindModule(const char* name);
         Port* FindPort(const char* name);
         MemoryPool* FindPool(const char* name);
         Factory* FindFactory(const char* name);
         Device* FindDevice(const char* name);
         WorkingThread* FindThread(const char* name, const char* required_class = 0);
         LocalDevice* FindLocalDevice(const char* name = 0);
         Application* GetApp();

         // ------------------ threads manipulation ------------------

         /** Create thread for processor and assigns processor to this thread
           * Thread name must be specified */
         bool MakeThreadFor(WorkingProcessor* proc, const char* thrdname = 0, unsigned startmode = 0);

         /** Create thread for module and assigns module to this thread.
           * If thread name is not specified, module name is used */
         bool MakeThreadForModule(Module* m, const char* thrdname = 0);

         const char* CurrentThrdName();

         void RunManagerMainLoop();

         // ---------------- modules manipulation ------------------

         void StartModule(const char* modulename);
         void StopModule(const char* modulename);
         bool StartAllModules(int appid = 0);
         bool StopAllModules(int appid = 0);
         bool DeleteModule(const char* name);
         bool IsModuleRunning(const char* name);
         bool IsAnyModuleRunning();

         bool ConnectPorts(const char* port1name,
                           const char* port2name,
                           const char* devname = 0);


         // ----------- memory pools creation/deletion -------------------

         /** Generic method to create memory pool.
           * Creates (or extends) memory pool with numbuffers buffers of size buffersize.
           * Together with buffers memory pool creates number of reference objects with
           * preallocated header and gather list.
           * One can configure that memory pool can be extended "on the fly" -
           * numincrement value specifies how much buffers memory pool can extend at once.
           * In case when expanding of pool is allowed, one can limit total size
           * of pool via ConfigurePool method. There one can also specify how often
           * memory pool should try to cleanup unused memory.*/
         MemoryPool* CreateMemoryPool(const char* poolname,
                                      unsigned buffersize,
                                      unsigned numbuffers,
                                      unsigned numincrement = 0,
                                      unsigned headersize = 0x20,
                                      unsigned numsegments = 8);

         /** This method create memory pools on the base of values,
           * configured in the newly created modules. */
         bool CreateMemoryPools();

         /** Set pools configuration.
           * fixlayout = true means memory pool cannot be increased/decreased automatically,
           * size_limit - maximum size of memory pool
           * cleanup_timeout - time in seconds, after which pool will delete unused buffers */
         MemoryPool* ConfigurePool(const char* poolname,
                                   bool fixlayout = false,
                                   uint64_t size_limit = 0,
                                   double cleanup_timeout = -1.);

         /** Delete memory pool */
         bool DeletePool(const char* name);


         // ----------- commands submission -------------------

         // next methods prepare commands arguments so, that
         // they can be directly submitted to the maneger via submit
         // for instance m.Submit(m.LocalCmd(new Command("Start"), "Modules/Generator"));
         // This queues commands first in manager queue and than submitted to sepcified
         // object. If object has own thread, it will be used for command execution

         Command* LocalCmd(Command* cmd, const char* fullitemname = "");

         Command* LocalCmd(Command* cmd, Basic* rcv);

         Command* RemoteCmd(Command* cmd, const char* nodename, const char* itemname = "");

         Command* RemoteCmd(Command* cmd, int nodeid, const char* itemname = "");

         bool SubmitLocal(CommandClientBase& cli, Command* cmd, const char* fullitemname = "")
            { return SubmitCl(cli, LocalCmd(cmd, fullitemname)); }

         bool SubmitLocal(CommandClientBase& cli, Command* cmd, Basic* rcv)
           { return SubmitCl(cli, LocalCmd(cmd, rcv)); }

         bool SubmitRemote(CommandClientBase& cli, Command* cmd, const char* nodename, const char* itemname = "")
           { return SubmitCl(cli, RemoteCmd(cmd, nodename, itemname)); }

         bool SubmitRemote(CommandClientBase& cli, Command* cmd, int nodeid, const char* itemname = "")
           { return SubmitCl(cli, RemoteCmd(cmd, nodeid, itemname)); }


         // ---------------- interface to control system -------------

         /** indicate if manager play central role in the system */
         virtual bool IsMainManager() { return true; }

         /** Return nodes id of local node */
         virtual int NodeId() const { return 0; }

         /** Indicate, if manager has information about cluster */
         virtual bool HasClusterInfo() { return false; };
         /** Returns number of nodes in the cluster */
         virtual int NumNodes() { return 1; }
         /** Return name of node */
         virtual const char* GetNodeName(int nodeid) { return GetName(); }
         /** Returns id of the node, -1 if error */
         int DefineNodeId(const char* nodename);
         /** Returns true if node with specified id is active */
         virtual bool IsNodeActive(int num) { return num==0; }
         /** Returns number of currently active nodes */
         int NumActiveNodes();

         // Subscribe/unsubscribe parameter against remote (local)
         virtual bool Subscribe(Parameter* par, int remnode, const char* remname) { return false; }
         virtual bool Unsubscribe(Parameter* par) { return false; }

         virtual Basic* GetParsHolder() { return this; }

         // -------------------------- misc functions ---------------

         /** Displays on std output list of running threads and modules */
         void Print();

         /** Delete deriver from Basic class object in manager thread.
           * Usefull as replasement of call "delete this;" */
         virtual void DestroyObject(Basic* obj);

         /** Delete of any kind of object in manager thread */
         template<class T> void DeleteAnyObject(T* obj)
         {
             DestroyObject(new CleanupEnvelope<T>(obj));
         }

         /** Register/unregister dependency between objects
           * One use dependency to detect situation when dependent (tgt) object is destroyed.
           * In this case virtual DependendDestroyed() method of src object will be called.
           * Normally one should "forget" pointer on dependent object at this moment. */
         bool RegisterDependency(Basic* src, Basic* tgt);
         bool UnregisterDependency(Basic* src, Basic* tgt);

         /** Method safely deletes all modules, memory pools and devices with
           * specified application id. appid==0 is default id for all user components.
           * In the end all unused thread also destroyed */
         bool CleanupManager(int appid = 0);

         static bool LoadLibrary(const char* libname);
         static unsigned Read_XDAQ_XML_NumNodes(const char* fname);
         static String Read_XDAQ_XML_NodeName(const char* fname, unsigned cnt = 0);
         bool Read_XDAQ_XML_Libs(const char* fname, unsigned cnt = 0);
         bool Read_XDAQ_XML_Pars(const char* fname, unsigned cnt = 0);

         bool InstallCtrlCHandler();
         void ProcessCtrlCSignal();
         void RaiseCtrlCSignal();

         // ------------ access to factories method -------------

         bool CreateApplication(const char* classname = 0, const char* appname = 0, const char* appthrd = 0, Command* cmd = 0);

         bool CreateDevice(const char* classname, const char* devname, Command* cmd = 0);

         WorkingThread* CreateThread(const char* thrdname, const char* classname = 0, unsigned startmode = 0, const char* devname = 0, Command* cmd = 0);

         bool CreateModule(const char* classname, const char* modulename, const char* thrdname = 0, Command* cmd = 0);

         bool CreateTransport(const char* devicename, const char* portname, Command* cmd = 0);

         bool CreateDataInputTransport(const char* portname, const char* thrdname,
                                       const char* typ, const char* name,
                                       Command* cmd = 0);

         bool CreateDataOutputTransport(const char* portname, const char* thrdname,
                                        const char* typ, const char* name,
                                        Command* cmd = 0);

         bool CreateDataIOTransport(const char* portname, const char* thrdname,
                                    const char* inp_typ, const char* inp_name,
                                    const char* out_typ, const char* out_name,
                                    Command* cmd = 0);

         FileIO* CreateFileIO(const char* typ, const char* name, int option);

         Folder* ListMatchFiles(const char* typ, const char* filemask);

         DataInput* CreateDataInput(const char* typ, const char* name, Command* cmd = 0);

         DataOutput* CreateDataOutput(const char* typ, const char* name, Command* cmd = 0);

      protected:
         bool                  fMgrMainLoop; // flag indicates if mainloop of manager should runs

         Mutex                *fMgrMutex; // main mutex to protect manager queues
         CommandsQueue         fReplyesQueue;
         Queue<Basic*>         fDestroyQueue;

         Mutex                *fSendCmdsMutex;
         int                   fSendCmdCounter;
         PointersVector        fSendCommands;

         PointersVector        fTimedPars;

         DependPairList       *fDepend; // list of objects dependencies

         Thread_t              fSigThrd;

         StateMachineModule   *fSMmodule;

         static Manager       *fInstance;

         virtual bool _ProcessReply(Command* cmd);
         virtual double ProcessTimeout(double last_diff);

         bool DoCreateMemoryPools();
         bool DoLocalPortConnect(const char* port1name, const char* port2name, const char* devname = 0);
         void DoCleanupThreads();
         void DoCleanupDevices(bool force);
         bool DoCleanupManager(int appid);
         void DoHaltManager();
         void DoPrint();

         virtual int PreviewCommand(Command* cmd);
         virtual int ExecuteCommand(Command* cmd);

         virtual bool PostCommandProcess(Command*);

         int AddInternalCmd(Command* cmd, const char* lblname);
         Command* FindInternalCmd(const char* lblname, int id);
         Command* TakeInternalCmd(const char* lblname, int id);

         void ProcessDestroyQueue();

         virtual void ProcessEvent(uint64_t evid);

         // virtual method to deliver some events to control system
         virtual void ModuleExecption(Module* m, const char* msg);
         virtual void ParameterEvent(Parameter* par, int event);
         virtual void CommandRegistration(Module* m, CommandDefinition* def, bool reg) {}

         // methods, used for remote command execution
         virtual bool CanSendCmdToManager(const char*) { return false; }
         virtual bool SendOverCommandChannel(const char* managername, const char* cmddata);
         void RecvOverCommandChannel(const char* cmddata);

         static void* FindXmlContext(void* engine, void* doc, unsigned cnt = 0, const char* context = 0, bool showerr = true);

         void InitSMmodule();

         // must be called in inherited class constructor & destructor
         void init();
         void destroy();

      private:
         // this method is used from Factory to register factory when it created
         void AddFactory(Factory* factory);
   };

   inline dabc::Manager* mgr() { return dabc::Manager::Instance(); }

}

#endif
