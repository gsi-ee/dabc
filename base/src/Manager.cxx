#include "dabc/Manager.h"

#include <list>
#include <vector>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>

#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/threads.h"

#include "dabc/Port.h"
#include "dabc/Module.h"
#include "dabc/Command.h"
#include "dabc/MemoryPool.h"
#include "dabc/Device.h"
#include "dabc/PoolHandle.h"
#include "dabc/LocalTransport.h"
#include "dabc/SocketDevice.h"
#include "dabc/WorkingThread.h"
#include "dabc/SocketThread.h"
#include "dabc/TimeSyncModule.h"
#include "dabc/Factory.h"
#include "dabc/Application.h"
#include "dabc/Iterator.h"
#include "dabc/Parameter.h"
#include "dabc/FileIO.h"
#include "dabc/BinaryFile.h"
#include "dabc/DataIOTransport.h"
#include "dabc/XmlEngine.h"
#include "dabc/StateMachineModule.h"

namespace dabc {

   class StdManagerFactory : public Factory {
      public:
         StdManagerFactory(const char* name) :
            Factory(name)
         {
         }

         virtual Module* CreateModule(const char* classname, const char* modulename, Command* cmd);

         virtual Device* CreateDevice(Basic* parent,
                                      const char* classname,
                                      const char* devname,
                                      Command*);

         virtual WorkingThread* CreateThread(Basic* parent, const char* classname, const char* thrdname, const char* thrddev, Command* cmd);

         virtual FileIO* CreateFileIO(const char* typ, const char* name, int option);
         virtual Folder* ListMatchFiles(const char* typ, const char* filemask);
   };

   typedef struct DependPair {
      Basic* src;
      Basic* tgt;
   };

   class DependPairList : public std::list<DependPair> {};

}

dabc::Module* dabc::StdManagerFactory::CreateModule(const char* classname, const char* modulename, Command* cmd)
{
   if (strcmp(classname, "TimeSyncModule")==0)
      return new TimeSyncModule(modulename, cmd);

   return 0;
}

dabc::Device* dabc::StdManagerFactory::CreateDevice(Basic* parent,
                                                    const char* classname,
                                                    const char* devname,
                                                    Command*)
{
   if (strcmp(classname, "SocketDevice")==0)
      return new SocketDevice(parent, devname);
   else
   if (strcmp(classname, "LocalDevice")==0)
      return new LocalDevice(parent, devname);
   return 0;
}

dabc::WorkingThread* dabc::StdManagerFactory::CreateThread(Basic* parent, const char* classname, const char* thrdname, const char* thrddev, Command* cmd)
{
   if ((classname==0) || (strlen(classname)==0) || (strcmp(classname, "WorkingThread")==0))
      return new WorkingThread(parent, thrdname, cmd ? cmd->GetInt("NumQueues", 3) : 3);
   else
   if (strcmp(classname, "SocketThread")==0)
      return new SocketThread(parent, thrdname, cmd ? cmd->GetInt("NumQueues", 3) : 3);

   return 0;
}

dabc::FileIO* dabc::StdManagerFactory::CreateFileIO(const char* typ, const char* name, int option)
{
   if (!((typ==0) || (strlen(typ)==0) ||
       (strcmp(typ,"std")==0) || (strcmp(typ,"posix")==0))) return 0;


   dabc::FileIO* io = new dabc::PosixFileIO(name, option);

   if (!io->IsOk()) { delete io; io = 0; }

   return io;
}

dabc::Folder* dabc::StdManagerFactory::ListMatchFiles(const char* typ, const char* filemask)
{
   if (!((typ==0) || (strlen(typ)==0) ||
       (strcmp(typ,"std")==0) || (strcmp(typ,"posix")==0))) return 0;

   if ((filemask==0) || (strlen(filemask)==0)) return 0;

   DOUT4(("List files %s", filemask));

   String pathname;
   const char* fname;

   const char* slash = strrchr(filemask, '/');

   if (slash==0) {
      pathname = ".";
      fname = filemask;
   } else {
      pathname.assign(filemask, slash - filemask + 1);
      fname = slash + 1;
   }

   struct dirent **namelist;
   int len = scandir(pathname.c_str(), &namelist, 0, alphasort);
   if (len < 0) {
      EOUT(("Directory %s not found",  pathname.c_str()));
      return 0;
   }

   Folder* res = 0;
   struct stat buf;

   for (int n=0;n<len;n++) {
      const char* item = namelist[n]->d_name;

      if ((fname==0) || (fnmatch(fname, item, FNM_NOESCAPE)==0)) {
         String fullitemname;
         if (slash) fullitemname += pathname;
         fullitemname += item;
         if (stat(fullitemname.c_str(), &buf)==0)
            if (!S_ISDIR(buf.st_mode) && (access(fullitemname.c_str(), R_OK)==0)) {
               if (res==0) res = new Folder(0, "FilesList", true);
               new Basic(res, fullitemname.c_str());
               DOUT4((" File: %s", fullitemname.c_str()));
            }
      }

      free(namelist[n]);
   }

   free(namelist);

   return res;
}


// ******************************************************************


dabc::Manager* dabc::Manager::fInstance = 0;

const char* dabc::Manager::stParName = "CoreState";

const char* dabc::Manager::stNull = "Null";
const char* dabc::Manager::stHalted = "Halted";
const char* dabc::Manager::stConfigured = "Configured";
const char* dabc::Manager::stReady = "Ready";
const char* dabc::Manager::stRunning = "Running";
const char* dabc::Manager::stFailure = "Failure";
const char* dabc::Manager::stError = "Error";

const char* dabc::Manager::stcmdDoConfigure = "DoConfigure";
const char* dabc::Manager::stcmdDoEnable = "DoEnable";
const char* dabc::Manager::stcmdDoStart = "DoStart";
const char* dabc::Manager::stcmdDoStop = "DoStop";
const char* dabc::Manager::stcmdDoError = "DoError";
const char* dabc::Manager::stcmdDoHalt = "DoHalt";

dabc::Manager::Manager(const char* managername, bool usecurrentprocess) :
   Folder(0, managername, true),
   WorkingProcessor(GetFolder("Pars", true, true)),
   CommandClientBase(),
   fMgrMainLoop(true),
   fMgrMutex(0),
   fReplyesQueue(true, false),
   fDestroyQueue(16, true),
   fSendCmdsMutex(0),
   fSendCmdCounter(0),
   fSendCommands(),
   fTimedPars(),
   fDepend(0),
   fSigThrd(0),
   fSMmodule(0)
{
   if (fInstance==0) {
      fInstance = this;
      dabc::SetDebugPrefix(managername);
   }

   // we create recursive mutex to avoid problem in Folder::GetFolder method,
   // where constructor is called under locked mutex,
   // let say - there is no a big problem, when mutex locked several times from one thread

   fMgrMutex = new Mutex(true);

   fSendCmdsMutex = new Mutex;

   fDepend = new DependPairList;

   // this will sets basic pointer on manager itself and mutex
   {
      LockGuard lock(fMgrMutex);
      _SetParent(fMgrMutex, 0);
   }

   if (fInstance == this) {
      static StdManagerFactory stdfactory("std");

      Factory* factory = 0;

      while ((factory = Factory::NextNewFactory()) != 0)
         AddFactory(factory);
   }

   MakeThreadFor(this, MgrThrdName(), usecurrentprocess ? 1 : 0);

   CreateDevice("LocalDevice", LocalDeviceName());

   Device* dev = FindDevice(LocalDeviceName());
   if (dev)
      dev->SetAppId(7);
   else {
      EOUT(("Problem to create local device"));
      exit(1);
   }

   // create state parameter, inherited class should call init to see it
   CreateParameter(stParName, parString, stHalted, true, false);
}

dabc::Manager::~Manager()
{
   // stop thread that it is not try to access objects which we are directly deleting
   // normally, as last operation in the main() program must be HaltManeger(true)
   // call, which suspend and erase all items in manager

   DOUT3(("Start ~Manager"));

   fSMmodule = 0;

   HaltManager();

   DOUT5(("~Manager -> CancelCommands()"));

   CancelCommands();

   DOUT5(("~Manager -> DeleteChilds()"));

   DeleteChilds();

   {
      LockGuard lock(fMgrMutex);
      _SetParent(0, 0);
   }

   while (fDestroyQueue.Size()>0)
      delete fDestroyQueue.Pop();

   delete fDepend; fDepend = 0;

   delete fSendCmdsMutex; fSendCmdsMutex = 0;

   delete fMgrMutex; fMgrMutex = 0;

   DOUT3(("Did ~Manager"));

   if (fInstance==this) {
      DOUT0(("Real EXIT"));
      fInstance = 0;
   }
}

void dabc::Manager::init()
{
   dabc::Iterator iter(GetParsFolder());

   while (iter.next()) {
      Parameter* par = dynamic_cast<Parameter*> (iter.current());
      if (par) {
         DOUT5(("Raise event 0 again for par %s", par->GetName()));
         ParameterEvent(par, 0);
      }
   }
}

