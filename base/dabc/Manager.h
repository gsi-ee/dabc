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

#ifndef DABC_Manager
#define DABC_Manager

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#ifndef DABC_Thread
#include "dabc/Thread.h"
#endif

#ifndef DABC_Command
#include "dabc/Command.h"
#endif

#ifndef DABC_Queue
#include "dabc/Queue.h"
#endif

#ifndef DABC_Application
#include "dabc/Application.h"
#endif

#ifndef DABC_Module
#include "dabc/Module.h"
#endif

#ifndef DABC_ConnectionRequest
#include "dabc/ConnectionRequest.h"
#endif

#ifndef DABC_ConfigBase
#include "dabc/ConfigBase.h"
#endif

namespace dabc {

   class Mutex;
   class Thread;
   class Port;
   class MemoryPool;
   class Manager;
   class ManagerRef;
   class Device;
   class Factory;
   class FactoryPlugin;
   class DependPairList;
   class DataInput;
   class DataOutput;
   class Configuration;
   class StdManagerFactory;
   class ReferencesVector;
   class Thread;
   class ParamEventReceiverList;

   class CmdDeletePool : public Command {
      public:
         static const char* CmdName() { return "DeletePool"; }

         CmdDeletePool(const std::string& name) : Command(CmdName())
            { SetStr("PoolName", name); }
   };


   class CmdModule : public Command {
      public:
         static const char* ModuleArg() { return "Module"; }

         CmdModule(const char* cmdname, const std::string& module) : Command(cmdname)
         {
            SetStr(ModuleArg(), module);
         }
   };

   class CmdCreateModule : public CmdModule {
      public:
         static const char* CmdName() { return "CreateModule"; }

         CmdCreateModule(const std::string& classname, const std::string& modulename, const std::string& thrdname = "") :
            CmdModule(CmdName(), modulename)
            {
               SetStr(xmlClassAttr, classname);
               if (!thrdname.empty()) SetStr(xmlThreadAttr, thrdname);
            }
   };

   class CmdStartModule : public CmdModule {
      public:
         static const char* CmdName() { return "StartModule"; }

         CmdStartModule(const std::string& modulename = "*") : CmdModule(CmdName(), modulename) {}
   };

   class CmdStopModule : public CmdModule {
      public:
         static const char* CmdName() { return "StopModule"; }

         CmdStopModule(const std::string& modulename = "*") : CmdModule(CmdName(), modulename) {}
   };

   class CmdDeleteModule : public CmdModule {
      public:
         static const char* CmdName() { return "DeleteModule"; }

         CmdDeleteModule(const std::string& modulename) : CmdModule(CmdName(), modulename) {}
   };

   class CmdCleanupApplication : public Command {
      DABC_COMMAND(CmdCleanupApplication, "CleanupManager");
   };

   class CmdCreateApplication : public Command {

      DABC_COMMAND(CmdCreateApplication, "CreateApplication");

      CmdCreateApplication(const std::string& appclass, const std::string& thrdname = "") :
         Command(CmdName())
      {
         SetStr("AppClass", appclass);
         if (!thrdname.empty()) SetStr(xmlThreadAttr, thrdname);
      }
   };


   class CmdCreateDevice : public Command {
      public:
         static const char* CmdName() { return "CreateDevice"; }

         CmdCreateDevice(const std::string& devclass, const std::string& devname) :
            Command(CmdName())
         {
            SetStr("DevClass", devclass);
            SetStr("DevName", devname);
         }
   };

   class CmdDestroyDevice : public Command {
      public:
         static const char* CmdName() { return "DestroyDevice"; }

         CmdDestroyDevice(const std::string& devname) :
            Command(CmdName())
         {
            SetStr("DevName", devname);
         }
   };

   class CmdCreateThread : public Command {
      public:
         static const char* CmdName() { return "CreateThread"; }

         static const char* ThrdNameArg() { return "ThrdName"; }

