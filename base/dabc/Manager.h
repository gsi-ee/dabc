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
   class DataInput;
   class DataOutput;
   class FileIO;
   class StateMachineModule;
   class Configuration;

   class CmdCreateModule : public Command {
      public:
         static const char* CmdName() { return "CreateModule"; }

         CmdCreateModule(const char* classname, const char* modulename, const char* thrdname = 0) :
            Command(CmdName())
            {
               SetStr("Class", classname);
               SetStr("Name", modulename);
               SetStr("Thread", thrdname);
            }
   };

   class CmdDeletePool : public Command {
      public:
         static const char* CmdName() { return "DeletePool"; }

         CmdDeletePool(const char* name) : Command(CmdName())
            { SetStr("PoolName", name); }
   };

   class CmdCheckConnModule : public Command {
      public:
         static const char* CmdName() { return "CheckConn"; }

         CmdCheckConnModule(const char* modulename) : Command(CmdName())
            { SetPar("Module", modulename); }
   };

   class CmdStartModule : public Command {
      public:
         static const char* CmdName() { return "StartModule"; }

         CmdStartModule(const char* modulename) : Command(CmdName())
            { SetPar("Module", modulename); }
   };

   class CmdStopModule : public Command {
      public:
         static const char* CmdName() { return "StopModule"; }

         CmdStopModule(const char* modulename) : Command(CmdName())
           { SetPar("Module", modulename); }
   };

   class CmdDeleteModule : public Command {
      public:
         static const char* CmdName() { return "DeleteModule"; }

         CmdDeleteModule(const char* modulename) : Command(CmdName())
            { SetPar("Module", modulename); }
   };

   class CmdStartAllModules : public Command {
      public:
         static const char* CmdName() { return "StartAllModules"; }

         CmdStartAllModules(int appid = 0) : Command(CmdName())
            { SetInt("AppId", appid); }
   };

   class CmdStopAllModules : public Command {
      public:
         static const char* CmdName() { return "StopAllModules"; }

         CmdStopAllModules(int appid = 0) : Command(CmdName())
            { SetInt("AppId", appid); }
   };

   class CmdCleanupManager : public Command {
      public:
         static const char* CmdName() { return "CleanupManager"; }

         CmdCleanupManager(int appid = 0) : Command(CmdName())
            { SetInt("AppId", appid); }
   };

   class CmdCreateApplication : public Command {
      public:
         static const char* CmdName() { return "CreateApplication"; }

         CmdCreateApplication(const char* appclass, const char* appthrd = 0) :
            Command(CmdName())
         {
            SetStr("AppClass", appclass);
            SetStr("AppThrd", appthrd);
         }
     };


   class CmdCreateDevice : public Command {
      public:
         static const char* CmdName() { return "CreateDevice"; }

         CmdCreateDevice(const char* devclass, const char* devname, const char* thrdname = 0) :
            Command(CmdName())
         {
            SetStr("DevClass", devclass);
            SetStr("DevName", devname);
            SetPar("Thread", thrdname);
         }
   };

   class CmdCreateThread : public Command {
      public:
         static const char* CmdName() { return "CreateThread"; }

         CmdCreateThread(const char* thrdname, const char* thrdclass = 0,  const char* devname = 0) :
            Command(CmdName())
         {
            SetStr("ThrdName", thrdname);
            SetStr("ThrdClass", thrdclass);
            SetStr("ThrdDev", devname);
         }
   };

   class CmdCreateTransport : public Command {
      public:
         static const char* CmdName() { return "CreateTransport"; }

         CmdCreateTransport(const char* portname, const char* transportkind, const char* thrdname = 0) :
            Command(CmdName())
         {
            SetPar("PortName", portname);
            SetPar("TransportKind", transportkind);
            SetPar("TransportThrdName", thrdname);
         }
   };

   class CmdConnectPorts: public Command {
      public:
         static const char* CmdName() { return "ConnectPorts"; }

         CmdConnectPorts(const char* port1fullname,
                         const char* port2fullname,
                         const char* device = 0,
                         const char* trthread = 0) :
            Command(CmdName())
         {
            SetPar("Port1Name", port1fullname);
            SetPar("Port2Name", port2fullname);
            SetPar("Device", device);
            SetPar(xmlTrThread, trthread);
         }
   };

   class CmdDirectConnect : public Command {
      public:
         static const char* CmdName() { return "DirectConnect"; }

         CmdDirectConnect(bool isserver, const char* portname, bool immidiate_reply = false) :
            Command(CmdName())
         {
            SetPar("PortName", portname);
            SetBool("IsServer", isserver);
            SetBool("ImmidiateReply", immidiate_reply);
         }
   };

   class CmdSetParameter : public Command {
      public:
         static const char* CmdName() { return "SetParameter"; }

         CmdSetParameter(const char* parname, const char* value) :
            Command(CmdName())
         {
            SetPar("ParName", parname);
            SetPar("ParValue", value);
         }

         CmdSetParameter(const char* parname, int value) :
            Command(CmdName())
         {
            SetPar("ParName", parname);
            SetInt("ParValue", value);
         }

         CmdSetParameter(const char* parname, bool value) :
            Command(CmdName())
         {
            SetPar("ParName", parname);
            SetBool("ParValue", value);
         }

   };

   class CmdStateTransition : public Command {
      public:
         static const char* CmdName() { return "StateTransition"; }

         CmdStateTransition(const char* state_transition_cmd) :
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

         enum MgrEvents { evntDestroyObj = evntFirstUser, evntManagerReply, evntManagerParam };

      public:

         Manager(const char* managername, bool usecurrentprocess = false, Configuration* cfg = 0);
         virtual ~Manager();

         static Manager* Instance() { return fInstance; }

          // candidates for protected

         /** Delete all modules and stop manager thread.
           * Normally, last command before exit from main program.
           * Automatically called from destructor */
         void HaltManager();

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
           * This MUST be asynchronous functions means calling thread should not be blocked.
           * Actual state transition will be performed in state-machine thread.
           * If command object is specified, it will be replied when state transition is
           * completed or when transition is failed */
         virtual bool InvokeStateTransition(const char* state_transition_name, Command* cmd = 0);

         /** Performs change of state */
         bool ChangeState(const char* state_transition_cmd, double tmout = 10.);

         /** Check if transition allowed */
         bool IsStateTransitionAllowed(const char* state_transition_cmd, bool errout = false);

         /** Perform action to makes required state transition
           * Should not be called from manager thread while
           * it is synchronous and returns only when transition is completed (true) or
           * error is detected (false) */
         bool DoStateTransition(const char* state_transition_cmd);

         /** Returns current state name */
         std::string CurrentState() const;

         // -------------- generic folder structure of manager

         static const char* ThreadsFolderName() { return "Threads"; }
         static const char* DevicesFolderName() { return "Devices"; }
         static const char* FactoriesFolderName() { return "Factories"; }
         static const char* LocalDeviceName()   { return "local"; }

         static const char* MgrThrdName()       { return "ManagerThrd"; }

         Folder* GetFactoriesFolder(bool force = false) { return GetFolder(FactoriesFolderName(), force, false); }
         Folder* GetDevicesFolder(bool force = false) { return GetFolder(DevicesFolderName(), force, true); }
         Folder* GetThreadsFolder(bool force = false) { return GetFolder(ThreadsFolderName(), force, true); }

         Module* FindModule(const char* name);
         Port* FindPort(const char* name);
         Factory* FindFactory(const char* name);
         Device* FindDevice(const char* name);
         WorkingThread* FindThread(const char* name, const char* required_class = 0);
         Device* FindLocalDevice(const char* name = 0);
         Application* GetApp();

         // ------------------ threads manipulation ------------------

         /** Create thread for processor and assigns processor to this thread
           * Thread name must be specified */
         bool MakeThreadFor(WorkingProcessor* proc, const char* thrdname = 0, unsigned startmode = 0);

         /** Create thread for module and assigns module to this thread.
           * If thread name is not specified, module name is used */
         bool MakeThreadForModule(Module* m, const char* thrdname = 0);

         std::string MakeThreadName(const char* base = "Thread");

         WorkingThread* CurrentThread();

         const char* CurrentThrdName();

         void RunManagerMainLoop(int runtime = 0);

         void Sleep(double tmout);

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
         bool CreateMemoryPool(const char* poolname,
                               unsigned buffersize = 0,
                               unsigned numbuffers = 0,
                               unsigned numincrement = 0,
                               unsigned headersize = 0x20,
                               unsigned numsegments = 0);

         MemoryPool* FindPool(const char* name);

         /** Delete memory pool */
         bool DeletePool(const char* name);

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
         /** Test active nodes - try to execute simpe ping command on each active node */
         bool TestActiveNodes(double tmout = 5.);
         /** Establish/test connection to control system */
         virtual bool ConnectControl(const char* connid) { return true; }

         // Subscribe/unsubscribe parameter against remote (local)
         virtual bool Subscribe(Parameter* par, int remnode, const char* remname) { return false; }
         virtual bool Unsubscribe(Parameter* par) { return false; }

         /** Provide log message to control system to dysplay on GUI */
         virtual void LogMessage(int, const char*) {}

         // -------------------------- misc functions ---------------

         Configuration* cfg() const { return fCfg; }

         /** Displays on std output list of running threads and modules */
         void Print();

         /** Delete deriver from Basic class object in manager thread.
           * Useful as replacement of call "delete this;" */
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

         std::string BuildControlName(Basic* obj);
         Basic* FindControlObject(const char* name);

         bool InstallCtrlCHandler();
         void ProcessCtrlCSignal();
         void RaiseCtrlCSignal();

         virtual bool Store(ConfigIO &cfg);
         virtual bool Find(ConfigIO &cfg);

         bool FindInConfiguration(Folder* fold, const char* itemname);

         // ------------ access to factories method -------------

         bool CreateApplication(const char* classname = 0, const char* appthrd = 0);

         bool CreateDevice(const char* classname, const char* devname, const char* devthrd = 0);

         WorkingThread* CreateThread(const char* thrdname, const char* classname = 0, unsigned startmode = 0, const char* devname = 0, Command* cmd = 0);

         bool CreateModule(const char* classname, const char* modulename, const char* thrdname = 0);

         bool CreateTransport(const char* portname, const char* transportkind, const char* thrdname = 0);

         FileIO* CreateFileIO(const char* typ, const char* name, int option);

         Folder* ListMatchFiles(const char* typ, const char* filemask);

      protected:

         struct ParamRec {
            Parameter* par;
            int event;
            bool processed;

            ParamRec() : par(0), event(0), processed(false) {}
            ParamRec(Parameter* p, int e) : par(p), event(e), processed(false) {}
            ParamRec(const ParamRec& src) : par(src.par), event(src.event), processed(src.processed) {}
         };

         bool                  fMgrMainLoop; // flag indicates if mainloop of manager runs
         bool                  fMgrStopped; // indicate if manager mainloop is stopped
         bool                  fMgrNormalThrd; // indicate if manager has normal thread

         Mutex                *fMgrMutex; // main mutex to protect manager queues
         CommandsQueue         fReplyesQueue;
         Queue<Basic*>         fDestroyQueue;
         Queue<ParamRec>       fParsQueue;
         bool                  fParsQueueBlocked;

         Mutex                *fSendCmdsMutex;
         int                   fSendCmdCounter;
         PointersVector        fSendCommands;

         PointersVector        fTimedPars;

         DependPairList       *fDepend; // list of objects dependencies

         Thread_t              fSigThrd;

         StateMachineModule   *fSMmodule;

         Configuration        *fCfg;
         std::string           fCfgHost;

         static Manager       *fInstance;

         virtual bool _ProcessReply(Command* cmd);
         virtual double ProcessTimeout(double last_diff);

         bool DoCreateMemoryPool(Command* cmd);

         bool DoDeleteAllModules(int appid = -1);
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
         void ProcessParameterEvent();

         virtual void ProcessEvent(uint64_t evid);

         // virtual method to deliver some events to control system
         virtual void ModuleExecption(Module* m, const char* msg);
         virtual void ParameterEvent(Parameter* par, int event) {}
         virtual void CommandRegistration(CommandDefinition* def, bool reg) {}

         // methods, used for remote command execution
         virtual bool CanSendCmdToManager(const char*) { return false; }
         virtual bool SendOverCommandChannel(const char* managername, const char* cmddata);
         void RecvOverCommandChannel(const char* cmddata);

         void FireParamEvent(Parameter* par, int evid);

         void InitSMmodule();

         // must be called in inherited class constructor & destructor
         void init();
         void destroy();

      private:
         // this method is used from Factory to register factory when it created
         void AddFactory(Factory* factory);
   };

   inline dabc::Manager* mgr() { return dabc::Manager::Instance(); }


   /** These functions prepare commands arguments so, that
     * they can be directly submitted to the manager via submit for instance:
     *    dabc::mgr()->Submit(SetCmdReceiver(new Command("Start"), "Generator"));
     * Command will be queued in manager queue and than submitted to specified
     * object. If object has own thread, it will be used for command execution */

   extern Command* SetCmdReceiver(Command* cmd, const char* itemname);
   extern Command* SetCmdReceiver(Command* cmd, const char* nodename, const char* itemname);
   extern Command* SetCmdReceiver(Command* cmd, int nodeid, const char* itemname);
   extern Command* SetCmdReceiver(Command* cmd, Basic* rcv);

}

#endif