void dabc::Manager::destroy()
{
   DestroyParameter(stParName);
}

void dabc::Manager::InitSMmodule()
{
   if (fSMmodule!=0) return;

   fSMmodule = new StateMachineModule();
   MakeThreadForModule(fSMmodule, "SMthread");
   fSMmodule->SetAppId(77);

   fSMmodule->Start();

   DOUT2(("InitSMmodule done"));
}

const char* dabc::Manager::TargetStateName(const char* stcmd)
{
   if (stcmd==0) return 0;

   if (strcmp(stcmd, stcmdDoConfigure) == 0) return stConfigured; else
   if (strcmp(stcmd, stcmdDoEnable) == 0) return stReady; else
   if (strcmp(stcmd, stcmdDoStart) == 0) return stRunning; else
   if (strcmp(stcmd, stcmdDoStop) == 0) return stReady; else
   if (strcmp(stcmd, stcmdDoHalt) == 0) return stHalted;

   return 0;
}


void dabc::Manager::ChangeManagerName(const char* newname)
{
   if ((newname==0) || (*newname==0)) return;
   SetName(newname);

   dabc::SetDebugPrefix(newname);
}

void dabc::Manager::ProcessDestroyQueue()
{
   Basic* obj = 0;

   do {
     if (obj!=0) {
        DOUT5(("Destroy object %p", obj));
        delete obj;
        obj = 0;
     }

     LockGuard lock(fMgrMutex);
     if (fDestroyQueue.Size() > 0)
        obj = fDestroyQueue.Pop();
   } while (obj!=0);
}

void dabc::Manager::ProcessEvent(EventId evnt)
{
   switch (GetEventCode(evnt)) {
      case evntDestroyObj:
         ProcessDestroyQueue();
         break;
      case evntManagerReply: {
         Command* cmd = 0;
         {
            LockGuard lock(fMgrMutex);
            cmd = fReplyesQueue.Pop();
         }
         if (PostCommandProcess(cmd))
            dabc::Command::Finalise(cmd);
         break;
      }
      default:
         WorkingProcessor::ProcessEvent(evnt);
   }
}

dabc::Module* dabc::Manager::FindModule(const char* name)
{
   Folder* folder = GetModulesFolder(false);
   if (folder==0) return 0;

   return dynamic_cast<dabc::Module*> (folder->FindChild(name));
}

bool dabc::Manager::DeleteModule(const char* name)
{
   return Execute(new CommandDeleteModule(name));
}

bool dabc::Manager::IsModuleRunning(const char* name)
{
   LockGuard lock(fMgrMutex);

   dabc::Module* m = FindModule(name);
   return m ? m->WorkStatus()>0 : false;
}

bool dabc::Manager::IsAnyModuleRunning()
{
   LockGuard lock(fMgrMutex);

   Folder* f = GetModulesFolder(false);
   if (f==0) return false;

   for (unsigned n=0;n<f->NumChilds();n++) {
      Module* m = dynamic_cast<Module*> (f->GetChild(n));
      if (m && (m->WorkStatus()>0)) return true;
   }

   return false;
}

dabc::Port* dabc::Manager::FindPort(const char* name)
{
   dabc::Port* port = dynamic_cast<dabc::Port*>(FindChild(name));
   if (port!=0) return port;

   Folder* folder = GetModulesFolder(false);
   if (folder==0) return 0;

   return dynamic_cast<dabc::Port*> (folder->FindChild(name));
}

dabc::Device* dabc::Manager::FindDevice(const char* name)
{
   dabc::Device* dev =
      dynamic_cast<dabc::Device*> (FindChild(name));
   if (dev!=0) return dev;

   Folder* folder = GetDevicesFolder();
   if (folder==0) return 0;

   return dynamic_cast<dabc::Device*> (folder->FindChild(name));
}

dabc::LocalDevice* dabc::Manager::FindLocalDevice(const char* name)
{
   if ((name==0) || (strlen(name)==0)) name = LocalDeviceName();

   return dynamic_cast<dabc::LocalDevice*> (FindDevice(name));
}

dabc::Factory* dabc::Manager::FindFactory(const char* name)
{
   Folder* folder = GetFactoriesFolder(false);
   if (folder==0) return 0;

   return dynamic_cast<dabc::Factory*> (folder->FindChild(name));
}

dabc::Application* dabc::Manager::GetApp()
{
   Folder* f = GetAppFolder(false);
   if ((f==0) || (f->NumChilds()<1)) return 0;

   return dynamic_cast<Application*>(f->GetChild(0));
}

dabc::MemoryPool* dabc::Manager::FindPool(const char* name)
{
   Folder* folder = GetPoolsFolder(false);
   if (folder==0) return 0;

   return dynamic_cast<dabc::MemoryPool*> (folder->FindChild(name));
}

bool dabc::Manager::DeletePool(const char* name)
{
   return Execute(new CmdDeletePool(name));
}

void dabc::Manager::DoHaltManager()
{
//   dabc::SetDebugLevel(5);

   DOUT3(("Start DoHaltManager"));

   DOUT3(("Deleting all plugins"));
   dabc::Folder* df = GetAppFolder(false);
   if (df) df->DeleteChilds();

   DOUT3(("Deleting all modules"));
   // than we delete all modules
   df = GetModulesFolder();
   if (df) df->DeleteChilds();

   DoCleanupDevices(true);

   // special case - verbs objects may in destroy queue and must be deleted before devices
   ProcessDestroyQueue();

   DOUT3(("Calling destructor of all devices"));
   df = GetDevicesFolder();
   if (df) df->DeleteChilds();

   DOUT3(("Calling destructor of all memory pools"));
   df = GetPoolsFolder(false);
   if (df) df->DeleteChilds();

   // to be on the safe side, destroy everything in the queue
   ProcessDestroyQueue();

   RemoveProcessorFromThread(true);

   DOUT3(("Done DoHaltManager"));
}


void dabc::Manager::HaltManager()
{
   Execute("HaltManager");

   // here we stopping and delete all threads
   Folder* df = GetThreadsFolder(false);
   DOUT3(("Calling destructor of all working threads %u", (df ? df->NumChilds() : 0)));
   if (df) df->DeleteChilds();

   DOUT3(("dabc::Manager::HaltManager done"));
}

bool dabc::Manager::ConnectPorts(const char* port1name,
                                 const char* port2name,
                                 const char* devname)
{
   return Execute(new CommandPortConnect(port1name, port2name, devname));
}

const char* dabc::Manager::ExtractManagerName(const char* fullname, std::string& managername)
{
   managername = "";
   if ((fullname==0) || (strlen(fullname)==0)) return fullname;

   const char* pos = strchr(fullname, '$');
   if (pos==0) return fullname;

   managername.assign(fullname, pos-fullname);
   pos++;
   if (*pos==0) pos=0;
   return pos;
}

dabc::Command* dabc::Manager::LocalCmd(Command* cmd, const char* fullitemname)
{
   if (cmd==0) return 0;

   // we should always set "_ItemName_" variable to be able identify command
   // later that it is not directly for manager, but for some item in manager

   cmd->SetStr("_ItemName_", fullitemname ? fullitemname : "");

   return cmd;
}

dabc::Command* dabc::Manager::LocalCmd(Command* cmd, Basic* rcv)
{
   if (cmd==0) return 0;

   if ((rcv==0) || (rcv->GetCmdReceiver()==0)) {
      EOUT(("Object cannot be used to recieve commands"));
      dabc::Command::Reply(cmd, false);
      return 0;
   }

   dabc::String s = rcv->GetFullName(this);

   DOUT5(("Redirect cmd %s to item %s", cmd->GetName(), s.c_str()));

   return LocalCmd(cmd, s.c_str());
}

dabc::Command* dabc::Manager::RemoteCmd(Command* cmd, const char* nodename, const char* itemname)
{
   if (cmd==0) return 0;

   if ((nodename==0) || (IsName(nodename)))
      return LocalCmd(cmd, itemname);

   String fullname = nodename;
   fullname.append("$");
   if (itemname!=0) fullname.append(itemname);

   return LocalCmd(cmd, fullname.c_str());
}

dabc::Command* dabc::Manager::RemoteCmd(Command* cmd, int nodeid, const char* itemname)
{
   return RemoteCmd(cmd, GetNodeName(nodeid), itemname);
}