         CmdCreateThread(const std::string& thrdname, const std::string& thrdclass = "",  const std::string& devname = "") :
            Command(CmdName())
         {
            SetStr(ThrdNameArg(), thrdname);
            SetStr("ThrdClass", thrdclass);
            SetStr("ThrdDev", devname);
         }

         std::string GetThrdName() const { return GetStdStr(ThrdNameArg()); }
   };

   class CmdCreateObject : public Command {

      DABC_COMMAND(CmdCreateObject, "CreateObject");

      CmdCreateObject(const std::string& classname, const std::string& objname) :
         Command(CmdName())
      {
         SetStr("ClassName", classname);
         SetStr("ObjName", objname);
      }
   };


   class CmdCreateTransport : public Command {

      DABC_COMMAND(CmdCreateTransport, "CreateTransport");

      static const char* PortArg() { return "PortName"; }
      static const char* KindArg() { return "TransportKind"; }

      CmdCreateTransport(const std::string& portname, const std::string& transportkind, const std::string& thrdname = "") :
         Command(CmdName())
      {
         SetStr(PortArg(), portname);
         SetStr(KindArg(), transportkind);
         if (!thrdname.empty()) SetStr(xmlThreadAttr, thrdname);
      }

      void SetPoolName(const std::string& name) { if (!name.empty()) SetStr(xmlPoolName, name); }

      std::string PortName() const { return GetStdStr(PortArg()); }
      std::string TransportKind() const { return GetStdStr(KindArg()); }
      std::string PoolName() const { return GetStdStr(xmlPoolName); }
   };



   class CmdDestroyTransport : public Command {

      DABC_COMMAND(CmdDestroyTransport, "DestroyTransport");

      CmdDestroyTransport(const std::string& portname) :
            Command(CmdName())
      {
         SetStr("PortName", portname);
      }
   };

   class CmdSetParameter : public Command {

      DABC_COMMAND(CmdSetParameter, "SetParameter");

      RecordField ParName() { return Field("ParName"); }

      RecordField ParValue() { return Field("ParValue"); }
   };

   class CmdShutdownControl : public Command {
      DABC_COMMAND(CmdShutdownControl, "CmdShutdownControl")
   };

   /** \brief %Command to request current state of known nodes */
   class CmdGetNodesState : public Command {

      DABC_COMMAND(CmdGetNodesState, "CmdGetNodesState")

      static const char* States() { return "States"; }
      static const char* NodeId() { return "NodeId"; }

      static void SetState(std::string& sbuf, unsigned node, bool on)
      {
         while (sbuf.length() <= node) sbuf.append("x");
         sbuf[node] = on ? 'x' : 'o';
      }

      static bool GetState(const std::string& sbuf, unsigned node)
      {
         if (node>=sbuf.length()) return false;
         return sbuf[node] == 'x';
      }
   };

   /** \brief Template to put any object, not derived from \ref dabc::Object, into cleanup queue */
   template<class T>
   class CleanupEnvelope : public Object {
      protected:
         T* fObj;
      public:
         CleanupEnvelope(T* obj) : Object(0, "noname"), fObj(obj) {}
         virtual ~CleanupEnvelope() { delete fObj; }
   };


   enum ThreadsLayout {
      layoutMinimalistic,  // minimal number of threads
      layoutPerModule,    // each module should use single thread, including all transports
      layoutBalanced,     // per each module three threads - for module, all input and all output transports
      layoutMaximal       // each entity will gets its own thread
   };



   /** \brief %Manager of everything in DABC
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    * It manages: modules, devices, memory pools, ...
    */

   class Manager : public Worker {

      friend class Object;
      friend class Factory;
      friend class FactoryPlugin;
      friend class ParameterContainer;
      friend class StdManagerFactory;
      friend class ManagerRef;

      private:

         enum { MagicInstanceId = 7654321 };

         // this method is used from Factory to register factory when it created
         static void ProcessFactory(Factory* factory);

         static Manager* fInstance; //! pointer on current manager instance
         static int fInstanceId; //! magic number which indicates that instance is initialized

      protected:

         enum MgrEvents { evntDestroyObj = evntFirstSystem, evntManagerParam };

