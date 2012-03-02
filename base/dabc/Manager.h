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

namespace dabc {

   class Mutex;
   class Thread;
   class Port;
   class MemoryPool;
   class Manager;
   class ManagerRef;
   class Device;
   class Factory;
   class DependPairList;
   class DataInput;
   class DataOutput;
   class FileIO;
   class Configuration;
   class StdManagerFactory;
   class ReferencesVector;
   class Thread;
   class ParamEventReceiverList;

   class CmdDeletePool : public Command {
      public:
         static const char* CmdName() { return "DeletePool"; }

         CmdDeletePool(const char* name) : Command(CmdName())
            { SetStr("PoolName", name); }
   };


   class CmdModule : public Command {
      public:
         static const char* ModuleArg() { return "Module"; }

         CmdModule(const char* cmdname, const char* module) : Command(cmdname)
         {
            SetStr(ModuleArg(), module);
         }
   };

   class CmdCreateModule : public CmdModule {
      public:
         static const char* CmdName() { return "CreateModule"; }

         CmdCreateModule(const char* classname, const char* modulename, const char* thrdname = 0) :
            CmdModule(CmdName(), modulename)
            {
               SetStr("Class", classname);
               SetStr("Thread", thrdname);
            }
   };

   class CmdStartModule : public CmdModule {
      public:
         static const char* CmdName() { return "StartModule"; }

         CmdStartModule(const char* modulename = "*") : CmdModule(CmdName(), modulename) {}
   };

   class CmdStopModule : public CmdModule {
      public:
         static const char* CmdName() { return "StopModule"; }

         CmdStopModule(const char* modulename = "*") : CmdModule(CmdName(), modulename) {}
   };

   class CmdDeleteModule : public CmdModule {
      public:
         static const char* CmdName() { return "DeleteModule"; }

         CmdDeleteModule(const char* modulename) : CmdModule(CmdName(), modulename) {}
   };

   class CmdCleanupApplication : public Command {
      DABC_COMMAND(CmdCleanupApplication, "CleanupManager");
   };

   class CmdCreateApplication : public Command {