int dabc::Manager::PreviewCommand(Command* cmd)
{
   // check if command dedicated for other node, module and so on
   // return true if command must be redirected

   int cmd_res = cmd_ignore;

   std::string managername;
   const char* fullitemname = cmd->GetPar("_ItemName_");
   const char* itemname = 0;
   if (fullitemname!=0)
      itemname = ExtractManagerName(fullitemname, managername);

   DOUT5(("PreviewCommand cmd:%p %s fullitemname:%s.", cmd, cmd->GetName(), (fullitemname ? fullitemname : "---")));

   if ((managername.size()==0) || (IsName(managername.c_str()))) {

      // this is local command submision
      CommandReceiver* rcv = 0;

      if ((itemname!=0) && (strlen(itemname)>0)) {
         Basic* obj = FindChild(itemname);
         if (obj!=0) rcv = obj->GetCmdReceiver();
      }

      cmd->RemovePar("_ItemName_");

      DOUT5(("Manager get command %s for item %s : %p", cmd->GetName(), (itemname ? itemname : "null"), rcv));

      if (rcv!=0) {
         rcv->Submit(cmd);
         cmd_res = cmd_postponed;
      }

   } else

   // this is remote command submission
   if (!CanSendCmdToManager(managername.c_str())) {
      EOUT(("Cannot submit remote command %s to manager %s", cmd->GetName(), managername.c_str()));
      cmd_res = cmd_false;
   } else {
      AddInternalCmd(cmd, "_cmdid_");
      // remove any node name from modulename
      cmd->SetPar("_ItemName_", itemname);
      // indicate that this is command from specific node to knew about reply destination
      cmd->SetStr("_sendcmd_", GetName());

      std::string sbuf;
      cmd->SaveToString(sbuf);

      if (!SendOverCommandChannel(managername.c_str(), sbuf.c_str())) {
         LockGuard guard(fSendCmdsMutex);
         fSendCommands.remove(cmd);
         cmd_res = cmd_false;
      } else
         cmd_res = cmd_postponed;
   }

   if (cmd_res == cmd_ignore)
      if (cmd->IsName(CommandSetParameter::CmdName())) {
         dabc::Parameter* par = dynamic_cast<dabc::Parameter*>(FindChild(cmd->GetStr("ParName")));
         cmd_res = par ? par->InvokeChange(cmd) : cmd_ignore;
      }

   if (cmd_res == cmd_ignore)
      cmd_res = WorkingProcessor::PreviewCommand(cmd);

   return cmd_res;
}

int dabc::Manager::ExecuteCommand(Command* cmd)
{
   DOUT3(("Manager::Execute %s", cmd->GetName()));

   int cmd_res = cmd_true;

   if (cmd->IsName("HaltManager")) {
      DoHaltManager();
   } else
   if (cmd->IsName(CommandCreateModule::CmdName())) {
      const char* classname = cmd->GetPar("Class");
      const char* modulename = cmd->GetPar("Name");
      const char* thrdname = cmd->GetStr("Thread");

      Module* m = 0;

      dabc::Folder* folder = GetAppFolder(false);
      if ((folder!=0) && (m==0))
         for (unsigned n=0;n<folder->NumChilds();n++) {
            Application* plugin =
               dynamic_cast<dabc::Application*> (folder->GetChild(n));
            if (plugin==0) continue;

            m = plugin->CreateModule(classname, modulename, cmd);
            if (m!=0) break;
         }

      folder = GetFactoriesFolder(false);
      if ((folder!=0) && (m==0))
         for (unsigned n=0;n<folder->NumChilds();n++) {
            Factory* factory =
               dynamic_cast<dabc::Factory*> (folder->GetChild(n));
            if (factory==0) continue;

            m = factory->CreateModule(classname, modulename, cmd);
            if (m!=0) break;
         }

      if (m!=0)
         MakeThreadForModule(m, thrdname);
      else
         EOUT(("Cannot create module of type %s", classname));

      cmd_res = cmd_bool(m!=0);
   } else
   if (cmd->IsName(CmdCreateApplication::CmdName())) {
      const char* classname = cmd->GetStr("AppClass");
      const char* appname = cmd->GetStr("AppName");
      const char* appthrd = cmd->GetStr("AppThrd");

      if ((classname==0) || (strlen(classname)==0)) classname = Factory::DfltAppClass();
      if ((appname!=0) && (strlen(appname)==0)) appname = 0;

      Application* app = GetApp();

      if ((app!=0) && ((appname==0) || (strcmp(app->GetName(), appname)==0))) {
        DOUT4(("Application with name %s already exists", app->GetName()));

      } else {
         if (app!=0) { delete app; app = 0; }

         dabc::Folder* folder = GetFactoriesFolder(false);

         if (folder!=0)
            for (unsigned n=0;n<folder->NumChilds();n++) {
               Factory* factory =
                  dynamic_cast<Factory*> (folder->GetChild(n));
               if (factory!=0)
                  app = factory->CreateApplication(classname, appname, cmd);
               if (app!=0) break;
            }

         if ((appthrd==0) || (strlen(appthrd)==0)) appthrd = MgrThrdName();

         if (app!=0) MakeThreadFor(app, appthrd);
      }

      cmd_res = cmd_bool(app!=0);

   } else

   if (cmd->IsName(CmdCreateDevice::CmdName())) {
      const char* classname = cmd->GetStr("DevClass");
      const char* devname = cmd->GetStr("DevName");
      if (classname==0) classname = "";
      if ((devname==0) || (strlen(devname)==0)) devname = classname;

      Device* dev = FindDevice(devname);

      if (dev!=0) {
        if (strcmp(dev->ClassName(), classname)==0)
           DOUT4(("Device %s of class %s already exists", devname, classname));
        else
           EOUT(("Device %s has other class name %s than requested", devname, dev->ClassName(), classname));
      } else {
         dabc::Folder* folder = GetFactoriesFolder(false);

         if (folder!=0)
            for (unsigned n=0;n<folder->NumChilds();n++) {
               Factory* factory =
                  dynamic_cast<Factory*> (folder->GetChild(n));
               if (factory!=0)
                  dev = factory->CreateDevice(GetDevicesFolder(true), classname, devname, cmd);
               if (dev!=0) break;
            }
      }

       cmd_res = cmd_bool(dev!=0);
   } else
   if (cmd->IsName(CmdCreateThread::CmdName())) {
      const char* thrdname = cmd->GetStr("ThrdName");
      const char* thrdclass = cmd->GetStr("ThrdClass");
      const char* thrddev = cmd->GetStr("ThrdDev");

      cmd_res = cmd_bool(CreateThread(thrdname, thrdclass, 0, thrddev, cmd)!=0);

   } else
   if (cmd->IsName("NullConnect")) {
      Port* port = FindPort(cmd->GetPar("Port"));
      LocalDevice* dev = FindLocalDevice();

      if (dev && port)
         cmd_res = dev->MakeNullTransport(port);
      else
         cmd_res = cmd_false;
   } else
   if (cmd->IsName(CmdCreatePool::CmdName())) {
      const char* poolname = cmd->GetPar("PoolName");
      unsigned numbuffers = cmd->GetUInt("NumBuffers", 0);
      unsigned buffersize = cmd->GetUInt("BufferSize", 0);
      unsigned headersize = cmd->GetInt("HeaderSize", 0);
      cmd_res = cmd_bool(CreateMemoryPool(poolname, buffersize, numbuffers, 0, headersize));
   } else
   if (cmd->IsName("CreateMemoryPools")) {
      cmd_res = DoCreateMemoryPools();
   } else
   if (cmd->IsName("CleanupManager")) {
      cmd_res = DoCleanupManager(cmd->GetInt("AppId", 0));
   } else
   if (cmd->IsName("StartModule")) {
      dabc::Module* m = FindModule(cmd->GetStr("Module",""));
      if (m!=0) m->Start();
//      DOUT1(("Call module start %s", (m ? m->GetName() : "null")));
      cmd_res = cmd_bool(m!=0);
   } else
   if (cmd->IsName("StopModule")) {
      dabc::Module* m = FindModule(cmd->GetStr("Module",""));
      if (m!=0) m->Stop();
//      DOUT1(("Call module stop %s", (m ? m->GetName() : "null")));
      cmd_res = cmd_bool(m!=0);
   } else
   if (cmd->IsName("DeleteModule")) {
      dabc::Module* m = FindModule(cmd->GetStr("Module",""));
      if (m!=0) delete m;
//      DOUT1(("Call module stop %s", (m ? m->GetName() : "null")));
      cmd_res = cmd_bool(m!=0);
   } else
   if (cmd->IsName("StartAllModules")) {
      dabc::Folder* folder = GetModulesFolder(false);

      int appid = cmd->GetInt("AppId", 0);

      if (folder!=0)
        for (unsigned n=0;n<folder->NumChilds();n++) {
           dabc::Module* m = dynamic_cast<dabc::Module*> (folder->GetChild(n));
           if (m==0) {
              EOUT(("Strange object in Modules folder indx %d",n));
              cmd_res = cmd_false;
           } else
           if ((appid<0) || (m->GetAppId() == appid))
              m->Start();
        }
   } else
   if (cmd->IsName("StopAllModules")) {
      dabc::Folder* folder = GetModulesFolder(false);

      int appid = cmd->GetInt("AppId", 0);

      if (folder!=0)
        for (unsigned n=0;n<folder->NumChilds();n++) {
           dabc::Module* m = dynamic_cast<dabc::Module*> (folder->GetChild(n));
           if (m==0) {
              EOUT(("Strange object in Modules folder indx %d",n));
              cmd_res = cmd_false;
           } else
           if ((appid<0) || (m->GetAppId() == appid))
              m->Stop();
        }
   } else
   if (cmd->IsName(CmdCreateDataTransport::CmdName())) {

      const char* portname = cmd->GetStr("PortName","");
      const char* thrdname = cmd->GetPar("ThrdName");
      const char* sourcename = cmd->GetPar("InpName");
      const char* targetname = cmd->GetPar("OutName");

      Port* port = FindPort(portname);

      if (port==0) {
         EOUT(("Port %s not found", portname));
         cmd_res = cmd_false;
      } else {
         DOUT4(("Create data channel for port %s", portname));

         dabc::DataInput* inp = 0;
         dabc::DataOutput* out = 0;

         if (sourcename!=0)
            inp = CreateDataInput(cmd->GetPar("InpType"), sourcename, cmd);

         if (targetname!=0)
            out = CreateDataOutput(cmd->GetPar("OutType"), targetname, cmd);

         DOUT4(( "inp = %p out = %p", inp, out));

         if ((inp!=0) || (out!=0)) {

            LocalDevice* dev = FindLocalDevice();
            DataIOTransport* tr = new DataIOTransport(dev, port, inp, out);

            if (thrdname==0) thrdname = dev->ProcessorThreadName();

            if (MakeThreadFor(tr, thrdname)) {
               port->AssignTransport(tr);
               cmd_res = cmd_true;
            } else
               cmd_res = cmd_false;

         } else {
            EOUT(("No input or output is created"));
            cmd_res = cmd_false;
         }
      }

      DOUT4(( "Result of data transport creation = %s", DBOOL(cmd_res)));
   } else
   if (cmd->IsName(CmdDeletePool::CmdName())) {
      dabc::MemoryPool* pool = FindPool(cmd->GetPar("PoolName"));
      if (pool) delete pool;
   } else
   if (cmd->IsName(CommandStateTransition::CmdName())) {
      InvokeStateTransition(cmd->GetStr("Cmd"), cmd);
      cmd_res = cmd_postponed;
   } else
   if (cmd->IsName("Print")) {
      DoPrint();
   } else

   // these are two special commands with postponed execution
   if (cmd->IsName(CommandPortConnect::CmdName())) {
      std::string manager1name, manager2name;

      const char* port1name = ExtractManagerName(cmd->GetPar("Port1Name"), manager1name);
      const char* port2name = ExtractManagerName(cmd->GetPar("Port2Name"), manager2name);

      DOUT3(("Doing ports connect %s::%s %s::%s",
              (manager1name.length()>0 ? manager1name.c_str() : "---"), port1name,
              (manager2name.length()>0 ? manager2name.c_str() : "---"), port2name));

      // first check local connect
      if ((manager1name.length()==0) && (manager2name.length()==0)) {

         LocalDevice* dev = FindLocalDevice(cmd->GetStr("Device"));
         if (dev==0) dev = FindLocalDevice();

         if (dev) {
            dev->Execute(cmd);
            cmd_res = cmd_postponed;
         } else
            cmd_res = cmd_false;
      } else
      if (!CanSendCmdToManager(manager1name.c_str())) {
         EOUT(("Server node for connection not valid %s",manager1name.c_str()));
         cmd_res = cmd_false;
      } else {

         Command* newcmd = 0;

         String remrecvname;

         if (manager1name == manager2name) {
            newcmd = new CommandPortConnect(port1name, port2name);

         } else {
            if (!CanSendCmdToManager(manager2name.c_str())) {
               EOUT(("Client node for connection not valid %s",manager2name.c_str()));
               return cmd_false;
            }

            remrecvname = String("Devices/") + cmd->GetStr("Device");

            newcmd = new CommandDirectConnect(true, port1name, true);
            // copy all aditional values from
         }

         newcmd->AddValuesFrom(cmd, false);
         int parentid = AddInternalCmd(cmd, "_PCID_");
         newcmd->SetInt("#_PCID_", parentid);
         newcmd->ClearResult();

         if (!SubmitRemote(*this, newcmd, manager1name.c_str(), remrecvname.c_str()))
            EOUT(("Cannot submit remote command"));

         cmd_res = cmd_postponed;
      }
   } else
      cmd_res = cmd_false;

   return cmd_res;
}

