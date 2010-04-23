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
#include "dabc/Manager.h"

#include <list>
#include <vector>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

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
#include "dabc/Factory.h"
#include "dabc/Application.h"
#include "dabc/Iterator.h"
#include "dabc/Parameter.h"
#include "dabc/FileIO.h"
#include "dabc/BinaryFile.h"
#include "dabc/DataIOTransport.h"
#include "dabc/Configuration.h"

#include "dabc/TimeSyncModule.h"
#include "dabc/StateMachineModule.h"
#include "dabc/CpuInfoModule.h"

namespace dabc {

   class StdManagerFactory : public Factory {
      public:
         StdManagerFactory(const char* name) :
            Factory(name)
         {
         }

         virtual Module* CreateModule(const char* classname, const char* modulename, Command* cmd);

         virtual Device* CreateDevice(const char* classname,
                                      const char* devname,
                                      Command*);

         virtual WorkingThread* CreateThread(const char* classname, const char* thrdname, const char* thrddev, Command* cmd);

         virtual FileIO* CreateFileIO(const char* typ, const char* name, int option);
         virtual Folder* ListMatchFiles(const char* typ, const char* filemask);

         virtual Transport* CreateTransport(Port* port, const char* typ, const char* thrdname, Command* cmd);

      protected:
         virtual bool CreateManagerInstance(const char* kind, Configuration* cfg);
   };

   struct DependPair {
      Basic* src;
      Basic* tgt;
   };

   class DependPairList : public std::list<DependPair> {};

}

dabc::Module* dabc::StdManagerFactory::CreateModule(const char* classname, const char* modulename, Command* cmd)
{
   if (strcmp(classname, "dabc::TimeSyncModule")==0)
      return new TimeSyncModule(modulename, cmd);
   else
   if (strcmp(classname, "dabc::CpuInfoModule")==0)
      return new CpuInfoModule(modulename, cmd);

   return 0;
}

dabc::Device* dabc::StdManagerFactory::CreateDevice(const char* classname,
                                                    const char* devname,
                                                    Command*)
{
   if (strcmp(classname, dabc::typeSocketDevice)==0)
      return new SocketDevice(dabc::mgr()->GetDevicesFolder(true), devname);
   else
   if (strcmp(classname, "LocalDevice")==0)
      return new LocalDevice(dabc::mgr()->GetDevicesFolder(true), devname);
   return 0;
}

dabc::Transport* dabc::StdManagerFactory::CreateTransport(Port* port, const char* typ, const char* thrdname, Command* cmd)
{
   if (strcmp(typ,typeNullTransport)!=0) return 0;

   dabc::Device* dev = dabc::mgr()->FindLocalDevice();
   if (dev==0) {
      EOUT(("Local device not found"));
      return 0;
   }

   return new NullTransport((dabc::LocalDevice*) dev);
}

dabc::WorkingThread* dabc::StdManagerFactory::CreateThread(const char* classname, const char* thrdname, const char* thrddev, Command* cmd)
{
   if ((classname==0) || (strlen(classname)==0) || (strcmp(classname, typeWorkingThread)==0))
      return new WorkingThread(dabc::mgr()->GetThreadsFolder(true), thrdname, cmd ? cmd->GetInt("NumQueues", 3) : 3);
   else
   if (strcmp(classname, typeSocketThread)==0)
      return new SocketThread(dabc::mgr()->GetThreadsFolder(true), thrdname, cmd ? cmd->GetInt("NumQueues", 3) : 3);

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

   std::string pathname;
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
         std::string fullitemname;
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

bool dabc::StdManagerFactory::CreateManagerInstance(const char* kind, Configuration* cfg)
{
   if ((kind==0) || (strcmp(kind, "Basic")==0) || (strcmp(kind, "BasicExtra")==0)) {
      bool usecurrentrocess = (kind!=0) && (strcmp(kind, "BasicExtra")==0);
      new dabc::Manager(cfg ? cfg->MgrName() : "mgr", usecurrentrocess, cfg);
      return true;
   }

   return false;
}

dabc::StdManagerFactory stdfactory("std");

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

dabc::Manager::Manager(const char* managername, bool usecurrentprocess, Configuration* cfg) :
   Folder(0, managername, true),
   WorkingProcessor(this),
   CommandClientBase(),
   fMgrMainLoop(false),
   fMgrNormalThrd(!usecurrentprocess),
   fMgrMutex(0),
   fReplyesQueue(true, false),
   fDestroyQueue(16, true),
   fParsQueue(1024, true),
   fParsQueueBlocked(true),
   fSendCmdsMutex(0),
   fSendCmdCounter(0),
   fSendCommands(),
   fTimedPars(),
   fDepend(0),
   fSigThrd(0),
   fSMmodule(0),
   fCfg(cfg),
   fCfgHost()
{
   if (fInstance==0) {
      fInstance = this;
      dabc::SetDebugPrefix(managername);
   }

   if (cfg) fCfgHost = cfg->MgrHost();

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
//      static StdManagerFactory stdfactory("std");

      Factory* factory = 0;

      while ((factory = Factory::NextNewFactory()) != 0)
         AddFactory(factory);
   }

   DOUT3(("Manager thread is current process %s", DBOOL(usecurrentprocess)));

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
   CreateParStr(stParName, stHalted);

   ActivateTimeout(1.);

   fParsQueueBlocked = false;
}