         struct ParamRec {
            Parameter par;  ///< reference on the parameter
            int event;

            ParamRec() : par(), event(0) {}
            ParamRec(Parameter p, int e) : par(p), event(e) {}
            ParamRec(const ParamRec& src) : par(src.par), event(src.event) {}
            ~ParamRec() { reset(); }

            ParamRec& operator=(const ParamRec& src)
            {
               par = src.par;
               event = src.event;
               return* this;
            }

            void reset()
            {
               par.Release();
               event = 0;
            }
         };

         TimeStamp            fMgrStoppedTime; // indicate when manager mainloop was stopped

         // FIXME: revise usage of manager mutex, it is not necessary everywhere
         Mutex                *fMgrMutex; // main mutex to protect manager queues

         ReferencesVector     *fDestroyQueue;
         RecordsQueue<ParamRec> fParsQueue;

         // TODO: timed parameters should be seen in the special manager folder ?
         ReferencesVector     *fTimedPars;

         /** list of workers, registered for receiving of parameter events
          * Used only from manager thread, therefore not protected with mutex */
         ParamEventReceiverList*  fParEventsReceivers;

         // TODO: dependency pairs should be seen in special manager folder
         DependPairList       *fDepend; // list of objects dependencies

         Configuration        *fCfg;
         std::string           fCfgHost;

         int                   fNodeId;
         int                   fNumNodes;

         ThreadsLayout          fThrLayout; ///< defines distribution of threads

         std::string fLastCreatedDevName;  ///< name of last created device, automatically used for connection establishing

         /** Find object in manager hierarchy with specified itemname.
          * Itemname can be complete url or just local path (islocal = true) */
         Reference FindItem(const std::string& itemname, bool islocal = false);

         /** Method should be used to produce name of object, which can be used as item name
          * in different Find methods of manager. Item name later can be used to produce url to the item*/
         void FillItemName(const Object* ptr, std::string& itemname, bool compact = true);

         ThreadRef FindThread(const std::string& name, const std::string& required_class = "");

         ThreadRef CurrentThread();

         WorkerRef FindDevice(const std::string& name);

         Reference FindPool(const std::string& name);

         ModuleRef FindModule(const std::string& name);

         Reference FindPort(const std::string& name);

         Reference GetAppFolder(bool force = false);
         ApplicationRef app();

         bool IsAnyModuleRunning();

         bool DoCreateMemoryPool(Command cmd);

         /** \brief Create thread with specified name and class name
          *
          * \param[in] thrdname    name of created thread
          * \param[in] classname   class name (when empty, dabc::Thread will be created)
          * \param[in] devname     device name to use for thread creation
          * \param[in] cmd         command object with additional optional arguments
          * \returns               reference on created thread */
         ThreadRef DoCreateThread(const std::string& thrdname, const std::string& classname = "", const std::string& devname = "", Command cmd = 0);

         WorkerRef DoCreateModule(const std::string& classname, const std::string& modulename, Command cmd);

         Reference DoCreateObject(const std::string& classname, const std::string& objname = "", Command cmd = 0);

         /** \brief Return reference on command channel
          * \details should be used from manager thread only
          * If force specified, new command channel will be created */
         WorkerRef GetCommandChannel(bool force = false);

         /** \brief Return reference on folder with factories
          *
          * @param force - if specified, missing factories folder will be created
          * @return reference on factories folder
          */
         Reference GetFactoriesFolder(bool force = false) { return GetFolder(FactoriesFolderName(), force); }

         /** \brief Return reference on folder with all registered threads
          *
          * @param force - if specified, missing threads folder will be created
          * @return reference on factories folder
          */
         Reference GetThreadsFolder(bool force = false) { return GetFolder(ThreadsFolderName(), force); }

         /** Perform sleeping with event loop running.
          *  If prefix specified, output on terminal is performed */
         void Sleep(double tmout, const char* prefix = 0);

         std::string GetLastCreatedDevName();

         /** \brief Runs manager mainloop.
          * \param[in] runtime  - time limit for running, 0 - unlimited */
         void RunManagerMainLoop(double runtime = 0.);