      DABC_COMMAND(CmdCreateApplication, "CreateApplication");

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
            SetStr("Thread", thrdname);
         }
   };

   class CmdDestroyDevice : public Command {
      public:
         static const char* CmdName() { return "DestroyDevice"; }

         CmdDestroyDevice(const char* devname) :
            Command(CmdName())
         {
            SetStr("DevName", devname);
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

      CmdCreateTransport(const std::string& portname, const std::string& transportkind, const std::string& thrdname = "") :
         Command(CmdName())
      {
         SetStr("PortName", portname);
         SetStr("TransportKind", transportkind);
         SetStr(xmlTrThread, thrdname);
      }
   };

   class CmdDestroyTransport : public Command {

      DABC_COMMAND(CmdDestroyTransport, "DestroyTransport");

         CmdDestroyTransport(const std::string& portname) :
            Command(CmdName())
         {
            SetStr("PortName", portname);
         }
   };


   class CmdConnectPorts: public Command {
      public:
         static const char* CmdName() { return "ConnectPorts"; }

         CmdConnectPorts(const char* port1fullname,
                         const char* port2fullname,
                         const char* device = 0,
                         const char* trthread = 0,
                         bool required = true) :
            Command(CmdName())
         {
            SetStr("Port1Name", port1fullname);
            SetStr("Port2Name", port2fullname);
            SetStr("Device", device);
            SetStr(xmlTrThread, trthread);
            SetBool("Required", required);
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

   /** \brief Command to request current state of known nodes */

   class CmdGetNodesState : public Command {
      public:
         static const char* CmdName() { return "CmdGetNodesState"; }
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

         CmdGetNodesState() : Command(CmdName()) {}
   };

   template<class T>
   class CleanupEnvelope : public Object {
      protected:
         T* fObj;
      public:
         CleanupEnvelope(T* obj) : Object(0, "noname"), fObj(obj) {}
         virtual ~CleanupEnvelope() { delete fObj; }
   };

   /** \brief This is central class of DABC which manages everything
    *
    * It manages: modules, devices, memory pools
    *
    */


   class Manager : public Worker {

      friend class Object;
      friend class Factory;
      friend class ParameterContainer;
      friend class StdManagerFactory;
      friend class ManagerRef;

      protected:

         enum MgrEvents { evntDestroyObj = evntFirstSystem, evntManagerParam };

         /** Find object in manager hierarchy with specified itemname.
          * Itemname can be complete url or just local path (islocal = true) */
         Reference FindItem(const std::string& itemname, bool islocal = false);

         /** Method should be used to produce name of object, which can be used as item name
          * in different Find methods of manager. Item name later can be used to produce url to the item*/
         void FillItemName(const Object* ptr, std::string& itemname, bool compact = true);

         ModuleRef FindModule(const char* name);

         Reference GetAppFolder(bool force = false);
         ApplicationRef GetApp();

         virtual const char* ClassName() const { return "Manager"; }

         WorkerRef DoCreateModule(const char* classname, const char* modulename, const char* thrdname = 0, Command cmd = 0);

         Reference DoCreateObject(const char* classname, const char* objname = 0, Command cmd = 0);

         /** Return reference on command channel, should be used from manager thread only
          * If force specified, new command channel will be created */
         WorkerRef GetCommandChannel(bool force = false);

      public:

         Manager(const char* managername, Configuration* cfg = 0);
         virtual ~Manager();

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


         Reference GetFactoriesFolder(bool force = false) { return GetFolder(FactoriesFolderName(), force, false); }

         Reference FindPort(const char* name);

         Factory* FindFactory(const char* name);
         WorkerRef FindDevice(const char* name);

         // ------------------ threads manipulation ------------------

         Reference GetThreadsFolder(bool force = false) { return GetFolder(ThreadsFolderName(), force, true); }

         ThreadRef FindThread(const char* name, const char* required_class = 0);

         /** \brief Returns reference on the thread, which is now active.
          * If thread object for given context does not exists (foreign thread), 0 will be return
          */
         ThreadRef CurrentThread();

         /** \brief Create thread with specified name and class name
          * \param startmode indicate if thread is real (=0)  or use current thread (=1) */
         ThreadRef CreateThread(const std::string& thrdname, const char* classname = 0, const char* devname = 0, Command cmd = 0);

         /** \brief Create thread for processor and assigns processor to this thread */
         bool MakeThreadFor(Worker* proc, const char* thrdname = 0);

         /** \brief Runs manager mainloop. If \param runtime bigger than 0, only specified time will be run */
         void RunManagerMainLoop(double runtime = 0.);

         /** Perform sleeping with event loop running.
          *  If prefix specified, output on terminal is performed */
         void Sleep(double tmout, const char* prefix = 0);

         // ---------------- modules manipulation ------------------

         bool IsAnyModuleRunning();

         bool ConnectPorts(const char* port1name,
                           const char* port2name,
                           const char* devname = 0);

         // ----------- memory pools creation/deletion -------------------

         Reference FindPool(const char* name);

         /** Delete memory pool */
         bool DeletePool(const char* name);

         // ---------------- interface to control system -------------

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

         /** Provide log message to control system to display on GUI */
         virtual void LogMessage(int, const char*) {}

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

         /** Register/unregister dependency between objects
           * One use dependency to detect in \param src object situation when
           * dependent \param tgt object is destroyed.
           * In this case virtual ObjectDestroyed(tgt) method of src object will be called.
           * Normally one should "forget" pointer on dependent object at this moment.
           * In case if reference was used, reference must be released */
         bool RegisterDependency(Object* src, Object* tgt, bool bidirectional = false);
         bool UnregisterDependency(Object* src, Object* tgt, bool bidirectional = false);

         bool InstallCtrlCHandler();
         void ProcessCtrlCSignal();

         virtual bool Find(ConfigIO &cfg);

         // ------------ access to factories method -------------

         bool CreateApplication(const char* classname = 0, const char* appthrd = 0);

         FileIO* CreateFileIO(const char* typ, const char* name, int option);

         Object* ListMatchFiles(const char* typ, const char* filemask);

      protected:

         struct ParamRec {
            Parameter par;  //!< reference on the paramer
            int event;

            ParamRec() : par(), event(0) {}
            ParamRec(Parameter p, int e) : par(p), event(e) {}
            ParamRec(const ParamRec& src) : par(src.par), event(src.event) {}
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

         std::string fLastCreatedDevName;  //!< name of last created device, automatically used for connection establishing

         std::string GetLastCreatedDevName();

         virtual double ProcessTimeout(double last_diff);

         bool DoCreateMemoryPool(Command cmd);

         bool DoCleanupApplication();
         void DoPrint();

         virtual int PreviewCommand(Command cmd);
         virtual int ExecuteCommand(Command cmd);
         virtual bool ReplyCommand(Command cmd);

         bool ProcessDestroyQueue();
         bool ProcessParameterEvents();
         void ProduceParameterEvent(ParameterContainer* par, int evid);

         virtual void ProcessEvent(const EventId&);

         // must be called in inherited class constructor & destructor
         void destroy();

      private:
         // this method is used from Factory to register factory when it created
         void AddFactory(Factory* factory);
   };


   /** Reference on manager object
    *  Should be used as thread-safe interface to manager functionality.
    *  Instance of ManagerRef is available via dabc::mgr function.
    *  FIXME: in future dabc::mgr() should be manager reference, manager pointer itself
    *  should not be seen by the user
    */

   class ManagerRef : public WorkerRef {

      DABC_REFERENCE(ManagerRef, WorkerRef, Manager)

      protected:

         friend class Manager;
         friend class Worker;
         friend class Port; // need to activate connection manager

         bool ParameterEventSubscription(Worker* ptr, bool subscribe, const std::string& mask, bool onlychangeevent = true);

         bool CreateConnectionManager();

         /** Returns true, if name of the item should specify name in local context or from remote node */
         bool IsLocalItem(const std::string& name);

      public:

         ThreadRef CurrentThread();

         Reference GetAppFolder(bool force = false);

         ApplicationRef GetApp();

         /** Method safely deletes all object created for application - modules, devices, memory pools.
          * All these objects normally collected under "/App" folder, therefore manager cleanup is just
          * deletion of this folder. Threads are not directly destroyed, while they are collected in other folder,
          * but they will be destroyed as soon as they are not used. */
         bool CleanupApplication();

         int NodeId() const;

         int NumNodes() const;

         /** Produces unique url of the object which can be used on other nodes too */
         std::string ComposeUrl(Object* ptr);

         ThreadRef CreateThread(const std::string& thrdname, const char* classname = 0);

         bool CreateDevice(const char* classname, const char* devname, const char* devthrd = 0);
         bool DestroyDevice(const char* devname);
         WorkerRef FindDevice(const std::string& name);

         Reference CreateObject(const std::string& classname, const std::string& objname);

         /** Generic method to create memory pool.
           * Creates memory pool with numbuffers buffers of size buffersize.
           * Together with buffers memory pool creates number of reference objects.
           * If zero arguments are specified, configuration from xml file will be used.
           * If none (arguments or xml file) provides non-zero values, only pool instance
           * without buffers will be created.
           * For more sophisticated configuration of memory pool CmdCreateMemoryPool should be used */
         bool CreateMemoryPool(const char* poolname,
                               unsigned buffersize = 0,
                               unsigned numbuffers = 0,
                               unsigned refcoeff = 0);

         bool CreateModule(const char* classname, const char* modulename, const char* thrdname = 0);
         ModuleRef FindModule(const char* name);
         void StartModule(const char* modulename);
         void StopModule(const char* modulename);
         bool StartAllModules();
         bool StopAllModules();
         bool DeleteModule(const char* name);

         bool CreateTransport(const std::string& portname, const std::string& transportkind, const std::string& thrdname = "");

         bool ActivateConnections(double tmout);

         Reference FindItem(const std::string& name);

         Reference FindPort(const std::string& port);

         Parameter FindPar(const std::string& parname);

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

   };

   extern ManagerRef mgr;
}

#endif