dabc::Manager::~Manager()
{
   // stop thread that it is not try to access objects which we are directly deleting
   // normally, as last operation in the main() program must be HaltManeger(true)
   // call, which suspend and erase all items in manager

   DOUT3(("Start ~Manager"));

   fParsQueueBlocked = true;

   fSMmodule = 0;

   HaltManager();

   DOUT3(("~Manager -> CancelCommands() locked:%s cmds:%u",
         DBOOL((fCmdsMutex ? fCmdsMutex->IsLocked() : false)),
         _NumSubmCmds()));

   CancelCommands();

   DOUT3(("~Manager -> DeleteChilds()"));

   DestroyAllPars();

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

   if (fInstance==this) {
      DOUT0(("Real EXIT"));
      dabc::Logger::Instance()->LogFile(0);
      fInstance = 0;
   } else
      EOUT(("What ??? !!!"));
}

void dabc::Manager::init()
{
   // method should be called from inherited class constructor to reactivate
   // all parameters events, which are created before

   unsigned cnt = 0;

   {
      LockGuard lock(fMgrMutex);
      cnt = fParsQueue.Size();
   }

   bool canexecute = ((ProcessorThread()==0) ||  ProcessorThread()->IsItself());

   while (cnt-->0)
      if (canexecute) ProcessParameterEvent();
                else  FireEvent(evntManagerParam);
}

void dabc::Manager::destroy()
{
   DestroyPar(stParName);
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

bool dabc::Manager::FindInConfiguration(dabc::Folder* fold, const char* itemname)
{
   LockGuard lock(fMgrMutex);

   if (fCfg==0) return false;

   std::string res;

   return fCfg->FindItem(fold, res, itemname, 0);
}

void dabc::Manager::FireParamEvent(Parameter* par, int evid)
{
   if (par==0) return;

   bool canexecute = true;
   bool cansubmit = true;

   {
      LockGuard lock(fMgrMutex);

      if (ProcessorThread()) canexecute = ProcessorThread()->IsItself();

      switch (evid) {
         case parCreated:
            DOUT3(("Read parameter %s from cfg %p", par->GetFullName().c_str(), fCfg));
            if (fCfg) par->Read(*fCfg);
            break;

         case parModified:
            // check if event of that parameter in the queue
            if (!par->fRegistered) return;
            for (unsigned n=0;n<fParsQueue.Size();n++)
                if (fParsQueue.ItemPtr(n)->par == par) return;
            break;

         case parDestroy:
            // disable all previous events, while parameter no longer valid
            for (unsigned n=0;n<fParsQueue.Size();n++)
               if (fParsQueue.ItemPtr(n)->par == par)
                  fParsQueue.ItemPtr(n)->processed = true;
            break;
      }

      // mask out all events for parameters with high visibility value
      if (!par->IsVisible() && (evid != parDestroy)) return;

      fParsQueue.Push(ParamRec(par,evid));

      // submit event, but execute it later, when queue is not blocked
      if (fParsQueueBlocked) return;
   }

   if (canexecute)
      ProcessParameterEvent();
   else
   if (cansubmit)
      FireEvent(evntManagerParam);
}

void dabc::Manager::ProcessParameterEvent()
{
   ParamRec rec;

   bool visible = true;
   bool activate = false;
   double interval = 0.;

   {
      LockGuard lock(fMgrMutex);
      if (fParsQueue.Size()==0) return;
      rec = fParsQueue.Pop();
      visible = rec.par->IsVisible();

      if (!rec.processed)

         switch (rec.event) {
            case parCreated:
               rec.par->fRegistered = true;

               if (rec.par->NeedTimeout()) {
                  if (fTimedPars.size()==0) { activate = true; interval = 0.; }
                  fTimedPars.push_back(rec.par);
               }

               break;

            case parModified:
               if (!rec.par->fRegistered) return;
               break;

            case parDestroy:
               rec.par->fRegistered = false;
               fTimedPars.remove(rec.par);
               if (fTimedPars.size()==0) { activate = true; interval = -1.; }
               break;
         }
//   } // unlock

   // generate parameter event from the manager thread
   if (!rec.processed && visible) ParameterEvent(rec.par, rec.event);
} // unlock

   if (rec.event == parDestroy) delete rec.par;

//   if (activate) ActivateTimeout(interval);
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
      case evntManagerParam: {
         ProcessParameterEvent();
         break;
      }

      default:
         WorkingProcessor::ProcessEvent(evnt);
   }
}

dabc::Module* dabc::Manager::FindModule(const char* name)
{
   return dynamic_cast<dabc::Module*> (FindChild(name));
}

bool dabc::Manager::DeleteModule(const char* name)
{
   return Execute(new CmdDeleteModule(name));
}