         bool DoCleanupApplication();

         // virtual methods from Worker

         virtual double ProcessTimeout(double last_diff);

         virtual int PreviewCommand(Command cmd);
         virtual int ExecuteCommand(Command cmd);
         virtual bool ReplyCommand(Command cmd);

         virtual void ProcessEvent(const EventId&);

      public:

         Manager(const std::string& managername, Configuration* cfg = 0);
         virtual ~Manager();

         virtual const char* ClassName() const { return "Manager"; }

          // candidates for protected

         /** Delete all modules and stop manager thread.
           * Normally, last command before exit from main program.
           * Automatically called from destructor */
         void HaltManager();

         // -------------- generic folder structure of manager

         static const char* FactoriesFolderName() { return "Factories"; }
         static const char* ThreadsFolderName() { return "Threads"; }

         static const char* MgrThrdName()       { return "ManagerThrd"; }
         static const char* AppThrdName()       { return "AppThrd"; }
         static const char* ConnMgrName()       { return "/#ConnMgr"; }
         static const char* CmdChlName()        { return "/#CommandChl"; }


         /** Return nodes id of local node */
         int NodeId() const { return fNodeId; }
         /** Returns number of nodes in the cluster FIXME:probably must be removed*/
         int NumNodes() { return fNumNodes; }
         /** Return name of node */
         std::string GetNodeName(int nodeid);

         /** Returns true if node with specified id is active */
         virtual bool IsNodeActive(int num) { return num==0; }
         /** Returns number of currently active nodes */
         int NumActiveNodes();
         /** Test active nodes - try to execute simpe ping command on each active node */
         bool TestActiveNodes(double tmout = 5.);

         /** \brief Establish/test connection to control system */
         virtual bool ConnectControl();
         /** \brief Disconnect connection to control system */
         virtual void DisconnectControl();

         ThreadsLayout GetThreadsLayout() const { return fThrLayout; }

         // -------------------------- misc functions ---------------

         Configuration* cfg() const { return fCfg; }

         /** Displays on std output list of running threads and modules */
         void Print();

         /** Delete derived from Object class object in manager thread.
           * Useful as replacement of call "delete this;" */
         virtual bool DestroyObject(Reference ref);

         /** Delete of any kind of object in manager thread */
         template<class T> bool DeleteAnyObject(T* obj)
         {
             return DestroyObject(Reference(new CleanupEnvelope<T>(obj)));
         }

         /** \brief Register dependency between objects
          *
           * One use dependency to notify source object situation when
           * dependent tgt object is destroyed.
           * In this case src->ObjectDestroyed(tgt) call will be done.
           * Normally one should "forget" pointer on dependent object at this moment.
           * In case if reference was used, reference must be released
           * \param[in] src,tgt        object src waiting when tgt object is destroyed
           * \param[in] bidirectional  define that dependency should be register in both directions
           * \returns                  true when dependency can be registered */
         bool RegisterDependency(Object* src, Object* tgt, bool bidirectional = false);

         /** \brief Unregister dependency between objects
          *
          * Opposite to RegisterDependency call */
         bool UnregisterDependency(Object* src, Object* tgt, bool bidirectional = false);

         bool InstallCtrlCHandler();
         void ProcessCtrlCSignal();

         virtual bool Find(ConfigIO &cfg);

         // ------------ access to factories method -------------

         /** Method set flag if manager instance should be destroyed when process finished */
         static void SetAutoDestroy(bool on);


         bool ProcessDestroyQueue();
         bool ProcessParameterEvents();
         void ProduceParameterEvent(ParameterContainer* par, int evid);
   };

   /** \brief %Reference on manager object
    *
    * \ingroup dabc_user_classes
    * \ingroup dabc_all_classes
    *
    *  Should be used as thread-safe interface to manager functionality.
    *  Instance of ManagerRef is available via dabc::mgr variable.
    */

   class ManagerRef : public WorkerRef {

      DABC_REFERENCE(ManagerRef, WorkerRef, Manager)

      protected:

         friend class Manager;
         friend class Worker;
         friend class Port;   // need to activate connection manager

         bool ParameterEventSubscription(Worker* ptr, bool subscribe, const std::string& mask, bool onlychangeevent = true);

         /** Returns true, if name of the item should specify name in local context or from remote node */
         bool IsLocalItem(const std::string& name);

         /** Creates connection manager, used for connection establishing */
         bool CreateConnectionManager();

      public:

         Reference GetAppFolder(bool force = false);

         ApplicationRef app();

         /** Method safely deletes all object created for application - modules, devices, memory pools.
          * All these objects normally collected under "/App" folder, therefore manager cleanup is just
          * deletion of this folder. Threads are not directly destroyed, while they are collected in other folder,
          * but they will be destroyed as soon as they are not used. */
         bool CleanupApplication();

         int NodeId() const;

         int NumNodes() const;

         /** Produces unique url of the object which can be used on other nodes too */
         std::string ComposeUrl(Object* ptr);

         bool CreateApplication(const std::string& classname = "", const std::string& appthrd = "");

         ThreadRef CreateThread(const std::string& thrdname, const std::string& classname = "", const std::string& devname = "");

         /** \brief Returns reference on the thread, which is now active.
          * If thread object for given context does not exists (foreign thread),
          * null reference will be return */
         ThreadRef CurrentThread();

         bool CreateDevice(const std::string& classname, const std::string& devname);
         bool DestroyDevice(const std::string& devname);
         WorkerRef FindDevice(const std::string& name);

         Reference CreateObject(const std::string& classname, const std::string& objname);

         /** Generic method to create memory pool.
           * Creates memory pool with numbuffers buffers of size buffersize.
           * Together with buffers memory pool creates number of reference objects.
           * If zero arguments are specified, configuration from xml file will be used.
           * If none (arguments or xml file) provides non-zero values, only pool instance
           * without buffers will be created.
           * For more sophisticated configuration of memory pool CmdCreateMemoryPool should be used */
         bool CreateMemoryPool(const std::string& poolname,
                               unsigned buffersize = 0,
                               unsigned numbuffers = 0,
                               unsigned refcoeff = 0);

         bool DeletePool(const std::string& name);

         ModuleRef CreateModule(const std::string& classname, const std::string& modulename, const std::string& thrdname = "");
         ModuleRef FindModule(const std::string& name);
         void StartModule(const std::string& modulename);
         void StopModule(const std::string& modulename);
         bool StartAllModules();
         bool StopAllModules();
         bool DeleteModule(const std::string& name);

         bool CreateTransport(const std::string& portname, const std::string& transportkind = "", const std::string& thrdname = "");

         bool ActivateConnections(double tmout);

         Reference FindItem(const std::string& name);

         Reference FindPort(const std::string& port);

         Parameter FindPar(const std::string& parname);

         Reference FindPool(const std::string& name);

         ThreadRef FindThread(const std::string& name, const std::string& required_class = "")
            { return null() ? ThreadRef() : GetObject()->FindThread(name, required_class); }

         unsigned NumThreads() const
            { return null() ? 0 : GetObject()->GetThreadsFolder().NumChilds(); }

         ThreadsLayout GetThreadsLayout() const
           { return null() ? layoutBalanced : GetObject()->GetThreadsLayout(); }

         /**\brief Request connection between two ports.
          * If both ports belong to local node, they will be connected immediately.
          * If both ports belong to remote nodes, nothing will happen.
          * If one of the port is local and other is remote, new connection request will be created.
          * Depending on current application state connection establishing will be started immediately or will
          * be performed by DoEnable state transition. */
         ConnectionRequest Connect(const std::string& port1, const std::string& port2);

         void StopApplication();

         /** Sleep for specified time, keep thread event loop running
          * See Manager::Sleep() method for more details */
         void Sleep(double tmout, const char* prefix = 0);

         /** Run manager main-loop, should be called either from manager thread or from main process */
         void RunMainLoop(double runtime = 0.);

   };

   extern ManagerRef mgr;
}

#endif