bool dabc::Manager::SendOverCommandChannel(const char* managername, const char* cmddata)
{
   EOUT(("NOT implemented"));
   return false;
}

int dabc::Manager::AddInternalCmd(Command* cmd, const char* lblname)
{
   LockGuard guard(fSendCmdsMutex);
   int cmdid = fSendCmdCounter++;
   fSendCommands.push_back(cmd);
   cmd->SetInt(lblname, cmdid);
   cmd->fCleanup = true; // id command will be destroyed, we can remove it from the list
   return cmdid;
}

dabc::Command* dabc::Manager::FindInternalCmd(const char* lblname, int cmdid)
{
   if (cmdid<0) return 0;

   LockGuard guard(fSendCmdsMutex);

   for (unsigned n=0; n<fSendCommands.size();n++) {
      Command* ccc = (Command*) fSendCommands.at(n);
      if (ccc->GetInt(lblname,-1) == cmdid) return ccc;
   }

   return 0;
}

dabc::Command* dabc::Manager::TakeInternalCmd(const char* lblname, int cmdid)
{
   if (cmdid<0) return 0;

   LockGuard guard(fSendCmdsMutex);

   for (unsigned n=0; n<fSendCommands.size();n++) {
      Command* ccc = (Command*) fSendCommands.at(n);
      if (ccc->GetInt(lblname,-1) == cmdid) {
         fSendCommands.remove_at(n);
         ccc->RemovePar(lblname);
         return ccc;
      }
   }

   return 0;
}

void dabc::Manager::RecvOverCommandChannel(const char* cmddata)
{
   Command* cmd = new Command;
   if (!cmd->ReadFromString(cmddata)) {
      dabc::Command::Finalise(cmd);
      EOUT(("Cannot instantiate command from string: %s", cmddata));
      return;
   }

   if (cmd->GetPar("_sendcmd_")) {
      // this is branch for getting commands

      cmd->SetStr("_remotereply_srcid_", cmd->GetStr("_sendcmd_"));
      cmd->RemovePar("_sendcmd_");

      DOUT5(("RecvRemoteCommand %s", cmd->GetName()));

      Submit(Assign(cmd));
   } else {
      // this is for replyes
      int cmdid = cmd->GetInt("_cmdid_",-1);
      bool cmdres = cmd->GetResult();

      Command* initcmd = TakeInternalCmd("_cmdid_", cmdid);

      if (initcmd!=0) {
         DOUT3(("Reply command %s with res %s", initcmd->GetName(), DBOOL(cmdres)));
         initcmd->AddValuesFrom(cmd);
         dabc::Command::Reply(initcmd, cmdres);
      } else
         EOUT(("Did not find initial command with id %d", cmdid));

      dabc::Command::Finalise(cmd);
   }
}

unsigned RoundBufferSize(unsigned bufsize)
{
   if (bufsize==0) return 0;

   unsigned size = 256;
   while (size<bufsize) size*=2;
   return size;
}

dabc::MemoryPool* dabc::Manager::CreateMemoryPool(const char* poolname,
                                                  unsigned buffersize,
                                                  unsigned numbuffers,
                                                  unsigned numincrement,
                                                  unsigned headersize,
                                                  unsigned numsegments)
{
   if (poolname==0) return 0;

   MemoryPool* mem_pool = FindPool(poolname);

   // round always buffer size to 256 limits

   if (mem_pool==0) {

      mem_pool = new dabc::MemoryPool(GetPoolsFolder(true), poolname);

      mem_pool->UseMutex();

      mem_pool->SetMemoryLimit(0); // one can extend pool as much as system can
      mem_pool->SetCleanupTimeout(1.0); // when changed, memory pool can be shrinked again

      mem_pool->AssignProcessorToThread(ProcessorThread());
   }

   if (mem_pool->IsMemLayoutFixed()) {
      if (!mem_pool->CanHasBufferSize(buffersize)) {
         EOUT(("Memory pool structure is fixed and pool will not provide buffers of size %u", buffersize));
      }
      if (!mem_pool->CanHasHeaderSize(headersize)) {
         EOUT(("Memory pool structure is fixed and pool will not provide headers of size %u", headersize));
      }

      return mem_pool;
   }

   if (numbuffers>0) {

      if (buffersize>0) {

         buffersize = RoundBufferSize(buffersize);

         DOUT3(("Create pool:%s buffers:%u number:%u", poolname, buffersize, numbuffers));

         mem_pool->AllocateMemory(buffersize, numbuffers,  numincrement);
      }

      DOUT3(("Create pool:%s references:%u headersize:%u", poolname, numbuffers, headersize));

      mem_pool->AllocateReferences(headersize, numbuffers, numincrement > 0 ? numincrement : numbuffers / 2, numsegments);
   }

   return mem_pool;
}