bool dabc::Manager::IsModuleRunning(const char* name)
{
   LockGuard lock(fMgrMutex);

   dabc::Module* m = FindModule(name);
   return m ? m->IsRunning() : false;
}

bool dabc::Manager::IsAnyModuleRunning()
{
   LockGuard lock(fMgrMutex);

   Iterator iter(this);

   while (iter.next()) {
      Module* m = dynamic_cast<Module*> (iter.current());
      if (m && m->IsRunning()) return true;
   }

   return false;
}

dabc::Port* dabc::Manager::FindPort(const char* name)
{
   LockGuard lock(fMgrMutex);

   return dynamic_cast<dabc::Port*>(FindChild(name));
}

dabc::Device* dabc::Manager::FindDevice(const char* name)
{
   dabc::Device* dev = 0;

   Folder* folder = GetDevicesFolder();
   if (folder) dev = dynamic_cast<dabc::Device*> (folder->FindChild(name));

   if (dev==0) dev = dynamic_cast<dabc::Device*> (FindChild(name));

   return dev;
}

dabc::Device* dabc::Manager::FindLocalDevice(const char* name)
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
   return dynamic_cast<Application*>(FindChild(xmlAppDfltName));
}

dabc::MemoryPool* dabc::Manager::FindPool(const char* name)
{
   return dynamic_cast<dabc::MemoryPool*> (FindChild(name));
}

std::string dabc::Manager::BuildControlName(Basic* obj)
{
   std::string res;

   if (obj==0) return res;
   res = obj->GetName();

   Basic* prnt = obj->GetParent();
   while ((prnt!=0) && (prnt!=this)) {
      res += ".";
      res += prnt->GetName();
      prnt = prnt->GetParent();
   }

   return res;
}

dabc::Basic* dabc::Manager::FindControlObject(const char* name)
{
   if ((name==0) || (strlen(name)==0)) return 0;

   Folder* prnt = this;

   std::string fullname = name;

   size_t pos = 0;

   while ((pos = fullname.find_last_of('.')) != std::string::npos) {
      const char* prntname = fullname.c_str() + pos + 1;

      prnt = dynamic_cast<Folder*> (prnt->FindChild(prntname));
      if (prnt==0) return 0;

      fullname.erase(pos);
   }

   return prnt->FindChild(fullname.c_str());
}


bool dabc::Manager::DeletePool(const char* name)
{
   return Execute(new CmdDeletePool(name));
}

bool dabc::Manager::DoDeleteAllModules(int appid)
{
   Module* m = 0;

   do {
      if (m!=0) {
         DOUT2(("Stop and delete module %s", m->GetName()));
         m->Stop();
         delete m;
         DOUT2(("Stop and delete module done"));
         m = 0;
      }

      LockGuard lock(fMgrMutex);

      Iterator iter(this);
      while (iter.next()) {
         m = dynamic_cast<Module*> (iter.current());
         if (m!=0)
            if ((appid<0) || (m->GetAppId()==appid)) break;
         m = 0;
      }
   } while (m!=0);

   return true;
}

void dabc::Manager::DoHaltManager()
{
//   dabc::SetDebugLevel(5);

   DOUT3(("Start DoHaltManager"));

   DOUT3(("Deleting application"));
   delete GetApp();

   DOUT3(("Deleting all modules"));
   // than we delete all modules

   DoDeleteAllModules();

   DOUT3(("Do device cleanup"));

   DoCleanupDevices(true);

   // special case - verbs objects may in destroy queue and must be deleted before devices
   ProcessDestroyQueue();

   DOUT3(("Calling destructor of all devices"));
   Folder* df = GetDevicesFolder();
   if (df) df->DeleteChilds();

   DOUT3(("Calling destructor of all memory pools"));
   DeleteChilds(-1, clMemoryPool);

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
   return Execute(new CmdConnectPorts(port1name, port2name, devname));
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

      // this is local command submission
      CommandReceiver* rcv = 0;
      Basic* obj = 0;

      if ((itemname!=0) && (strlen(itemname)>0)) obj = FindChild(itemname);

      if (obj!=0) rcv = obj->GetCmdReceiver();

      cmd->RemovePar("_ItemName_");

      DOUT5(("Manager get command %s for item %s : %p", cmd->GetName(), (itemname ? itemname : "null"), rcv));

      if (rcv!=0) {
         rcv->Submit(cmd);
         cmd_res = cmd_postponed;
      } else
      if ((obj!=0) && cmd->IsName(CmdSetParameter::CmdName())) {
         dabc::Parameter* par = dynamic_cast<dabc::Parameter*> (obj);
         if (par && par->InvokeChange(cmd)) cmd_res = cmd_postponed;
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
   if (cmd->IsName(CmdCreateModule::CmdName())) {
      const char* classname = cmd->GetPar("Class");
      const char* modulename = cmd->GetPar("Name");
      const char* thrdname = cmd->GetStr("Thread");

      Module* m = FindModule(modulename);

      if (m!=0) {
         DOUT4(("Module name %s already exists", modulename));
      } else {
         Folder* folder = GetFactoriesFolder(false);
         if (folder!=0)
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
      }

      cmd_res = cmd_bool(m!=0);
   } else
   if (cmd->IsName(CmdCreateApplication::CmdName())) {
      const char* classname = cmd->GetStr("AppClass");
      const char* appthrd = cmd->GetStr("AppThrd");

      if (classname==0) classname = typeApplication;

      Application* app = GetApp();

      if ((app!=0) && (strcmp(app->ClassName(), classname)==0)) {
         DOUT2(("Application of class %s already exists", classname));
      } else
      if (strcmp(classname, typeApplication)!=0) {
         if (app!=0) { delete app; app = 0; }

         dabc::Folder* folder = GetFactoriesFolder(false);

         if (folder!=0)
            for (unsigned n=0;n<folder->NumChilds();n++) {
               Factory* factory =
                  dynamic_cast<Factory*> (folder->GetChild(n));
               if (factory!=0)
                  app = factory->CreateApplication(classname, cmd);
               if (app!=0) break;
            }
      } else {
         if (app!=0) { delete app; app = 0; }

         void* func = dabc::Factory::FindSymbol(fCfg->InitFuncName());

         app = new Application((Application::ExternalFunction*)func);
      }

      if ((appthrd==0) || (strlen(appthrd)==0)) appthrd = MgrThrdName();

      if (app!=0) MakeThreadFor(app, appthrd);

      cmd_res = cmd_bool(app!=0);

      if (app!=0) DOUT1(("Create application of class %s", classname));
             else EOUT(("Cannot create application of class %s", classname));

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
        else {
           EOUT(("Device %s has other class name %s than requested", devname, dev->ClassName(), classname));
           dev = 0;
        }
      } else {
         dabc::Folder* folder = GetFactoriesFolder(false);

         if (folder!=0)
            for (unsigned n=0;n<folder->NumChilds();n++) {
               Factory* factory =
                  dynamic_cast<Factory*> (folder->GetChild(n));
               if (factory!=0)
                  dev = factory->CreateDevice(classname, devname, cmd);
               if (dev!=0) break;
            }
      }

       cmd_res = cmd_bool(dev!=0);
   } else
   if (cmd->IsName(CmdCreateTransport::CmdName())) {
      const char* portname = cmd->GetStr("PortName");
      const char* transportkind = cmd->GetStr("TransportKind");
      const char* thrdname = cmd->GetStr("TransportThrdName");

      Port* port = FindPort(portname);
      Device* dev = FindDevice(transportkind);

      if (port==0) {
         EOUT(("Port %s not found - cannot create transport %s", portname, transportkind));
         cmd_res = cmd_false;
      } else
      if (dev!=0) {
         dev->Submit(cmd);
         cmd_res = cmd_postponed;
      } else
      if ((transportkind==0) || (strlen(transportkind)==0)) {
         port->AssignTransport(0);
         cmd_res = cmd_true;
      } else {
         Transport* tr = 0;

         Folder* folder = GetFactoriesFolder(false);

         if (folder!=0)
            for (unsigned n=0;n<folder->NumChilds();n++) {
               Factory* factory =
                  dynamic_cast<Factory*> (folder->GetChild(n));
               if (factory!=0)
                  tr = factory->CreateTransport(port, transportkind, thrdname, cmd);
               if (tr!=0) break;
            }

         if (tr==0) {
            EOUT(("Cannot create transport of kind %s", transportkind));
            cmd_res = cmd_false;
         } else {
            port->AssignTransport(tr);
            cmd_res = cmd_true;
         }
      }
   } else
   if (cmd->IsName(CmdCreateThread::CmdName())) {
      const char* thrdname = cmd->GetStr("ThrdName");
      const char* thrdclass = cmd->GetStr("ThrdClass");
      const char* thrddev = cmd->GetStr("ThrdDev");

      cmd_res = cmd_bool(CreateThread(thrdname, thrdclass, 0, thrddev, cmd)!=0);

   } else
   if (cmd->IsName(CmdCreateMemoryPool::CmdName())) {
      cmd_res = cmd_bool(DoCreateMemoryPool(cmd));
   } else
   if (cmd->IsName(CmdCleanupManager::CmdName())) {
      cmd_res = DoCleanupManager(cmd->GetInt("AppId", 0));
   } else
   if (cmd->IsName(CmdCheckConnModule::CmdName())) {
      dabc::Module* m = FindModule(cmd->GetStr("Module",""));
      if (m!=0) {
         m->Submit(cmd);
         cmd_res = cmd_postponed;
      } else
         cmd_res = cmd_false;
   } else
   if (cmd->IsName(CmdStartModule::CmdName())) {
      dabc::Module* m = FindModule(cmd->GetStr("Module",""));
      if (m!=0) m->Stop();
      cmd_res = cmd_bool(m!=0);
   } else
   if (cmd->IsName(CmdStopModule::CmdName())) {
      dabc::Module* m = FindModule(cmd->GetStr("Module",""));
      if (m!=0) m->Stop();
      cmd_res = cmd_bool(m!=0);
   } else
   if (cmd->IsName(CmdDeleteModule::CmdName())) {
      dabc::Module* m = FindModule(cmd->GetStr("Module",""));
      if (m!=0) {
         DOUT2(("Stop and delete module %s", m->GetName()));
         m->Stop();
         delete m;
         DOUT2(("Stop and delete module done"));
      }
//      DOUT1(("Call module stop %s", (m ? m->GetName() : "null")));
      cmd_res = cmd_bool(m!=0);
   } else
   if (cmd->IsName(CmdStartAllModules::CmdName())) {

      int appid = cmd->GetInt("AppId", 0);

      Iterator iter(this);

      while (iter.next()) {
         dabc::Module* m = dynamic_cast<dabc::Module*> (iter.current());
         if (m!=0)
            if ((appid<0) || (m->GetAppId() == appid))
               m->Start();
      }
   } else
   if (cmd->IsName(CmdStopAllModules::CmdName())) {

      int appid = cmd->GetInt("AppId", 0);

      Iterator iter(this);

      while (iter.next()) {
         dabc::Module* m = dynamic_cast<dabc::Module*> (iter.current());
         if (m!=0)
            if ((appid<0) || (m->GetAppId() == appid))
               m->Stop();
      }
   } else
   if (cmd->IsName(CmdDeletePool::CmdName())) {
      dabc::MemoryPool* pool = FindPool(cmd->GetPar("PoolName"));
      if (pool) delete pool;
   } else
   if (cmd->IsName(CmdStateTransition::CmdName())) {
      InvokeStateTransition(cmd->GetStr("Cmd"), cmd);
      cmd_res = cmd_postponed;
   } else
   if (cmd->IsName("Print")) {
      DoPrint();
   } else

   // these are two special commands with postponed execution
   if (cmd->IsName(CmdConnectPorts::CmdName())) {
      std::string manager1name, manager2name;

      const char* port1name = ExtractManagerName(cmd->GetPar("Port1Name"), manager1name);
      const char* port2name = ExtractManagerName(cmd->GetPar("Port2Name"), manager2name);

      DOUT3(("Doing ports connect %s::%s %s::%s",
              (manager1name.length()>0 ? manager1name.c_str() : "---"), port1name,
              (manager2name.length()>0 ? manager2name.c_str() : "---"), port2name));

      // first check local connect
      if ((manager1name.length()==0) && (manager2name.length()==0)) {

         Device* dev = FindLocalDevice(cmd->GetStr("Device"));
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

         std::string remrecvname;

         if (manager1name == manager2name) {
            newcmd = new CmdConnectPorts(port1name, port2name);

         } else {
            if (!CanSendCmdToManager(manager2name.c_str())) {
               EOUT(("Client node for connection not valid %s",manager2name.c_str()));
               return cmd_false;
            }

            remrecvname = DevicesFolderName();
            remrecvname += "/";
            remrecvname += cmd->GetStr("Device");

            newcmd = new CmdDirectConnect(true, port1name, true);
            // copy all additional values from
         }

         newcmd->AddValuesFrom(cmd, false);
         int parentid = AddInternalCmd(cmd, "_PCID_");
         newcmd->SetInt("#_PCID_", parentid);
         newcmd->ClearResult();

         SetCmdReceiver(newcmd, manager1name.c_str(), remrecvname.c_str());

         if (!Submit(Assign(newcmd)))
            EOUT(("Cannot submit remote command"));

         cmd_res = cmd_postponed;
      }
   } else
   if (cmd->IsName("Ping"))
      cmd_res = cmd_true;
   else
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
      // this is for replies
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

bool dabc::Manager::CreateMemoryPool(const char* poolname,
                                     unsigned buffersize,
                                     unsigned numbuffers,
                                     unsigned numincrement,
                                     unsigned headersize,
                                     unsigned numsegments)
{

   DOUT3(("Create memory pool %s %u x %u", poolname, buffersize, numbuffers));

   Command* cmd = new CmdCreateMemoryPool(poolname);

   if ((buffersize>0) && (numbuffers>0)) {
      if (!CmdCreateMemoryPool::AddMem(cmd, buffersize, numbuffers, numincrement)) {
         dabc::Command::Finalise(cmd);
         return false;
      }

      if (!CmdCreateMemoryPool::AddRef(cmd, numbuffers*2, headersize, numincrement, numsegments)) {
         dabc::Command::Finalise(cmd);
         return false;
      }

      if (numincrement==0)
         cmd->SetBool(xmlFixedLayout, true);
   }

   return Execute(cmd);
}

bool dabc::Manager::DoCreateMemoryPool(Command* cmd)
{
   if (cmd==0) return false;

   const char* poolname = cmd->GetPar(xmlPoolName);
   if (poolname==0) {
      EOUT(("Pool name is not specified"));
      return false;
   }

   MemoryPool* pool = FindPool(poolname);

   if (pool!=0) {
      DOUT0(("Pool %s already exists, do not try to create it again!!!", poolname));
      return true;
   }

   pool = new dabc::MemoryPool(this, poolname);

   pool->UseMutex();
   pool->SetMemoryLimit(0); // one can extend pool as much as system can
   pool->SetCleanupTimeout(1.0); // when changed, memory pool can be shrink again
   pool->AssignProcessorToThread(ProcessorThread());

   return pool->Reconstruct(cmd);
}

void dabc::Manager::ModuleExecption(Module* m, const char* msg)
{
   EOUT(("EXCEPTION Module: %s message %s", m->GetName(), msg));
}

bool dabc::Manager::PostCommandProcess(Command* cmd)
{
   // PostCommandProcess return true, when command can be safely deleted

   if (cmd->GetPar("_remotereply_srcid_")) {
      std::string mgrname = cmd->GetStr("_remotereply_srcid_");
      cmd->RemovePar("_remotereply_srcid_");

      std::string sbuf;
      cmd->SaveToString(sbuf);

      SendOverCommandChannel(mgrname.c_str(), sbuf.c_str());

      return true;
   } else
   if (cmd->IsName(CmdConnectPorts::CmdName())) {

      int parentid = cmd->GetInt("#_PCID_", -1);
      if (parentid<0) return true;

      Command* prnt = TakeInternalCmd("_PCID_", parentid);
      dabc::Command::Reply(prnt, cmd->GetResult());
   } else

   if (cmd->IsName(CmdDirectConnect::CmdName())) {

      int parentid = cmd->GetInt("#_PCID_", -1);
      if (parentid<0) return true;

      DOUT5(("!!!!!!!!!!!!  Reply of CmdDirectConnect parent = %d", parentid));

      bool res = cmd->GetResult();
      cmd->ClearResult();

      if (cmd->GetBool("ClientSide", false) || !res) {
         Command* prnt = TakeInternalCmd("_PCID_", parentid);
         dabc::Command::Reply(prnt, res);
         return true;
      }

      Command* prnt = FindInternalCmd("_PCID_", parentid);
      if (prnt==0) {
         EOUT(("Parent %d is disappear, parentid"));
         return true;
      }

      std::string manager2name;
      const char* port2name = ExtractManagerName(prnt->GetPar("Port2Name"), manager2name);

      Command* newcmd = new CmdDirectConnect(false, port2name, false);
      newcmd->AddValuesFrom(prnt, false); // copy initial settings
      newcmd->AddValuesFrom(cmd, false);  // copy new values from first command
      newcmd->SetInt("#_PCID_", parentid);
      newcmd->SetBool("ClientSide", true);
      newcmd->ClearResult();

      std::string devname = DevicesFolderName();
      devname += "/";
      devname += prnt->GetStr("Device");

      SetCmdReceiver(newcmd, manager2name.c_str(), devname.c_str());

      if (!Submit(Assign(newcmd))) {
         Command* prnt = TakeInternalCmd("_PCID_", parentid);
         dabc::Command::Reply(prnt, false);
      }
   }

   return true;
}

void dabc::Manager::StartModule(const char* modulename)
{
   Execute(new dabc::CmdStartModule(modulename));
}

void dabc::Manager::StopModule(const char* modulename)
{
   Execute(new dabc::CmdStopModule(modulename));
}

bool dabc::Manager::StartAllModules(int appid)
{
   return Execute(new CmdStartAllModules(appid));
}

bool dabc::Manager::StopAllModules(int appid)
{
   return Execute(new CmdStopAllModules(appid));
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
        DOUT2(("!!! Thread %s has nothing to do, delete", thrd->GetName()));

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

   DoDeleteAllModules(appid);

   DOUT3(( "Cleanup all devices"));
   DoCleanupDevices(false);

   // here we delete all pools

   DOUT3(("Deleting app pools"));
   DeleteChilds(appid, clMemoryPool);

   Folder* pf = GetDevicesFolder();
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
            thrd = factory->CreateThread(thrdclass, thrdname, thrddev, cmd);
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

std::string dabc::Manager::MakeThreadName(const char* base)
{
   if ((base==0) || (strlen(base)==0)) base = "Thread";

   std::string newname;

   int cnt = 0;
   do {
     dabc::formats(newname, "%s%d", base, cnt++);
   } while (FindThread(newname.c_str()));

   return newname;
}

bool dabc::Manager::MakeThreadFor(WorkingProcessor* proc, const char* thrdname, unsigned startmode)
{
   if (proc==0) return false;

   std::string newname;

   if ((thrdname==0) || (strlen(thrdname)==0)) {
      DOUT2(("Thread name not specified - generate default"));
      newname = MakeThreadName("Thread");
      thrdname = newname.c_str();
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

void dabc::Manager::DoPrint()
{
   dabc::Iterator iter(GetThreadsFolder(false));

   while (iter.next()) {
      WorkingThread* thrd = dynamic_cast<WorkingThread*> (iter.current());
      if (thrd) DOUT1(("Thrd: %s", thrd->GetName()));
   }

   dabc::Iterator iter2(this);

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

bool dabc::Manager::TestActiveNodes(double tmout)
{
   CommandClient cli;

   for (int node=0; node<NumNodes(); node++)
      if ((node!=NodeId()) && IsNodeActive(node)) {
         Command* cmd = new Command("Ping");
         SetCmdReceiver(cmd, GetNodeName(node), "");
         Submit(cli.Assign(cmd));
      }

   return cli.WaitCommands(tmout);
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

      if (cmd==0) cmd = new CmdStateTransition(state_transition_name);

      fSMmodule->Submit(cmd);
      DOUT3(("Submit state transition %s", state_transition_name));
      return true;
   }

   dabc::Command::Reply(cmd, false);

   if ((state_transition_name!=0) && (strcmp(state_transition_name, stcmdDoStop)==0)) {
      DOUT1(("Application can stop its execution"));

      if (!fMgrStopped && fMgrMainLoop) {
         fMgrStopped = true;
         WorkingThread* thrd = ProcessorThread();
         if (thrd && !fMgrNormalThrd) thrd->SetWorkingFlag(false);
      }
//      if (thrd && !fMgrNormalThrd) {
//         RemoveProcessorFromThread(true);
//         thrd->Stop(false); // stop thread - means leave thread main loop
//      }
   }

   return false;
}

bool dabc::Manager::ChangeState(const char* state_transition_cmd, double tmout)
{
   bool res = false;

   if (fSMmodule==0)
      res = DoStateTransition(state_transition_cmd);
   else {
      dabc::CommandClient cli;

      dabc::Command* cmd = new dabc::CmdStateTransition(state_transition_cmd);

      if (InvokeStateTransition(state_transition_cmd, cli.Assign(cmd)))
         res = cli.WaitCommands(tmout);
   }

   if (!res) {
      EOUT(("State change %s fail. EXIT!!!! ", state_transition_cmd));
      exit(1);
   } else
      DOUT1(("State change %s done", state_transition_cmd));

   return res;
}

std::string dabc::Manager::CurrentState() const
{
   return GetParStr(stParName, stNull);
}

bool dabc::Manager::IsStateTransitionAllowed(const char* stcmd, bool errout)
{
   if ((stcmd==0) || (strlen(stcmd)==0)) {
      if (errout) EOUT(("State transition command not specified"));
      return false;
   }

   const char* tgtstate = TargetStateName(stcmd);

   std::string currstate = CurrentState();

   if (currstate == tgtstate) {
      DOUT1(("SM Command %s leads to current state, do nothing", stcmd));
      return false;
   }

   bool allowed = false;
   if ((currstate == stHalted) && (strcmp(stcmd, stcmdDoConfigure)==0))
      allowed = true;
   else
   if ((currstate == stConfigured) && (strcmp(stcmd, stcmdDoEnable)==0))
      allowed = true;
   else
   if ((currstate == stReady) && (strcmp(stcmd, stcmdDoStart)==0))
       allowed = true;
   else
   if ((currstate == stRunning) && (strcmp(stcmd, stcmdDoStop)==0))
       allowed = true;
   else
   if (strcmp(stcmd, stcmdDoHalt)==0)
      allowed = true;

   if (!allowed && errout)
      EOUT(("Command %s not allowed at state %s", stcmd, currstate.c_str()));

   return allowed;
}

bool dabc::Manager::DoStateTransition(const char* stcmd)
{
   // must be called from SM thread

   Application* app = GetApp();

   if (app==0) return false;

/*
   if (strcmp(stcmd, stcmdDoHalt) == 0) {
     SetDebugLevel(2);
     SetFileLevel(4);
   } else {
      SetDebugLevel(0);
      SetFileLevel(1);
   }
*/

   DOUT4(("DoStateTransition %s", stcmd));

   const char* tgtstate = TargetStateName(stcmd);

   bool res = app->DoStateTransition(stcmd);

   if (!res) tgtstate = stFailure;

   if (!Execute(new CmdSetParameter(stParName, tgtstate))) res = false;

   DOUT4(("DoStateTransition %s res = %s", stcmd, DBOOL(res)));

   return res;
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

double dabc::Manager::ProcessTimeout(double last_diff)
{
   dabc::Logger::CheckTimeout();

   LockGuard lock(fMgrMutex);

//   Parameter* par = FindPar(stParName);
//   if (par!=0) par->Changed();

   for (unsigned n=0;n<fTimedPars.size();n++)
      if (fTimedPars[n]!=0)
         ((Parameter*) fTimedPars[n])->ProcessTimeout(last_diff);

   // return (fTimedPars.size()==0 && fMgrNormalThrd) ? -1. : 1.;

   return 1.;
}

void dabc::Manager::AddFactory(Factory* factory)
{
   if (factory==0) return;

   Folder* f = GetFactoriesFolder(true);
   f->AddChild(factory);

   DOUT3(("Add factory %s", factory->GetName()));
}

bool dabc::Manager::CreateApplication(const char* classname, const char* appthrd)
{
   return Execute(new CmdCreateApplication(classname, appthrd));
}

bool dabc::Manager::CreateDevice(const char* classname, const char* devname, const char* devthrd)
{
   return Execute(new CmdCreateDevice(classname, devname, devthrd));
}

bool dabc::Manager::CreateModule(const char* classname, const char* modulename, const char* thrdname)
{
   return Execute(new CmdCreateModule(classname, modulename, thrdname));
}

bool dabc::Manager::CreateTransport(const char* portname, const char* transportkind, const char* thrdname)
{
   return Execute(new CmdCreateTransport(portname, transportkind, thrdname));
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

void dabc_Manager_CtrlCHandler(int number)
{
   if (dabc::mgr())
      dabc::mgr()->ProcessCtrlCSignal();
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

   DOUT1(("Install Ctrl-C handler from thrd %d", Thread::Self()));

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
   //if ((ProcessorThread()==0) || !ProcessorThread()->IsItself()) return;

   if (fSigThrd != dabc::Thread::Self()) return;

   DOUT1(("Process CTRL-C signal"));

   if (fMgrMainLoop) {
      if (fMgrStopped) return;

      fMgrStopped = true;

      WorkingThread* thrd = ProcessorThread();

      if (thrd && !fMgrNormalThrd) thrd->SetWorkingFlag(false);

      return;
   }

   DOUT1(("Ctrl-C out of main loop - force manager halt"));

   DoHaltManager();

   DOUT1(("Cancel commands"));

   CancelCommands();

   DOUT1(("Delete all parameters"));

   DestroyAllPars();

   DOUT1(("Delete children"));

   DeleteChilds();

   dabc::Logger::Instance()->ShowStat();

   DOUT0(("Exit after Ctrl-C"));

   exit(0);
}

void dabc::Manager::RunManagerMainLoop(int runtime)
{
   DOUT2(("Enter dabc::Manager::RunManagerMainLoop"));

   WorkingThread* thrd = ProcessorThread();

   if (thrd==0) return;

   fMgrMainLoop = true;
   fMgrStopped = false;

   if (fMgrNormalThrd) {
      DOUT3(("Manager has normal thread - just wait until application modules are stopped"));
      while(!fMgrStopped) {
         dabc::LongSleep(1);
         if (runtime>0)
           if (--runtime==0) break;
      }
   } else {
      DOUT3(("Manager uses main process as thread - run mainloop ourself"));

      // to be sure that timeout processing is active
      // only via timeout one should be able to stop processing of main loop
//      bool activate = false;
//      {
//         LockGuard lock(fMgrMutex);
//         activate = (fTimedPars.size()==0);
//      }
//      if (activate) ActivateTimeout(1.);

      ActivateTimeout(1.);

      thrd->MainLoop();

      // set true to be able process some other commands
      thrd->SetWorkingFlag(true);
   }

   fMgrMainLoop = false;
   fMgrStopped = true;

   DOUT2(("Exit dabc::Manager::RunManagerMainLoop"));
}


bool dabc::Manager::Store(ConfigIO &cfg)
{
   cfg.CreateItem(xmlContext);

   if (!fCfgHost.empty())
      cfg.CreateAttr(xmlHostAttr, fCfgHost.c_str());

   if (fCfgHost != GetName())
      cfg.CreateAttr(xmlNameAttr, GetName());

   StoreChilds(cfg);

   cfg.PopItem();

   return true;
}

bool dabc::Manager::Find(ConfigIO &cfg)
{
   if (!cfg.FindItem(xmlContext)) return false;

   if (!fCfgHost.empty())
      if (!cfg.CheckAttr(xmlHostAttr, fCfgHost.c_str())) return false;

   if (fCfgHost != GetName())
      if (!cfg.CheckAttr(xmlNameAttr, GetName())) return false;

   return true;
}

// __________________________________________________________________


dabc::Command* dabc::SetCmdReceiver(Command* cmd, const char* itemname)
{
   // we set "_ItemName_" parameter to be able identify receiver for the command

   if (cmd)
      cmd->SetStr("_ItemName_", itemname ? itemname : "");

   return cmd;
}

dabc::Command* dabc::SetCmdReceiver(Command* cmd, const char* nodename, const char* itemname)
{
   if ((nodename==0) || (strlen(nodename)==0) || dabc::mgr()->IsName(nodename))
      return SetCmdReceiver(cmd, itemname);

   std::string fullname = nodename;
   fullname.append("$");
   if (itemname!=0) fullname.append(itemname);

   return SetCmdReceiver(cmd, fullname.c_str());
}

dabc::Command* dabc::SetCmdReceiver(Command* cmd, int nodeid, const char* itemname)
{
   return SetCmdReceiver(cmd, dabc::mgr()->GetNodeName(nodeid), itemname);
}

dabc::Command* dabc::SetCmdReceiver(Command* cmd, Basic* rcv)
{
   if (cmd==0) return 0;

   if (rcv==0) return SetCmdReceiver(cmd, "");

   std::string s = rcv->GetFullName(dabc::mgr());

   if (rcv->GetCmdReceiver()==0)
      EOUT(("Object %s cannot be used to receive commands", s.c_str()));

   return SetCmdReceiver(cmd, s.c_str());
}