dabc::MemoryPool* dabc::Manager::ConfigurePool(const char* poolname,
                                               bool fixlayout,
                                               uint64_t size_limit,
                                               double cleanup_timeout)
{
   MemoryPool* mem_pool = FindPool(poolname);
   if (mem_pool!=0) {
      mem_pool->SetMemoryLimit(size_limit);
      mem_pool->SetCleanupTimeout(cleanup_timeout);
      if (fixlayout) {
         DOUT2(("Fix layou of pool %s", poolname));
         mem_pool->SetLayoutFixed();
      }
   }
   return mem_pool;
}

bool dabc::Manager::DoCreateMemoryPools()
{
   const char* selectedname = 0;

   bool res = true;

   dabc::Folder* modules = GetModulesFolder(false);
   if (modules==0) return res;

   do {
      selectedname = 0;
      BufferSize_t selectedsize = 0;
      BufferSize_t headersize = 0;
      BufferNum_t totalbufnum = 0;
      BufferNum_t increment = 0;

      Queue<PoolHandle*> pools(16, true);

      dabc::Iterator iter(modules);

      while (iter.next()) {
         PoolHandle* pool = dynamic_cast<PoolHandle*> (iter.current());
         if ((pool==0) || pool->IsPoolAssigned()) continue;

         if (selectedname==0) {
            selectedname = pool->GetName();
            selectedsize = RoundBufferSize(pool->GetRequiredBufferSize());
         }

         if (pool->IsName(selectedname) &&
             (selectedsize == RoundBufferSize(pool->GetRequiredBufferSize()))) {
               totalbufnum += pool->GetRequiredBuffersNumber();
               if (headersize < pool->GetRequiredHeaderSize()) headersize = pool->GetRequiredHeaderSize();


               if (increment < pool->GetRequiredIncrement()) increment = pool->GetRequiredIncrement();
               pools.Push(pool);
            }
      }

      if ((selectedname!=0) && (pools.Size()>0)) {

         DOUT3(("Start creating pool %s %u x 0x%x, increment: %u", selectedname, totalbufnum, selectedsize, increment));
         MemoryPool* mem_pool =
             CreateMemoryPool(selectedname, selectedsize, totalbufnum, increment, headersize, 8);
         if (mem_pool==0) {
            EOUT(("Was not able to create memory pool %s", selectedname));
            exit(1);
         }
         DOUT3(("Done creating pool %s", selectedname));

         while (pools.Size()>0)
            pools.Pop()->AssignPool(mem_pool);
      }
   } while (selectedname!=0);

   return res;
}

bool dabc::Manager::DoLocalPortConnect(const char* port1name, const char* port2name, const char* devname)
{
   DOUT3(("Doint local connect %s %s dev:%s", port1name, port2name, devname));

   Port* port1 = FindPort(port1name);
   Port* port2 = FindPort(port2name);

   if (port1==0) EOUT(("Cannot find port %s", port1name));
   if (port2==0) EOUT(("Cannot find port %s", port2name));

   LocalDevice* dev = FindLocalDevice(devname);
   if (dev==0) dev = FindLocalDevice();

   return dev ? dev->ConnectPorts(port1, port2) : false;
}

void dabc::Manager::ModuleExecption(Module* m, const char* msg)
{
   EOUT(("EXCEPTION Module: %s message %s", m->GetName(), msg));
}

bool dabc::Manager::PostCommandProcess(Command* cmd)
{
   // PostCommandProcess return true, when command can be safely deleted

   if (cmd->GetPar("_remotereply_srcid_")) {
      String mgrname = cmd->GetStr("_remotereply_srcid_");
      cmd->RemovePar("_remotereply_srcid_");

      String sbuf;
      cmd->SaveToString(sbuf);

      SendOverCommandChannel(mgrname.c_str(), sbuf.c_str());

      return true;
   } else
   if (cmd->IsName(CommandPortConnect::CmdName())) {

      int parentid = cmd->GetInt("#_PCID_", -1);
      if (parentid<0) return true;

      Command* prnt = TakeInternalCmd("_PCID_", parentid);
      dabc::Command::Reply(prnt, cmd->GetResult());
   } else

   if (cmd->IsName(CommandDirectConnect::CmdName())) {

      int parentid = cmd->GetInt("#_PCID_", -1);
      if (parentid<0) return true;

//      DOUT1(("Reply of CommandDirectConnect parent = %d", parentid));

      bool res = cmd->GetResult();
      cmd->ClearResult();

      if (cmd->GetBool("ClientSide", false) || !res) {
         Command* prnt = TakeInternalCmd("_PCID_", parentid);
         dabc::Command::Reply(prnt, res);
         return true;
      }

      Command* prnt = FindInternalCmd("_PCID_", parentid);
      if (prnt==0) {
         EOUT(("Parent %d is dissaper, parentid"));
         return true;
      }

      std::string manager2name;
      const char* port2name = ExtractManagerName(prnt->GetPar("Port2Name"), manager2name);

      Command* newcmd = new CommandDirectConnect(false, port2name, false);
      newcmd->AddValuesFrom(prnt, false); // copy initial settings
      newcmd->AddValuesFrom(cmd, false);  // copy new values from first command
      newcmd->SetInt("#_PCID_", parentid);
      newcmd->SetBool("ClientSide", true);
      newcmd->ClearResult();

      String devname("Devices/"); devname += prnt->GetStr("Device");

      if (!SubmitRemote(*this, newcmd, manager2name.c_str(), devname.c_str())) {
         Command* prnt = TakeInternalCmd("_PCID_", parentid);
         dabc::Command::Reply(prnt, false);
      }
   }

   return true;
}

void dabc::Manager::StartModule(const char* modulename)
{
   Execute(new dabc::CommandStartModule(modulename));
}

void dabc::Manager::StopModule(const char* modulename)
{
   Execute(new dabc::CommandStopModule(modulename));
}

bool dabc::Manager::StartAllModules(int appid)
{
   return Execute(new CmdStartAllModules(appid));
}

bool dabc::Manager::StopAllModules(int appid)
{
   return Execute(new CmdStopAllModules(appid));
}

bool dabc::Manager::CreateMemoryPools()
{
   return Execute("CreateMemoryPools");
}

bool dabc::Manager::CleanupManager(int appid)
{
   // this method must delete all threads, modules and pools and clean device drivers

   return Execute(new CmdCleanupManager(appid));
}

void dabc::Manager::Print()
{
   Execute("Print");
}

bool dabc::Manager::_ProcessReply(Command* cmd)
{
   DOUT4(("ProcessReply %s evnt:%d", cmd->GetName(), evntManagerReply));

   {
      LockGuard lock(fMgrMutex);
      fReplyesQueue.Push(cmd);
   }

   FireEvent(evntManagerReply);
   return true;
}

void dabc::Manager::DestroyObject(Basic* obj)
{
   DOUT5(("DeleteObject fire %p", obj));

   bool dofire = false;

   {
      LockGuard lock(fMgrMutex);

      // remove object from parent list to avoid situation,
      // that parent is destroyed before object destructor called

      if (obj->GetParent())
         obj->GetParent()->RemoveChild(obj);

      dofire = fDestroyQueue.Size() == 0;

      fDestroyQueue.Push(obj);
   }

   if (dofire) FireEvent(evntDestroyObj);
}

void dabc::Manager::DoCleanupThreads()
{
   Folder* thf = GetThreadsFolder();
   if (thf==0) return;

   DOUT3(("DoCleanupThreads start"));

   for (unsigned n=0;n<thf->NumChilds();n++) {
     WorkingThread* thrd = dynamic_cast<WorkingThread*> (thf->GetChild(n));
     if (thrd==0) continue;

     DOUT5(("DoCleanupThread thrd:%s", thrd->GetName()));

     if (thrd->NoLongerUsed()) {
        DOUT1(("!!! Thread %s has nothing to do, delete", thrd->GetName()));

        thrd->Stop();
        delete thrd;
        n--;
     }
   }

   DOUT3(("DoCleanupThreads finish"));
}

void dabc::Manager::DoCleanupDevices(bool force)
{
   Folder* df = GetDevicesFolder();
   if (df==0) return;
   for (unsigned n=0;n<df->NumChilds();n++) {
      Device* device = dynamic_cast<Device*> (df->GetChild(n));
      if (device==0) continue;

      DOUT3(("DoCleanupDevices dev:%s", device->GetName()));
      device->CleanupDevice(force);
      DOUT3(("DoCleanupDevices dev:%s done", device->GetName()));
   }

//   ProcessDestroyQueue();
}


bool dabc::Manager::DoCleanupManager(int appid)
{
//   SetDebugLevel(5);

   DOUT3(("DoCleanupManager appid = %d", appid));

   // than we can safely delete all modules, while no execution can happen

   Folder* mf = GetModulesFolder();
   if (mf) mf->DeleteChilds(appid);

   DOUT3(( "Cleanup all devices"));
   DoCleanupDevices(false);

   // here we delete all pools

   DOUT3(("Deleting app pools"));
   Folder* pf = GetPoolsFolder();
   if (pf) pf->DeleteChilds(appid);

   pf = GetDevicesFolder();
   DOUT3(( "Deleting app devices num = %u", (pf ? pf->NumChilds() : 0)));
   if (pf) pf->DeleteChilds(appid);

   DOUT3(( "Cleanup all working threads"));

   DoCleanupThreads();

   DOUT3(("DoCleanupManager appid = %d done", appid));

//   SetDebugLevel(1);

   return true;
}

dabc::WorkingThread* dabc::Manager::FindThread(const char* name, const char* required_class)
{
   Folder* f = GetThreadsFolder();
   if (f==0) return 0;

   dabc::WorkingThread* thrd = dynamic_cast<WorkingThread*> (f->FindChild(name));

   if (thrd == 0) return 0;

   if (!thrd->CompatibleClass(required_class)) return 0;

   return thrd;
}

dabc::WorkingThread* dabc::Manager::CreateThread(const char* thrdname, const char* thrdclass, unsigned startmode, const char* thrddev, Command* cmd)
{
   WorkingThread* thrd = FindThread(thrdname);

   if (thrd!=0) {
      if (thrd->CompatibleClass(thrdclass)) {
      } else {
         EOUT(("Thread %s of class %s exists and incompatible with %s class",
            thrdname, thrd->ClassName(), (thrdname ? thrdname : "---")));
         thrd = 0;
      }
      return thrd;
   }

   DOUT4(("CreateThread %s of class %s, is any %p", thrdname, (thrdclass ? thrdclass : "---"), thrd));

   dabc::Folder* folder = GetFactoriesFolder(false);

   if (folder!=0)
      for (unsigned n=0;n<folder->NumChilds();n++) {
         Factory* factory =
            dynamic_cast<Factory*> (folder->GetChild(n));
         if (factory!=0)
            thrd = factory->CreateThread(GetThreadsFolder(true), thrdclass, thrdname, thrddev, cmd);
         if (thrd!=0) break;
      }

    if (thrd!=0)
       if (!thrd->Start(10, startmode != 0)) {
          EOUT(("Thread %s cannot be started!!!", thrdname));
          delete thrd;
          thrd = 0;
       }

   DOUT4(("Create thread %s of class %s thrd %p", thrdname, (thrdclass ? thrdclass : "---"), thrd));

   return thrd;
}

bool dabc::Manager::MakeThreadFor(WorkingProcessor* proc, const char* thrdname, unsigned startmode)
{
   if (proc==0) return false;

   String newname;

   if ((thrdname==0) || (strlen(thrdname)==0)) {
      EOUT(("Thread name not specified - generate default"));

      int cnt = 0;
      do {
         dabc::formats(newname, "Thread%d", cnt++);
         thrdname = newname.c_str();
      } while (FindThread(thrdname));
   }

   WorkingThread* thrd = CreateThread(thrdname, proc->RequiredThrdClass(), startmode);

   return thrd ? proc->AssignProcessorToThread(thrd) : false;
}

bool dabc::Manager::MakeThreadForModule(Module* m, const char* thrdname)
{
   if (m==0) return false;

   if (thrdname==0) thrdname = m->GetName();

   return MakeThreadFor(m, thrdname);
}

const char* dabc::Manager::CurrentThrdName()
{
   Folder* f = GetThreadsFolder(false);
   if (f==0) return "no threrads";
   for (unsigned n=0; n < f->NumChilds(); n++) {
      WorkingThread* thrd = dynamic_cast<WorkingThread*> (f->GetChild(n));
      if (thrd && thrd->IsItself()) return thrd->GetName();
   }
   return "uncknown";
}

void dabc::Manager::RunManagerMainLoop()
{
   WorkingThread* thrd = ProcessorThread();

   if (thrd==0) return;

   if (thrd->IsReadThrd()) {
      DOUT3(("Manager has normal thread - just wait until application modules are stopped"));
      while(fMgrMainLoop) { dabc::LongSleep(1); }
   } else {
      DOUT3(("Manager uses process as thread - run mainloop ourself"));

      thrd->MainLoop();

      DOUT3(("Manager is stopped - just keep process as it is"));// make virtual run, while thread in reality is running

      thrd->Start(-1, true);
   }
}

void dabc::Manager::DoPrint()
{
   dabc::Iterator iter(GetThreadsFolder(false));

   while (iter.next()) {
      WorkingThread* thrd = dynamic_cast<WorkingThread*> (iter.current());
      if (thrd) DOUT1(("Thrd: %s", thrd->GetName()));
   }

   dabc::Iterator iter2(GetModulesFolder(false));

   while (iter2.next()) {
      Module* m = dynamic_cast<Module*> (iter2.current());
      if (m) DOUT1(("Module: %s", m->GetName()));
   }
}

int dabc::Manager::NumActiveNodes()
{
   int cnt = 0;
   for (int n=0;n<NumNodes();n++)
      if (IsNodeActive(n)) cnt++;
   return cnt;
}

int dabc::Manager::DefineNodeId(const char* nodename)
{
   if ((nodename==0) || (strlen(nodename)==0)) return -1;
   for (int n=0;n<NumNodes();n++) {
      const char* name = GetNodeName(n);
      if (name)
         if (strcmp(name, nodename)==0) return n;
   }
   return -1;
}

bool dabc::Manager::InvokeStateTransition(const char* state_transition_name, Command* cmd)
{
   if (fSMmodule!=0) {

      if ((state_transition_name==0) || (*state_transition_name == 0)) {
         EOUT(("State transition is not specified"));
         dabc::Command::Reply(cmd, false);
         return false;
      }

      if (cmd==0) cmd = new CommandStateTransition(state_transition_name);

      fSMmodule->Submit(cmd);
      DOUT3(("Submit state transition %s", state_transition_name));
      return true;
   }

   dabc::Command::Reply(cmd, false);

   if ((state_transition_name!=0) && (strcmp(state_transition_name, stcmdDoStop)==0)) {
      DOUT1(("Application can stops its execution"));

      fMgrMainLoop = false;

      WorkingThread* thrd = ProcessorThread();
      if (thrd)
        if (!thrd->IsReadThrd()) {
           RemoveProcessorFromThread(true);
           thrd->Stop(false); // stop thread - means leave thread main loop
        }
   }

   return false;
}

const char* dabc::Manager::CurrentState() const
{
   return GetParCharStar(stParName, stNull);
}

bool dabc::Manager::DoStateTransition(const char* stcmd)
{
   // must be called from SM thread

   Application* plugin = GetApp();

   if (plugin==0) return false;


   const char* tgtstate = TargetStateName(stcmd);
/////////// take this out, already handled in xdaq fsm JA
//
//   if (strcmp(CurrentState(), tgtstate)==0) {
//      DOUT1(("SM Command %s leads to current state, do nothing", stcmd));
//      return true;
//   }
//
//   bool allowed = false;
//
//   if ((strcmp(CurrentState(), stHalted)==0) && (strcmp(stcmd, stcmdDoConfigure)==0))
//     allowed = true;
//   else
//   if ((strcmp(CurrentState(), stConfigured)==0) && (strcmp(stcmd, stcmdDoEnable)==0))
//      allowed = true;
//   else
//   if ((strcmp(CurrentState(), stReady)==0) && (strcmp(stcmd, stcmdDoStart)==0))
//      allowed = true;
//   else
//   if ((strcmp(CurrentState(), stRunning)==0) && (strcmp(stcmd, stcmdDoStop)==0))
//      allowed = true;
//   else
//   if (strcmp(stcmd, stcmdDoHalt)==0)
//      allowed = true;
//
//   if (!allowed) {
//      EOUT(("Command %s not allowed at state %s", stcmd, CurrentState()));
//      return cmd_false;
//   }
////////////////////////////////

   if (!plugin->DoStateTransition(stcmd)) return false;

   return Execute(new CommandSetParameter(stParName, tgtstate));
}


bool dabc::Manager::RegisterDependency(Basic* src, Basic* tgt)
{
   if ((src==0) || (tgt==0)) return false;

   src->fCleanup = true;
   tgt->fCleanup = true;

   DependPair rec;
   rec.src = src;
   rec.tgt = tgt;

   LockGuard guard(fMgrMutex);

   fDepend->push_back(rec);

   return true;
}

bool dabc::Manager::UnregisterDependency(Basic* src, Basic* tgt)
{
   if ((src==0) || (tgt==0)) return false;

   LockGuard guard(fMgrMutex);
   DependPairList::iterator iter = fDepend->begin();
   while (iter != fDepend->end()) {
      if ((iter->src == src) && (iter->tgt == tgt))
         fDepend->erase(iter++);
      else
         iter++;
   }

   return true;
}


void dabc::Manager::ObjectDestroyed(Basic* obj)
{
   if (obj==0) return;

   std::vector<Basic*> info;

   {
      LockGuard guard(fMgrMutex);
      DependPairList::iterator iter = fDepend->begin();
      while (iter != fDepend->end()) {
         if ((iter->src == obj) || (iter->tgt == obj)) {
           if (iter->tgt == obj) info.push_back(iter->src);
           fDepend->erase(iter++);
         } else
           iter++;
      }
   }

   for (unsigned n=0;n<info.size();n++)
      info[n]->DependendDestroyed(obj);

   // protect ourself from the situation,
   // when command deleted by any means before reply from dependent command is coming
   if (dynamic_cast<Command*> (obj)) {
      LockGuard guard(fSendCmdsMutex);
      fSendCommands.remove(obj);
   }
}

void dabc::Manager::ParameterEvent(Parameter* par, int event)
{
   if (event==1) return;

   bool activate = false;
   double interval = 0.;

   {
      LockGuard lock(fMgrMutex);

      if ((event==0) && par->NeedTimeout()) {
         if (fTimedPars.size()==0) { activate = true; interval = 0.; }
         fTimedPars.push_back(par);
      } else
      if (event==2) {
         fTimedPars.remove(par);
         if (fTimedPars.size()==0) { activate = true; interval = -1.; }
      }
   }

   if (activate) ActivateTimeout(interval);
}

double dabc::Manager::ProcessTimeout(double last_diff)
{
   LockGuard lock(fMgrMutex);

   for (unsigned n=0;n<fTimedPars.size();n++)
      if (fTimedPars[n]!=0)
         ((Parameter*) fTimedPars[n])->ProcessTimeout(last_diff);

   return fTimedPars.size()==0 ? -1. : 1.;
}


void dabc::Manager::AddFactory(Factory* factory)
{
   if (factory==0) return;

   Folder* f = GetFactoriesFolder(true);
   f->AddChild(factory);

   DOUT3(("Add factory %s", factory->GetName()));
}

bool dabc::Manager::CreateApplication(const char* classname, const char* appname, const char* appthrd, Command* cmd)
{
   if (cmd==0)
      cmd = new CmdCreateApplication(classname, appname, appthrd);
   else {
      if (!cmd->IsName(CmdCreateApplication::CmdName())) {
          EOUT(("Wrong command name %s", cmd->GetName()));
          dabc::Command::Reply(cmd, false);
          return false;
      }
      CmdCreateApplication::SetArguments(cmd, classname, appname, appthrd);
   }

   return Execute(cmd);
}

bool dabc::Manager::CreateDevice(const char* classname, const char* devname, Command* cmd)
{
   if (cmd==0)
      cmd = new CmdCreateDevice(classname, devname);
   else {
      if (!cmd->IsName(CmdCreateDevice::CmdName())) {
         EOUT(("Wrong command name %s", cmd->GetName()));
         dabc::Command::Reply(cmd, false);
         return false;
      }

      CmdCreateDevice::SetArguments(cmd, classname, devname);
   }

   return Execute(cmd);
}

bool dabc::Manager::CreateModule(const char* classname, const char* modulename, const char* thrdname, Command* cmd)
{
   if (cmd==0)
      cmd = new CommandCreateModule(classname, modulename, thrdname);
   else {
      if (!cmd->IsName(CommandCreateModule::CmdName())) {
         EOUT(("Wrong command name %s", cmd->GetName()));
         dabc::Command::Reply(cmd, false);
         return false;
      }

      CommandCreateModule::SetArguments(cmd, classname, modulename, thrdname);
   }

   return Execute(cmd);
}

bool dabc::Manager::CreateTransport(const char* devicename, const char* portname, Command* cmd)
{
   Device* dev = FindDevice(devicename);
   if (dev==0) {
      EOUT(("Device %s not exists", devicename));
      dabc::Command::Reply(cmd, false);
      return false;
   }

   if (cmd==0)
      cmd = new CmdCreateTransport(portname);
   else {
       if (!cmd->IsName(CmdCreateTransport::CmdName())) {
           EOUT(("Wrong command name %s", cmd->GetName()));
           dabc::Command::Reply(cmd, false);
           return false;
       }

       CmdCreateTransport::SetArguments(cmd, portname);
   }

   return dev->Execute(cmd);
}

bool dabc::Manager::CreateDataInputTransport(const char* portname, const char* thrdname,
                                             const char* typ, const char* name, Command* cmd)
{
   if (cmd==0)
      cmd = new CmdCreateDataTransport();
   else
      if (!cmd->IsName(CmdCreateDataTransport::CmdName())) {
         EOUT(("Wrong command name %s", cmd->GetName()));
         dabc::Command::Reply(cmd, false);
         return false;
      }

   CmdCreateDataTransport::SetArgs(cmd, portname, thrdname);
   CmdCreateDataTransport::SetArgsInp(cmd, typ, name);

   return Execute(cmd);
}

bool dabc::Manager::CreateDataOutputTransport(const char* portname, const char* thrdname,
                                              const char* typ, const char* name, Command* cmd)
{
   if (cmd==0)
      cmd = new CmdCreateDataTransport();
   else
      if (!cmd->IsName(CmdCreateDataTransport::CmdName())) {
         EOUT(("Wrong command name %s", cmd->GetName()));
         dabc::Command::Reply(cmd, false);
         return false;
      }

   CmdCreateDataTransport::SetArgs(cmd, portname, thrdname);
   CmdCreateDataTransport::SetArgsOut(cmd, typ, name);

   return Execute(cmd);
}

bool dabc::Manager::CreateDataIOTransport(const char* portname, const char* thrdname,
                                          const char* inp_typ, const char* inp_name,
                                          const char* out_typ, const char* out_name,
                                          Command* cmd)
{
   if (cmd==0)
      cmd = new CmdCreateDataTransport();
   else
      if (!cmd->IsName(CmdCreateDataTransport::CmdName())) {
         EOUT(("Wrong command name %s", cmd->GetName()));
         dabc::Command::Reply(cmd, false);
         return false;
      }

   CmdCreateDataTransport::SetArgs(cmd, portname, thrdname);
   CmdCreateDataTransport::SetArgsInp(cmd, inp_typ, inp_name);
   CmdCreateDataTransport::SetArgsOut(cmd, out_typ, out_name);

   return Execute(cmd);
}


dabc::FileIO* dabc::Manager::CreateFileIO(const char* typ, const char* name, int option)
{
   Folder* folder = GetFactoriesFolder(false);
   if (folder==0) return 0;

   for (unsigned n=0;n<folder->NumChilds();n++) {
      Factory* factory =
         dynamic_cast<dabc::Factory*> (folder->GetChild(n));
      if (factory==0) continue;

      FileIO* io = factory->CreateFileIO(typ, name, option);
      if (io!=0) return io;
   }

   return 0;
}

dabc::Folder* dabc::Manager::ListMatchFiles(const char* typ, const char* filemask)
{
   Folder* folder = GetFactoriesFolder(false);
   if (folder==0) return 0;

   for (unsigned n=0;n<folder->NumChilds();n++) {
      Factory* factory =
         dynamic_cast<dabc::Factory*> (folder->GetChild(n));
      if (factory==0) continue;

      Folder* res = factory->ListMatchFiles(typ, filemask);
      if (res!=0) return res;
   }

   return 0;
}

dabc::DataInput* dabc::Manager::CreateDataInput(const char* typ, const char* name, Command* cmd)
{
   Folder* folder = GetFactoriesFolder(false);
   if (folder==0) return 0;

   for (unsigned n=0;n<folder->NumChilds();n++) {
      Factory* factory =
         dynamic_cast<dabc::Factory*> (folder->GetChild(n));
      if (factory==0) continue;

      DataInput* inp = factory->CreateDataInput(typ, name, cmd);
      if (inp!=0) return inp;
   }

   return 0;
}

dabc::DataOutput* dabc::Manager::CreateDataOutput(const char* typ, const char* name, Command* cmd)
{
   Folder* folder = GetFactoriesFolder(false);
   if (folder==0) return 0;

   for (unsigned n=0;n<folder->NumChilds();n++) {
      Factory* factory =
         dynamic_cast<dabc::Factory*> (folder->GetChild(n));
      if (factory==0) continue;

      DataOutput* out = factory->CreateDataOutput(typ, name, cmd);
      if (out!=0) return out;
   }

   return 0;
}

void* dabc::Manager::FindXmlContext(void* engine, void* doc, unsigned cnt, const char* context, bool showerr)
{
   if ((engine==0) || (doc==0)) {
      if (showerr) EOUT(("engine or doc are not speciefied"));
      return 0;
   }

   dabc::XmlEngine* xml = (dabc::XmlEngine*) engine;

   dabc::XMLNodePointer_t rootnode = xml->DocGetRootElement((dabc::XMLDocPointer_t)doc);
   if (strcmp(xml->GetNodeName(rootnode), "Partition")!=0) {
      if (showerr) EOUT(("Instead of Partition root node %s found", xml->GetNodeName(rootnode)));
      return 0;

   }

   dabc::XMLNodePointer_t contextnode = xml->GetChild(rootnode);

   unsigned icnt = cnt;

   while (contextnode!=0) {
      if (strcmp(xml->GetNodeName(contextnode), "Context")!=0) {
         if (showerr) EOUT(("Instead of Context node %s found", xml->GetNodeName(contextnode)));
         return 0;
      }

      bool findcontext = false;

      if (context==0)
         findcontext = (icnt-- == 0);
      else
         findcontext = (strcmp(xml->GetAttr(contextnode, "url"), context) == 0);

      if (findcontext) return contextnode;

      contextnode = xml->GetNext(contextnode);
   }

   if (showerr) EOUT(("Specified context not found"));

   return 0;
}

bool dabc::Manager::LoadLibrary(const char* libname)
{
   String name = libname;

   while (name.find("${") != name.npos) {
      size_t pos1 = name.find("${");
      size_t pos2 = name.find("}");

      if (pos1>pos2) {
         EOUT(("Wrnog variable parentesis %s", name.c_str()));
         return false;
      }

      String var(name, pos1+2, pos2-pos1-2);

      name.erase(pos1, pos2-pos1+1);

      if (var.length()>0) {
         const char* value = getenv(var.c_str());
         if (value==0)
            if (var=="DABCSYS") value = ".";

         DOUT3(("LoadLibrary:: Replace variable %s with value %s", var.c_str(), value));

         if (value!=0) name.insert(pos1, value);
      }
   }

   void* lib = ::dlopen(name.c_str(), RTLD_LAZY | RTLD_GLOBAL);

   if (lib==0)
      EOUT(("Cannot load library %s err:%s", name.c_str(), ::dlerror()));
   else
      DOUT1(("Library loaded %s", name.c_str()));

   return (lib!=0);
}

unsigned dabc::Manager::Read_XDAQ_XML_NumNodes(const char* fname)
{
   dabc::XmlEngine xml;

   dabc::XMLDocPointer_t doc = xml.ParseFile(fname, false);

   unsigned cnt = 0;

   while (FindXmlContext(&xml, doc, cnt, 0, false) != 0) cnt++;

   xml.FreeDoc(doc);

   return cnt;
}

dabc::String dabc::Manager::Read_XDAQ_XML_NodeName(const char* fname, unsigned cnt)
{
   dabc::XmlEngine xml;

   dabc::String res("error");

   dabc::XMLDocPointer_t doc = xml.ParseFile(fname, false);

   dabc::XMLNodePointer_t contextnode =
      (dabc::XMLNodePointer_t) FindXmlContext(&xml, doc, cnt, 0, false);

   const char* url = contextnode ? xml.GetAttr(contextnode, "url") : 0;

   if (url!=0)
       if (strstr(url,"http://")==url) {
          url += 7;
          const char* pos = strstr(url, ":");
          if (pos==0) res = url;
                 else res.assign(url, pos-url);
       }

   xml.FreeDoc(doc);
   return res;
}


bool dabc::Manager::Read_XDAQ_XML_Libs(const char* fname, unsigned cnt)
{
   dabc::XmlEngine xml;

   dabc::XMLDocPointer_t doc = xml.ParseFile(fname);

   if (doc==0) {
      EOUT(("Not able to open/parse xml file %s", fname));
      return false;
   }

   dabc::XMLNodePointer_t contextnode =
      (dabc::XMLNodePointer_t) FindXmlContext(&xml, doc, cnt);

   if (contextnode==0) {
      xml.FreeDoc(doc);
      return false;
   }

   dabc::XMLNodePointer_t modnode = xml.GetChild(contextnode);

   while (modnode!=0) {
      if (strcmp(xml.GetNodeName(modnode), "Module")==0) {

         const char* libname = xml.GetNodeContent(modnode);

         if ((strstr(libname,"libdim")==0) &&
             (strstr(libname,"libDabcBase")==0) &&
             (strstr(libname,"libDabcXDAQControl")==0))
                LoadLibrary(libname);
      }

      modnode = xml.GetNext(modnode);
   }

   xml.FreeDoc(doc);
   return true;
}


bool dabc::Manager::Read_XDAQ_XML_Pars(const char* fname, unsigned cnt)
{
   dabc::XmlEngine xml;

   dabc::XMLDocPointer_t doc = xml.ParseFile(fname);

   if (doc==0) {
      EOUT(("Not able to open/parse xml file %s", fname));
      return false;
   }

   dabc::XMLNodePointer_t contextnode =
      (dabc::XMLNodePointer_t) FindXmlContext(&xml, doc, cnt);

   if (contextnode==0) {
      xml.FreeDoc(doc);
      return false;
   }

   dabc::XMLNodePointer_t appnode = xml.GetChild(contextnode);
   if (strcmp(xml.GetNodeName(appnode), "Application")!=0) {
      EOUT(("Application node in context not found"));
      xml.FreeDoc(doc);
      return false;
   }

   dabc::XMLNodePointer_t propnode = xml.GetChild(appnode);
   if (strcmp(xml.GetNodeName(propnode), "properties")!=0) {
      EOUT(("properties node in Application not found"));
      xml.FreeDoc(doc);
      return false;
   }

   dabc::XMLNodePointer_t parnode = xml.GetChild(propnode);

   Application* plugin = GetApp();

   while (parnode != 0) {
      const char* parname = xml.GetNodeName(parnode);
      const char* parvalue = xml.GetNodeContent(parnode);
//      const char* partyp = xml.GetAttr(parnode, "xsi:type");

      if (strcmp(parname,"debugLevel")==0) {
         dabc::SetDebugLevel(atoi(parvalue));
         DOUT1(("Set debug level = %s", parvalue));
      } else
      if (strcmp(parname,"nodeId")==0) {
         DOUT1(("Ignore set of node id %s", parvalue));
      } else {
         const char* separ = strchr(parname, '.');
         if ((separ!=0) && (plugin!=0)) {
            dabc::String shortname(parname, separ-parname);
            const char* ownername = separ+1;

            if (plugin->IsName(ownername)) {
               Parameter* par = plugin->FindPar(shortname.c_str());
               if (par!=0) {
                  par->SetStr(parvalue);
                  DOUT1(("Set parameter %s = %s", parname, parvalue));
               }
               else
                  EOUT(("Not found parameter %s in plugin", shortname.c_str()));
            } else {
               EOUT(("Not find owner %s for parameter %s", ownername, parname));
            }
         }
      }

      parnode = xml.GetNext(parnode);
   }

   xml.FreeDoc(doc);
   return true;
}

void dabc_Manager_CtrlCHandler(int number)
{
   if (dabc::Manager::Instance())
      dabc::Manager::Instance()->ProcessCtrlCSignal();
   else {
      DOUT0(("Ctrl-C pressed, but no manager found !!!"));
      exit(0);
   }
}


bool dabc::Manager::InstallCtrlCHandler()
{
   if (fSigThrd!=0) {
      EOUT(("Signal handler was already installed !!!"));
      return false;
   }

   fSigThrd = dabc::Thread::Self();

   if (signal(SIGINT, dabc_Manager_CtrlCHandler)==SIG_ERR) {
      EOUT(("Cannot change handler for SIGINT"));
      return false;
   }

   return true;
}

void dabc::Manager::RaiseCtrlCSignal()
{
   if (fSigThrd==0)
      EOUT(("No signal handler installed - it is dangerous !!!"));
   else
      DOUT1(("dabc::Manager::RaiseCtrlCSignal"));

   kill(0, SIGINT);
}


void dabc::Manager::ProcessCtrlCSignal()
{
   if (fSigThrd != dabc::Thread::Self()) return;

   if (strcmp(CurrentState(), stRunning)==0) {
      DOUT1(("First, stop manager"));
      if(!DoStateTransition(stcmdDoStop)) {
         EOUT(("State transition % failed. Abort", stcmdDoStop));
         exit(1);
      }
   }

   DOUT1(("Now halt manager"));
   if(!DoStateTransition(stcmdDoHalt)) {
      EOUT(("State transition % failed. Abort", stcmdDoHalt));
      exit(1);
   }

   HaltManager();

   CancelCommands();

   DeleteChilds();

   dabc::Logger::Instance()->ShowStat();

   DOUT0(("Normal exit after Ctrl-C"));

   exit(0);
}

