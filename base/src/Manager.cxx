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

#include "dabc/Manager.h"

#include <list>
#include <vector>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>

#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/threads.h"

#include "dabc/Port.h"
#include "dabc/Module.h"
#include "dabc/Command.h"
#include "dabc/CommandsSet.h"
#include "dabc/MemoryPool.h"
#include "dabc/Device.h"
#include "dabc/Url.h"
#include "dabc/PoolHandle.h"
#include "dabc/LocalTransport.h"
#include "dabc/Thread.h"
#include "dabc/Factory.h"
#include "dabc/Application.h"
#include "dabc/Iterator.h"
#include "dabc/FileIO.h"
#include "dabc/BinaryFile.h"
#include "dabc/DataIOTransport.h"
#include "dabc/Configuration.h"
#include "dabc/ConnectionManager.h"
#include "dabc/ReferencesVector.h"
#include "dabc/CpuInfoModule.h"

namespace dabc {

   ManagerRef mgr;

   class StdManagerFactory : public Factory {
      public:
         StdManagerFactory(const char* name) : Factory(name) { }

         virtual Module* CreateModule(const char* classname, const char* modulename, Command cmd);

         virtual Reference CreateThread(Reference parent, const char* classname, const char* thrdname, const char* thrddev, Command cmd);

         virtual FileIO* CreateFileIO(const char* typ, const char* name, int option);

         virtual Object* ListMatchFiles(const char* typ, const char* filemask);
   };



   /** DependPair keeps dependency between two objects.
    * When \field tgt object want to be deleted,
    * ObjectDestroyed() method will be called in \param src object to perform correct cleanup
    * It is supposed that \param src has reference on \param tgt and this reference should be
    * released, otherwise \param tgt object will be not possible to destroy
    *
    */
   struct DependPair {
      Reference src; //!< reference on object which want to be informed when \param tgt object is deleted
      Object* tgt;   //!< object
      int fire;      //!< how to proceed pair 0 - remain, 1 - inform src, 2 - just delete

      DependPair() : src(), tgt(0), fire(0) {}
      DependPair(Object* _src, Object* _tgt) : src(_src), tgt(_tgt), fire(0) {}
      DependPair(const DependPair& d) : src(d.src), tgt(d.tgt), fire(d.fire) {}
   };

   class DependPairList : public std::list<DependPair> {};


   struct ParamEventReceiverRec {
      WorkerRef    recv;          //!< only workers can be receiver of the parameters events
      std::string  remote_recv;   //!< address of remote receiver of parameter events
      bool         only_change;   //!< specify if only parameter-change events are produced
      std::string  name_mask;     //!< mask only for parameter names, useful when only specific names are interested
      std::string  fullname_mask; //!< mask for parameter full names, necessary when full parameter name is important
      int          queue;         //!< number of parameters events submitted, if bigger than some limit events will be skipped

      ParamEventReceiverRec() :
         recv(),
         remote_recv(),
         only_change(false),
         name_mask(),
         fullname_mask(),
         queue(0)
      {}

      bool match(const std::string& parname, int event, const std::string& fullname)
      {
         if (only_change && (event!=parModified)) return false;

         if (name_mask.length()>0)
            return Object::NameMatch(parname, name_mask);
         if (fullname_mask.length() > 0)
            return Object::NameMatch(fullname, fullname_mask);
         return true;
      }

   };

   class ParamEventReceiverList : public std::list<ParamEventReceiverRec> {};

}

dabc::Module* dabc::StdManagerFactory::CreateModule(const char* classname, const char* modulename, Command cmd)
{
   if (strcmp(classname, "dabc::CpuInfoModule")==0)
      return new CpuInfoModule(modulename, cmd);
   else
   if (strcmp(classname, "dabc::ConnectionManager")==0)
      return new ConnectionManager(modulename, cmd);

   return 0;
}

dabc::Reference dabc::StdManagerFactory::CreateThread(Reference parent, const char* classname, const char* thrdname, const char* thrddev, Command cmd)
{
   dabc::Thread* thrd = 0;

   if ((classname==0) || (strlen(classname)==0) || (strcmp(classname, typeThread)==0))
      thrd = new Thread(parent, thrdname, cmd.GetInt("NumQueues", 3));

   return Reference(thrd);
}

dabc::FileIO* dabc::StdManagerFactory::CreateFileIO(const char* typ, const char* name, int option)
{
   if (!((typ==0) || (strlen(typ)==0) ||
       (strcmp(typ,"std")==0) || (strcmp(typ,"posix")==0))) return 0;


   dabc::FileIO* io = new dabc::PosixFileIO(name, option);

   if (!io->IsOk()) { delete io; io = 0; }

   return io;
}

dabc::Object* dabc::StdManagerFactory::ListMatchFiles(const char* typ, const char* filemask)
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

   Object* res = 0;
   struct stat buf;

   for (int n=0;n<len;n++) {
      const char* item = namelist[n]->d_name;

      if ((fname==0) || (fnmatch(fname, item, FNM_NOESCAPE)==0)) {
         std::string fullitemname;
         if (slash) fullitemname += pathname;
         fullitemname += item;
         if (stat(fullitemname.c_str(), &buf)==0)
            if (!S_ISDIR(buf.st_mode) && (access(fullitemname.c_str(), R_OK)==0)) {
               if (res==0) res = new Object(0, "FilesList", true);
               new Object(res, fullitemname.c_str());
               DOUT4((" File: %s", fullitemname.c_str()));
            }
      }

      free(namelist[n]);
   }

   free(namelist);

   return res;
}

dabc::FactoryPlugin stdfactory(new dabc::StdManagerFactory("std"));


/** Helper class to destroy manager when finishing process */
namespace dabc {
   class AutoDestroyClass {
         friend class dabc::Manager;

         static bool fAutoDestroy;

      public:
         AutoDestroyClass() {}
         ~AutoDestroyClass()
         {
            //printf("Vary last action mgr = %p\n", dabc::mgr());
            if (dabc::mgr() && fAutoDestroy) {
               //printf("Destroy manager\n");
               dabc::mgr()->HaltManager();
               dabc::mgr.Destroy();
               //printf("Destroy manager done\n");
            }
         }
   };
}

bool dabc::AutoDestroyClass::fAutoDestroy = false;

dabc::AutoDestroyClass auto_destroy_instance;

// ******************************************************************

void dabc::Manager::SetAutoDestroy(bool on)
{
   AutoDestroyClass::fAutoDestroy = on;
}


dabc::Manager::Manager(const char* managername, Configuration* cfg) :
   Worker(0, managername, true),
   fMgrStoppedTime(),
   fMgrMutex(0),
   fDestroyQueue(0),
   fParsQueue(1024),
   fTimedPars(0),
   fParEventsReceivers(0),
   fDepend(0),
   fCfg(cfg),
   fCfgHost(),
   fNodeId(0),
   fNumNodes(1)
{
   if (dabc::mgr.null()) {
      dabc::mgr = dabc::ManagerRef(this);
      dabc::mgr.SetTransient(false);
      dabc::SetDebugPrefix(managername);
   }

   if (cfg) {
      fCfgHost = cfg->MgrHost();
      fNodeId = cfg->MgrNodeId();
      fNumNodes = cfg->MgrNumNodes();
   }

   // we create recursive mutex to avoid problem in Folder::GetFolder method,
   // where constructor is called under locked mutex,
   // let say - there is no a big problem, when mutex locked several times from one thread

   fMgrMutex = new Mutex(true);

   fDepend = new DependPairList;

   fLastCreatedDevName.clear();

   fParEventsReceivers = new ParamEventReceiverList;

   if (dabc::mgr() == this) {
//      static StdManagerFactory stdfactory("std");

      Factory* factory = 0;

      while ((factory = FactoryPlugin::NextNewFactory()) != 0)
         AddFactory(factory);
   }

   MakeThreadFor(this, MgrThrdName());

   ActivateTimeout(1.);
}

dabc::Manager::~Manager()
{
   // stop thread that it is not try to access objects which we are directly deleting
   // normally, as last operation in the main() program must be HaltManeger(true)
   // call, which suspend and erase all items in manager

//   dabc::SetDebugLevel(3);
//   dabc::SetDebugLevel(3);

   DOUT3(("~Manager -> DestroyQueue"));

   if (fDestroyQueue && (fDestroyQueue->GetSize()>0))
      EOUT(("Manager destroy queue is not empty"));

   delete fParEventsReceivers; fParEventsReceivers = 0;

   delete fDestroyQueue; fDestroyQueue = 0;

   if (fTimedPars!=0) {
      if (fTimedPars->GetSize() > 0) {
         EOUT(("Manager timed parameters list not empty"));
      }

      delete fTimedPars;
      fTimedPars = 0;
   }

   DOUT3(("~Manager -> ~fDepend"));

   if (fDepend && (fDepend->size()>0))
      EOUT(("Dependencies parameters list not empty"));

   delete fDepend; fDepend = 0;

   DOUT3(("~Manager -> ~fMgrMutex"));

   delete fMgrMutex; fMgrMutex = 0;

   if (dabc::mgr()==this) {
      DOUT1(("Real EXIT"));
      if (dabc::Logger::Instance())
         dabc::Logger::Instance()->LogFile(0);
   } else
      EOUT(("What ??? !!!"));
}


void dabc::Manager::HaltManager()
{
   ThreadRef thrd = thread();
   // only for case, when treads does not run its own main loop, we should help him to finish processing
   if (thrd.IsRealThrd()) thrd.Release();

   // command channel is special case - we let him inform others that we are going done
   GetCommandChannel().Execute(CmdShutdownControl(), 1);

   FindModule(ConnMgrName()).Destroy();

   DettachFromThread();

   DeleteChilds();

   // run dummy event loop several seconds to complete event which may be submitted there

   // FIXME: one should avoid any kind of timeouts
   int maxcnt = 200; // about 0.2 seconds

   int cnt = maxcnt;

   TimeStamp tm1 = dabc::Now();

   do {

      ProcessParameterEvents();

      ProcessDestroyQueue();

      if (!thrd.null()) {
         thrd()->SingleLoop(0, 0.001);
         if ((cnt < maxcnt/2) || (thrd()->TotalNumberOfEvents()==0)) thrd.Release();
      } else {
         if (cnt>0) dabc::Sleep(0.001);
      }

   } while ((--cnt > 0) && (dabc::Thread::NumThreadInstances() > 0));

   TimeStamp tm2 = dabc::Now();

   if (dabc::Thread::NumThreadInstances() > 0)
      EOUT(("!!!!!!!!! There are still %u threads - anyway declare manager halted cnt = %d  %5.3f !!!!!!", dabc::Thread::NumThreadInstances(), cnt, tm2 - tm1));
   else
      DOUT1((" ALL THREADS STOP AFTER %d tries tm %5.3f", maxcnt-cnt, tm2-tm1));

   dabc::Object::InspectGarbageCollector();

   DOUT3(("dabc::Manager::HaltManager done refcnt = %u", fObjectRefCnt));
}

bool dabc::Manager::ProcessDestroyQueue()
{
   ReferencesVector* vect = 0;

   // this is references, which want to be informed that object destroyed
   ReferencesVector clr_vect;
   PointersVector ptr_vect;

   // this is references which need to be released
   ReferencesVector rel_vect;

   // stole destroy queue - to avoid any kind of copy
   {
      LockGuard lock(fMgrMutex);

      vect = fDestroyQueue;
      fDestroyQueue = 0;

      DependPairList::iterator iter = fDepend->begin();
      while (iter != fDepend->end()) {
         if (iter->fire != 0) {
            if (iter->fire & 2) {
               clr_vect.Add(iter->src);
               ptr_vect.push_back(iter->tgt);
            } else
            if (iter->fire & 1) {
               rel_vect.Add(iter->src);
            }

            fDepend->erase(iter++);
         } else
            iter++;
      }
   }

   // first inform about dependencies
   for (unsigned n=0;n<clr_vect.GetSize();n++) {
      Object* obj = clr_vect.GetObject(n);
      Worker* w = dynamic_cast<Worker*> (obj);
      if (w!=0) {
         if (obj->IsLogging())
            DOUT0(("Submit worker %p to thread", obj));
         dabc::Command cmd("ObjectDestroyed");
         cmd.SetPtr("#Object", ptr_vect[n]);
         cmd.SetPriority(priorityMagic);

         if (w->Submit(cmd)) continue;
      }

      obj->ObjectDestroyed((Object*)ptr_vect[n]);
   }

   // clear all references
   clr_vect.Clear();
   ptr_vect.clear();
   rel_vect.Clear();

   if (vect==0) return false;

   ParamEventReceiverList::iterator iter = fParEventsReceivers->begin();

   while (iter!=fParEventsReceivers->end()) {
      if (vect->HasObject(iter->recv()))
         fParEventsReceivers->erase(iter++);
      else
         iter++;
   }

   DOUT3(("MGR: Process destroy QUEUE vect = %u", (vect ? vect->GetSize() : 0)));

   // FIXME: remove timed parameters, otherwise it is not possible to delete it
   // if (fTimedPars!=0)
   //      for(unsigned n=0;n<vect->GetSize();n++)
   //         fTimedPars->Remove(vect->GetObject(n));

   vect->DestroyAll();
   delete vect;

   return true;
}

void dabc::Manager::ProduceParameterEvent(ParameterContainer* par, int evid)
{
   // method called from parameter itself therefore we do not need mutex to access parameter field

   if (par==0) return;

   // first analyze parameters without manager mutex - we do not need it now

   if ((evid==parModified) && !par->IsDeliverAllEvents()) {
      // check if event of that parameter in the queue
      LockGuard lock(ObjectMutex());
      for (unsigned n=0;n<fParsQueue.Size();n++)
         if ((fParsQueue.Item(n).par == par) && (fParsQueue.Item(n).event==evid)) return;
   }

   bool fire = false;

   Parameter parref(par);

   {
      // now add record to the queue

      LockGuard lock(ObjectMutex());

      fire = fParsQueue.Size() == 0;

      // add parameter event to the queue
      ParamRec* rec = fParsQueue.PushEmpty();

      if (rec!=0) {
         rec->par << parref; // we are trying to avoid parameter locking under locked queue mutex
         rec->event = evid;
      }

   }

//   DOUT0(("FireParamEvent id %d par %s", evid, par->GetName()));

   if (fire) FireEvent(evntManagerParam);
}

bool dabc::Manager::ProcessParameterEvents()
{
   // do not process more than 100 events a time
   int maxcnt = 100;

   while (maxcnt-->0) {

      ParamRec rec;

      {
         LockGuard lock(fMgrMutex);
         if (fParsQueue.Size()==0) return false;
         rec = fParsQueue.Front();
         fParsQueue.PopOnly();
      }

      if (rec.par.null()) continue;

      bool checkadd(false), checkremove(false);

      switch (rec.event) {
         case parCreated:
            checkadd = rec.par.NeedTimeout();
            break;

         case parConfigured:
            checkadd = rec.par.NeedTimeout();
            checkremove = !checkadd;
            DOUT2(("Parameter %s configured checkadd %s", rec.par.GetName(), DBOOL(checkadd)));
            break;

         case parModified:
            break;

         case parDestroy:
            checkremove = rec.par.NeedTimeout();
            break;
      }

      if (checkadd) {
         rec.par()->SetCleanupBit();
         if (fTimedPars==0)
            fTimedPars = new ReferencesVector;
         if (!fTimedPars->HasObject(rec.par()))
            fTimedPars->Add(rec.par.Ref());
      }

      if (checkremove && fTimedPars) {
         fTimedPars->Remove(rec.par());
         if (fTimedPars->GetSize()==0) {
            delete fTimedPars;
            fTimedPars = 0;
         }
      }

      // from here we provide parameter to the external world

      bool attrmodified = rec.par.TakeAttrModified();

      if (rec.event == parConfigured) {
         attrmodified = true;
         rec.event = parModified;
      }

      std::string fullname, value;

      FillItemName(rec.par(), fullname);

      for (ParamEventReceiverList::iterator iter = fParEventsReceivers->begin();
             iter != fParEventsReceivers->end(); iter++) {

         if (!iter->match(rec.par.GetName(), rec.event, fullname)) continue;

         // TODO: provide complete list of fields - mean complete xml record ?????
         if (value.length()==0)
            value = rec.par.AsStdStr();

         if (iter->queue > 1000) {
            EOUT(("Too many events for receiver %s - block any following", iter->recv.GetName()));
            continue;
         }

         if (!iter->recv.null() && !iter->recv.CanSubmitCommand()) {
            DOUT4(("receiver %s cannot be used to submit command - ignore", iter->recv.GetName()));
            continue;
         }

         // TODO: probably one could use special objects and not command to deliver parameter events to receivers
         CmdParameterEvent evnt(fullname, value, rec.event, attrmodified);

         evnt.SetPtr("#Iterator", &(*iter));

         iter->queue++;

         Assign(evnt);

         if (iter->remote_recv.length() > 0) {
            evnt.SetReceiver(iter->remote_recv);
            GetCommandChannel(false).Submit(evnt);
         } else
            iter->recv.Submit(evnt);
      }

   }

   // generate one more event - we do not process all of records
   FireEvent(evntManagerParam);

   // generate parameter event from the manager thread
   return true;
}

void dabc::Manager::ProcessEvent(const EventId& evnt)
{
   DOUT5(("MGR::ProcessEvent %s", evnt.asstring().c_str()));

   switch (evnt.GetCode()) {
      case evntDestroyObj:
         ProcessDestroyQueue();
         break;
      case evntManagerParam:
         ProcessParameterEvents();
         break;
      default:
         Worker::ProcessEvent(evnt);
         break;
   }
   DOUT5(("MGR::ProcessEvent %s done", evnt.asstring().c_str()));
}

bool dabc::Manager::IsAnyModuleRunning()
{
   Module* m = 0;
   Iterator iter(this);

   while (iter.next_cast(m, m==0)) {
      if (m->IsRunning()) return true;
   }

   return false;
}

dabc::Reference dabc::Manager::FindItem(const std::string& name, bool islocal)
{
   Reference ref;
   std::string itemname;
   int nodeid(-1);

   if (islocal) {
      itemname = name;
   } else {
      if (!Url::DecomposeItemName(name, nodeid, itemname)) return ref;
      if ((nodeid>=0) && (nodeid!=NodeId())) return ref;
   }

   if (itemname.empty()) return ref;

   if (itemname[0] != '/') ref = GetAppFolder().FindChild(itemname.c_str());

   if (ref.null()) ref = FindChildRef(itemname.c_str());

   return ref;
}

dabc::ModuleRef dabc::Manager::FindModule(const char* name)
{
   return FindItem(name);
}

dabc::Reference dabc::Manager::FindPort(const char* name)
{
   PortRef ref = FindItem(name);

   return ref;
}

dabc::Reference dabc::Manager::FindPool(const char* name)
{
   MemoryPoolRef ref = FindItem(name);

   return ref;
}

dabc::WorkerRef dabc::Manager::FindDevice(const char* name)
{
   DeviceRef ref = FindItem(name);

   return ref;
}

dabc::Factory* dabc::Manager::FindFactory(const char* name)
{
   return dynamic_cast<dabc::Factory*> (GetFactoriesFolder().FindChild(name)());
}

dabc::ApplicationRef dabc::Manager::GetApp()
{
   return FindChildRef(xmlAppDfltName);
}

dabc::Reference dabc::Manager::GetAppFolder(bool force)
{
   return GetFolder(xmlAppDfltName, force, true);
}

void dabc::Manager::FillItemName(const Object* ptr, std::string& itemname, bool compact)
{
   itemname.clear();

   if (ptr==0) return;

   dabc::Reference app;

   if (compact) app = GetAppFolder();

   if (!app.null() && ptr->IsParent(app()))
      ptr->FillFullName(itemname, app());
   else {
      itemname = "/";
      ptr->FillFullName(itemname, this);
   }
}

bool dabc::Manager::DeletePool(const char* name)
{
   return Execute(CmdDeletePool(name));
}

bool dabc::Manager::ConnectPorts(const char* port1name,
                                 const char* port2name,
                                 const char* devname)
{
   return Execute(CmdConnectPorts(port1name, port2name, devname));
}

dabc::WorkerRef dabc::Manager::GetCommandChannel(bool force)
{
   WorkerRef ref = FindItem(CmdChlName());

   if (!ref.null() || !force) return ref;

   ref = DoCreateObject("SocketCommandChannel", CmdChlName());

   if (!ref.null()) MakeThreadFor(ref(), "CmdThrd");

   return ref;
}


int dabc::Manager::PreviewCommand(Command cmd)
{
   // check if command dedicated for other node, module and so on
   // returns: cmd_ignore - command will be processed in manager itself
   //          cmd_postponed - command redirected to other instance
   //          ..            - command is executed

   std::string url = cmd.GetReceiver();
   int tgtnode(-1);
   std::string itemname;

   if (!url.empty() && Url::DecomposeItemName(url, tgtnode, itemname)) {

      DOUT5(("MGR: Preview command %s item %s tgtnode %d", cmd.GetName(), url.c_str(), tgtnode));

      if ((tgtnode>=0) && (tgtnode != NodeId())) {

         if (!GetCommandChannel(true).Submit(cmd))
            EOUT(("Cannot submit command to remote node %d", tgtnode));

         return cmd_postponed;
      } else
      if (!itemname.empty()) {

         WorkerRef worker = FindItem(itemname, true);

         if (!worker.null()) {
            cmd.RemoveReceiver();
            worker.Submit(cmd);
            return cmd_postponed;
         }

         if (cmd.IsName(CmdSetParameter::CmdName())) {
            Parameter par = FindItem(itemname, true);

            if (!par.null()) {
               cmd.RemoveReceiver();
               return par.ExecuteChange(cmd);
            }
         }

         EOUT(("Did not found receiver %s for command %s", itemname.c_str(), cmd.GetName()));

         return cmd_false;
      }

   }

   return Worker::PreviewCommand(cmd);
}

#define FOR_EACH_FACTORY(arg) \
{ \
   ReferencesVector factories; \
   GetFactoriesFolder().GetAllChildRef(&factories); \
   Factory* factory = 0; \
   for (unsigned n=0; n<factories.GetSize(); n++) { \
      factory = dynamic_cast<Factory*> (factories.GetObject(n)); \
      if (factory==0) continue; \
      arg \
   } \
}

dabc::WorkerRef dabc::Manager::DoCreateModule(const char* classname, const char* modulename, const char* thrdname, Command cmd)
{
   ModuleRef ref = FindModule(modulename);

   if (!ref.null()!=0) {
      DOUT4(("Module name %s already exists", modulename));

   } else {

      FOR_EACH_FACTORY(
         ref = factory->CreateModule(classname, modulename, cmd);
         if (!ref.null()) break;
      )

      if (ref.null())
         EOUT(("Cannot create module of type %s", classname));
      else
         MakeThreadFor(ref(), thrdname);
   }

   return ref;
}


dabc::Reference dabc::Manager::DoCreateObject(const char* classname, const char* objname, Command cmd)
{

   dabc::Reference ref;

   FOR_EACH_FACTORY(
     ref = factory->CreateObject(classname, objname, cmd);
     if (!ref.null()) break;
   )

   return ref;
}


int dabc::Manager::ExecuteCommand(Command cmd)
{
   DOUT5(("MGR: Execute %s", cmd.GetName()));

   int cmd_res = cmd_true;

   if (cmd.IsName(CmdCreateModule::CmdName())) {
      const char* classname = cmd.GetField("Class");
      const char* modulename = cmd.GetField(CmdCreateModule::ModuleArg());
      const char* thrdname = cmd.GetStr("Thread");

      ModuleRef ref = DoCreateModule(classname, modulename, thrdname, cmd);
      cmd_res = cmd_bool(!ref.null());

   } else
   if (cmd.IsName("InitFactories")) {
      FOR_EACH_FACTORY(
         factory->Initialize();
      )
      cmd_res = cmd_true;
   } else
   if (cmd.IsName(CmdCreateApplication::CmdName())) {
      const char* classname = cmd.GetStr("AppClass");
      const char* appthrd = cmd.GetStr("AppThrd");

      // FIXME: classname can be specified as "*" in config file, should not be
      if ((classname==0) || (strcmp(classname,"*")==0)) classname = typeApplication;

      ApplicationRef app = GetApp();

      if (!app.null() && (strcmp(app()->ClassName(), classname)==0)) {
         DOUT2(("Application of class %s already exists", classname));
      } else
      if (strcmp(classname, typeApplication)!=0) {
         app.Destroy();

         FOR_EACH_FACTORY(
            app = factory->CreateApplication(classname, cmd);
            if (!app.null()) break;
         )
      } else {
         app.Destroy();

         void* func = dabc::Factory::FindSymbol(fCfg->InitFuncName());

         app = new Application();

         if (func) app()->SetInitFunc((Application::ExternalFunction*)func);
      }

      if ((appthrd==0) || (strlen(appthrd)==0)) appthrd = AppThrdName();

      if (!app.null()) MakeThreadFor(app(), appthrd);

      cmd_res = cmd_bool(!app.null());

      if (app!=0) DOUT2(("Application of class %s created", classname));
             else EOUT(("Cannot create application of class %s", classname));

   } else

   if (cmd.IsName(CmdCreateDevice::CmdName())) {
      const char* classname = cmd.GetStr("DevClass");
      const char* devname = cmd.GetStr("DevName");
      if (classname==0) classname = "";
      if ((devname==0) || (strlen(devname)==0)) devname = classname;

      WorkerRef dev = FindDevice(devname);

      cmd_res = cmd_false;

      if (!dev.null()) {
        if (strcmp(dev.ClassName(), classname)==0) {
           DOUT4(("Device %s of class %s already exists", devname, classname));
           cmd_res = cmd_true;
        } else {
           EOUT(("Device %s has other class name %s than requested", devname, dev.ClassName(), classname));
        }
      } else {
         FOR_EACH_FACTORY(
            if (factory->CreateDevice(classname, devname, cmd)) {
               cmd_res = cmd_true;
               break;
            }
         )

         if ((cmd_res==cmd_true) && FindDevice(devname).null())
            EOUT(("Cannot find device %s after it is created", devname));
      }

      if ((cmd_res==cmd_true) && (devname!=0)) {
         LockGuard guard(fMgrMutex);
         fLastCreatedDevName = devname;
      }

   } else
   if (cmd.IsName(CmdDestroyDevice::CmdName())) {
      const char* devname = cmd.GetStr("DevName");

      Reference dev = FindDevice(devname);

      cmd_res = cmd_bool(!dev.null());

      dev.Destroy();
   } else
   if (cmd.IsName(CmdCreateTransport::CmdName())) {
      const char* portname = cmd.GetStr("PortName");
      const char* transportkind = cmd.GetStr("TransportKind");
      const char* thrdname = cmd.GetStr(xmlTrThread);

      PortRef portref = FindPort(portname);
      Port* port = portref();
      WorkerRef dev = FindDevice(transportkind);

      if (portref.null()) {
         EOUT(("Port %s not found - cannot create transport %s", portname, transportkind));
         cmd_res = cmd_false;
      } else
      if (!dev.null()) {
         dev.Submit(cmd);
         cmd_res = cmd_postponed;
      } else
      if ((transportkind==0) || (strlen(transportkind)==0)) {
         port->AssignTransport(Reference(), 0, true);
         cmd_res = cmd_true;
      } else {

         cmd_res = cmd_false;
         portref.SetTransient(false);

         Transport* tr = 0;
         FOR_EACH_FACTORY(
            tr = factory->CreateTransport(portref, transportkind, cmd);
            if (tr!=0) break;
         )

         if (tr!=0) {

            // with explicit handle we lock transport before it starts to be active
            Reference handle(tr->GetWorker());

            RegisterDependency(handle(), port, true);

            std::string name(thrdname ? thrdname : "");
            if (name.length()==0) name = port->ThreadName();

            if (!MakeThreadFor(tr->GetWorker(), name.c_str())) {
               EOUT(("Fail to create thread for transport"));
               handle.Destroy();
            } else
            if (port->AssignTransport(handle, tr))
               cmd_res = cmd_true;
         }
      }
   } else
   if (cmd.IsName(CmdDestroyTransport::CmdName())) {
      PortRef portref = FindPort(cmd.GetStr("PortName"));
      if (!portref.Disconnect())
        cmd_res = cmd_false;
   } else
   if (cmd.IsName(CmdCreateThread::CmdName())) {
      const char* thrdname = cmd.GetStr("ThrdName");
      const char* thrdclass = cmd.GetStr("ThrdClass");
      const char* thrddev = cmd.GetStr("ThrdDev");

      ThreadRef thrd = CreateThread(thrdname, thrdclass, thrddev, cmd);

      cmd_res = cmd_bool(!thrd.null());

   } else
   if (cmd.IsName(CmdCreateMemoryPool::CmdName())) {
      cmd_res = cmd_bool(DoCreateMemoryPool(cmd));
   } else
   if (cmd.IsName(CmdCreateObject::CmdName())) {
      cmd.SetRef("Object", DoCreateObject(cmd.GetStr("ClassName"), cmd.GetStr("ObjName"), cmd));
      cmd_res = cmd_true;
   } else
   if (cmd.IsName(CmdCleanupApplication::CmdName())) {
      cmd_res = DoCleanupApplication();
   } else
   if (cmd.IsName(CmdStartModule::CmdName())) {

      std::string name = cmd.GetStr(CmdStartModule::ModuleArg());

      if (name.compare("*")==0) {
         dabc::Module* m(0);
         Iterator iter(GetAppFolder());

         while (iter.next_cast(m))
            m->Start();

         cmd_res = cmd_true;

      } else {
         ModuleRef m = FindModule(name.c_str());
         cmd_res = cmd_bool(m.Start());
      }
   } else
   if (cmd.IsName(CmdStopModule::CmdName())) {
      std::string name = cmd.GetStr(CmdStopModule::ModuleArg());

      if (name.compare("*")==0) {
         dabc::Module* m(0);
         Iterator iter(GetAppFolder());

         while (iter.next_cast(m))
            m->Stop();
      } else {
         ModuleRef m = FindModule(cmd.GetStr("Module",""));
         cmd_res = cmd_bool(m.Stop());
      }
   } else
   if (cmd.IsName(CmdDeleteModule::CmdName())) {
      ModuleRef ref = FindModule(cmd.GetStr(CmdDeleteModule::ModuleArg()));

      cmd_res = cmd_bool(ref()!=0);

      DOUT2(("Stop and delete module %s", ref.GetName()));

      ref.Destroy();

      DOUT2(("Stop and delete module done"));

   } else
   if (cmd.IsName(CmdDeletePool::CmdName())) {
      FindPool(cmd.GetField("PoolName")).Destroy();
   } else
   if (cmd.IsName("Print")) {
      DoPrint();
   } else

   // these are two special commands with postponed execution
   if (cmd.IsName(CmdConnectPorts::CmdName())) {

      int node1(0), node2(0);
      std::string item1, item2;

      cmd_res = cmd_false;

      DOUT3(("Connecting : %s %s", cmd.GetField("Port1Name"), cmd.GetField("Port2Name")));

      if (Url::DecomposeItemName(cmd.GetStdStr("Port1Name"), node1, item1) &&
          Url::DecomposeItemName(cmd.GetStdStr("Port2Name"), node2, item2)) {

         if (node1<0) node1 = NodeId();
         if (node2<0) node2 = NodeId();

         DOUT2(("Connect : %d %s %d %s", node1, item1.c_str(), node2, item2.c_str()));

         if ((node1 != NodeId()) && (node2 != NodeId()))
            cmd_res = cmd_true;
         else
         if (node1==node2) {

            Reference port1 = FindPort(item1.c_str());
            Reference port2 = FindPort(item2.c_str());

            DOUT2(("Connect : %s %s %p %p", item1.c_str(), item2.c_str(), port1(), port2()));

            if (port1.null() || port2.null())
               cmd_res = cmd_false;
            else
               cmd_res = dabc::LocalTransport::ConnectPorts(port1, port2);

         } else {

            FindModule(ConnMgrName()).Submit(cmd);

            cmd_res = cmd_postponed;
         }
      }
   } else
   if (cmd.IsName(CmdGetNodesState::CmdName())) {
      GetCommandChannel().Submit(cmd);

      cmd_res = cmd_postponed;
   } else
   if (cmd.IsName("Ping")) {
      cmd_res = cmd_true;
   } else
   if (cmd.IsName("ParameterEventSubscription")) {


      Worker* worker = (Worker*) cmd.GetPtr("Worker");
      std::string mask = cmd.GetStdStr("Mask");
      std::string remote = cmd.GetStdStr("RemoteWorker");

      DOUT2(("Subscription with mask %s", mask.c_str()));

      if (cmd.GetBool("IsSubscribe")) {

         // first add empty record to avoid usage of copy constructor
         fParEventsReceivers->push_back(ParamEventReceiverRec());
         ParamEventReceiverRec& rec = fParEventsReceivers->back();

         if (worker) worker->SetCleanupBit();
         rec.recv = worker;
         rec.remote_recv = remote;
         rec.name_mask = mask;
         rec.only_change = cmd.GetBool("OnlyChange");
      } else {
         ParamEventReceiverList::iterator iter = fParEventsReceivers->begin();
         while (iter!=fParEventsReceivers->end()) {
            if ((iter->name_mask==mask) && (iter->recv == worker) && (iter->remote_recv == remote))
               fParEventsReceivers->erase(iter++);
            else
               iter++;
         }
      }
      cmd_res = cmd_true;
   } else
   if (cmd.IsName("StopManagerMainLoop")) {
      if (fMgrStoppedTime.null())
         fMgrStoppedTime = dabc::Now();
   } else
      cmd_res = cmd_false;

   return cmd_res;
}

std::string dabc::Manager::GetLastCreatedDevName()
{
   LockGuard guard(fMgrMutex);
   return fLastCreatedDevName;
}

bool dabc::Manager::DoCreateMemoryPool(Command cmd)
{
   if (cmd.null()) return false;

   std::string poolname = cmd.Field(xmlPoolName).AsStdStr();
   if (poolname.empty()) {
      EOUT(("Pool name is not specified"));
      return false;
   }

   MemoryPoolRef ref = FindPool(poolname.c_str());
   if (ref.null()) {
      ref = new dabc::MemoryPool(poolname.c_str(), true);
      // TODO: make thread name for pool configurable
      ref()->AssignToThread(thread());
   }

   return ref()->Reconstruct(cmd);
}

bool dabc::Manager::ReplyCommand(Command cmd)
{
   // ReplyCommand return true, when command can be safely deleted

   if (cmd.IsName(CmdParameterEvent::CmdName())) {

      void* origin = cmd.GetPtr("#Iterator");

      for (ParamEventReceiverList::iterator iter = fParEventsReceivers->begin();
            iter != fParEventsReceivers->end(); iter++) {

         if (&(*iter) == origin) {
            iter->queue--;
            if (iter->queue<0)
               EOUT(("Internal error - parameters event queue negative"));
            return true;
         }
      }

      EOUT(("Did not find original record with receiver for parameter events"));

      return true;
   }


   return dabc::Worker::ReplyCommand(cmd);
}

void dabc::Manager::Print()
{
   Execute("Print");
}

bool dabc::Manager::DestroyObject(Reference ref)
{
   Object* obj = ref();

   if (obj==0) return true;

   if (obj->IsLogging())
      DOUT0(("dabc::Manager::DestroyObject %p %s", obj, obj->GetName()));

   bool fire = false;

   {
      LockGuard lock(fMgrMutex);

      // analyze that object presented in some dependency lists and mark record to process it
      DependPairList::iterator iter = fDepend->begin();
      while (iter != fDepend->end()) {
         if (iter->src() == obj) {
            iter->fire = iter->fire | 1;
            if (obj->IsLogging())
               DOUT0(("Find object %p as dependency source", obj));
         }

         if (iter->tgt == obj) {
            iter->fire = iter->fire | 2;
            if (obj->IsLogging())
               DOUT0(("Find object %p as dependency target", obj));
         }

         iter++;
      }

      if (fDestroyQueue==0) {
         fDestroyQueue = new ReferencesVector;
         fire = true;
      }

      fDestroyQueue->Add(ref);
   }

   // FIXME: check that thread is working - probably we can destroy ourself if manager does not active
   if (fire) FireEvent(evntDestroyObj);

   return true;
}

bool dabc::Manager::DoCleanupApplication()
{
   DOUT3(("DoCleanupApplication starts numchilds %u", GetAppFolder().NumChilds()));

   // destroy application if exist
   GetAppFolder().Destroy();

   ProcessDestroyQueue();

   Object::InspectGarbageCollector();

   return true;
}

bool dabc::Manager::MakeThreadFor(Worker* proc, const char* thrdname)
{
   if (proc==0) return false;

   if ((thrdname==0) || (strlen(thrdname)==0)) {
      DOUT2(("Thread name not specified - generate default, for a moment - processor name"));
      thrdname = proc->GetName();
   }

   if ((thrdname==0) || (strlen(thrdname)==0)) {
      EOUT(("Still no thread name - used name Thread"));
      thrdname = "Thread";
   }

   ThreadRef thrd = CreateThread(thrdname, proc->RequiredThrdClass());

   return thrd() ? proc->AssignToThread(thrd) : false;
}

void dabc::Manager::Sleep(double tmout, const char* prefix)
{
   if (tmout<=0.) return;

   ThreadRef thrd = CurrentThread();

   if (thrd()==0) {
      if (prefix) {
         fprintf(stdout, "%s    ", prefix);
         int sec = lrint(tmout);
         while (sec-->0) {
            fprintf(stdout, "\b\b\b%3d", sec);
            fflush(stdout);
            dabc::Sleep(1);
         }
         fprintf(stdout, "\n");
      } else {
         dabc::Sleep(tmout);
      }
   } else {
      if (prefix) {
         fprintf(stdout, "%s ",  prefix);
         fflush(stdout);
      }

      TimeStamp finish = dabc::Now() + tmout;
      double remain;

      while ((remain = finish - dabc::Now()) > 0) {

         if (prefix) {
            fprintf(stdout, "\b\b\b%3ld", lrint(remain));
            fflush(stdout);
         }

         thrd.RunEventLoop(remain > 1. ? 1 : remain);
      }

      if (prefix) {
         fprintf(stdout, "\b\b\b\n");
         fflush(stdout);
      }
   }
}

void dabc::Manager::DoPrint()
{
   dabc::Iterator iter(GetThreadsFolder());

   while (iter.next()) {
      Thread* thrd = dynamic_cast<Thread*> (iter.current());
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
   CommandsSet cli(thread());

   for (int node=0; node<NumNodes(); node++)
      if ((node!=NodeId()) && IsNodeActive(node)) {
         Command cmd("Ping");
         cmd.SetReceiver(node);
         cli.Add(cmd, this);
      }

   return cli.ExecuteSet(tmout);
}

std::string dabc::Manager::GetNodeName(int nodeid)
{
   if ((nodeid<0) || (nodeid>=NumNodes())) return std::string();

   LockGuard lock(fMgrMutex);

   if (fCfg==0) return std::string();

   return fCfg->NodeName(nodeid);

}

bool dabc::Manager::RegisterDependency(Object* src, Object* tgt, bool bidirectional)
{
   if ((src==0) || (tgt==0)) return false;

   // one only need cleanup for tgt, for src Reference will force object call
   tgt->SetCleanupBit();

   // we create record outside that mutexes not block each other
   DependPair rec(src,tgt);

   {
      LockGuard guard(fMgrMutex);

      fDepend->push_back(rec);
   }

   if (!bidirectional) return true;

   return RegisterDependency(tgt, src, false);
}

bool dabc::Manager::UnregisterDependency(Object* src, Object* tgt, bool bidirectional)
{
   if ((src==0) || (tgt==0)) return false;

   DependPair rec;

   {
      LockGuard guard(fMgrMutex);
      DependPairList::iterator iter = fDepend->begin();
      while (iter != fDepend->end()) {
         if ((iter->src() == src) && (iter->tgt == tgt)) {
            rec = *iter; // we should not release reference inside manager mutex
            fDepend->erase(iter++);
            break;
         } else
            iter++;
      }
   }

   // just do it in clear code and outside manager mutex
   rec.src.Release();

   if (!bidirectional) return true;

   return UnregisterDependency(tgt, src, false);
}

double dabc::Manager::ProcessTimeout(double last_diff)
{
   dabc::Logger::CheckTimeout();

   // we can process timeouts without mutex while vector can be only changed from the thread itself
   if (fTimedPars!=0)
      for (unsigned n=0; n<fTimedPars->GetSize(); n++) {
         ParameterContainer* par = (ParameterContainer*) fTimedPars->GetObject(n);
         if (par) par->ProcessTimeout(last_diff);
      }

   return 1.;
}

void dabc::Manager::AddFactory(Factory* factory)
{
   if (factory==0) return;

   GetFactoriesFolder(true).AddChild(factory, false);

   DOUT3(("Add factory %s", factory->GetName()));
}

bool dabc::Manager::CreateApplication(const char* classname, const char* appthrd)
{
   return Execute(CmdCreateApplication(classname, appthrd));
}

dabc::FileIO* dabc::Manager::CreateFileIO(const char* typ, const char* name, int option)
{
   FOR_EACH_FACTORY(
      FileIO* io = factory->CreateFileIO(typ, name, option);
      if (io!=0) return io;
   )

   return 0;
}

dabc::Object* dabc::Manager::ListMatchFiles(const char* typ, const char* filemask)
{
   FOR_EACH_FACTORY(
      Object* res = factory->ListMatchFiles(typ, filemask);
      if (res!=0) return res;
   )

   return 0;
}

void dabc::Manager::ProcessCtrlCSignal()
{
   DOUT0(("Process CTRL-C signal"));

   if (fMgrStoppedTime.null()) {
      fMgrStoppedTime = dabc::Now();
      return;
   }

   double spent = fMgrStoppedTime - dabc::Now();

   // TODO: make 10 second configurable
   if (spent<10.) return;

   DOUT0(("Ctrl-C repeated more than after 10 sec out of main loop - force manager halt"));

   HaltManager();

   DOUT0(("Exit after Ctrl-C"));

   exit(0);
}

void dabc::Manager::RunManagerMainLoop(double runtime)
{
   DOUT2(("Enter dabc::Manager::RunManagerMainLoop"));

   ThreadRef thrd = thread();

   if (thrd.null()) return;

   if (!fMgrStoppedTime.null()) {
      DOUT1(("Manager stopped before entering to the mainloop - stop running"));
      return;
   }

   if (thrd.IsRealThrd()) {
      DOUT3(("Manager has normal thread - just wait until application modules are stopped"));
   } else {
      DOUT3(("Run manager thread mainloop inside main process"));

      // to be sure that timeout processing is active
      // only via timeout one should be able to stop processing of main loop
      ActivateTimeout(1.);
   }

   TimeStamp start = dabc::Now();

   bool appstopped = false;


   while (true) {

      // we run even loop in units of 0.1 sec
      // TODO: make 0.1 sec configurable
      if (thrd.IsRealThrd())
         dabc::Sleep(0.1);
      else
         thrd.RunEventLoop(0.1);

      TimeStamp now = dabc::Now();

      ApplicationRef app = GetApp();

      if (app.IsFinished()) break;

      // check if stop time was not set
      if (fMgrStoppedTime.null()) {

         if ((runtime > 0) && (now > start + runtime)) fMgrStoppedTime = now;

         // TODO: logic with automatic stop of application should be implemented in the application itself
         if (app.IsWorkDone()) fMgrStoppedTime = now;
      }

      if (!fMgrStoppedTime.null() && !appstopped) {
         appstopped = true;
         app.Submit(InvokeAppFinishCmd());
      }

      // TODO: make 10 second configurable
      if (!fMgrStoppedTime.null())
         if (now > fMgrStoppedTime + 10.) break;
   }

   DOUT2(("Exit dabc::Manager::RunManagerMainLoop"));
}



bool dabc::Manager::Find(ConfigIO &cfg)
{
   while (cfg.FindItem(xmlContext)) {
      if (!fCfgHost.empty())
         if (!cfg.CheckAttr(xmlHostAttr, fCfgHost.c_str())) continue;

      if (fCfgHost != GetName())
         if (!cfg.CheckAttr(xmlNameAttr, GetName())) continue;

      return true;
   }

   return false;
}

// __________________________________ NEW CODE ______________________________________


bool dabc::Manager::ConnectControl()
{

   if (NumNodes()==1) return true;

   // FIXME: move this code later in dabc_exe - one need connection manager only when
   //        several nodes are running and should be connected with each other

   return !GetCommandChannel(true).null();

   /* int res = GetCommandChannel(true).Execute("ConnectAll", 10);

   DOUT0(("All slaves connected res = %d", res));

   for (int n=0;n<NumNodes();n++) {
      dabc::Command cmd("Ping");
      cmd.SetReceiver(n);

      TimeStamp t1 = dabc::Now();

      res = Execute(cmd, 5);

      TimeStamp t2 = dabc::Now();

      DOUT0(("Ping %d -> %d res = %d  tm = %8.6f", NodeId(), n, res, t2-t1));
   }

   return true;

   */

}

void dabc::Manager::DisconnectControl()
{
   // WorkerSleep(5.);

   FindModule(ConnMgrName()).Destroy();

   GetCommandChannel().Execute(CmdShutdownControl(), 1.);

   GetCommandChannel().Destroy();
}

// =================================== classes from ManagerRef class ==================================


dabc::ThreadRef dabc::Manager::FindThread(const char* name, const char* required_class)
{
   ThreadRef ref = GetThreadsFolder().FindChild(name);

   if (ref.null()) return ref;

   if ((required_class!=0) && !ref()->CompatibleClass(required_class)) ref.Release();

   return ref;
}

dabc::ThreadRef dabc::Manager::CurrentThread()
{
   ReferencesVector vect;

   if (GetThreadsFolder().GetAllChildRef(&vect))
      while (vect.GetSize()>0) {
         ThreadRef thrd = vect.TakeLast();
         if (thrd.IsItself()) return thrd;
      }

   return ThreadRef();
}

dabc::ThreadRef dabc::Manager::CreateThread(const std::string& thrdname, const char* thrdclass, const char* thrddev, Command cmd)
{
   ThreadRef thrd = FindThread(thrdname.c_str());

   if (!thrd.null()) {
      if (thrd()->CompatibleClass(thrdclass)) {
      } else {
         EOUT(("Thread %s of class %s exists and incompatible with %s class",
            thrdname.c_str(), thrd()->ClassName(), (thrdclass ? thrdclass : "---")));
         thrd.Release();
      }
      return thrd;
   }

   DOUT3(("CreateThread %s of class %s, is any %p", thrdname.c_str(), (thrdclass ? thrdclass : "---"), thrd()));

   FOR_EACH_FACTORY(
      thrd = factory->CreateThread(GetThreadsFolder(true), thrdclass, thrdname.c_str(), thrddev, cmd);
      if (!thrd.null()) break;
   )

   bool noraml_thread = true;
   if ((thrdname == MgrThrdName()) && cfg())
      noraml_thread = cfg()->NormalMainThread();

   DOUT2(("Starting thread %s as noraml %s refcnt %d", thrd.GetName(), DBOOL(noraml_thread), thrd()->fObjectRefCnt));

   if (!thrd.null())
      if (!thrd()->Start(10, noraml_thread)) {
         EOUT(("Thread %s cannot be started!!!", thrdname.c_str()));
         thrd.Destroy();
      }

   DOUT2(("Create thread %s of class %s thrd %p refcnt %d", thrdname.c_str(), (thrdclass ? thrdclass : "---"), thrd(), thrd()->fObjectRefCnt));

   return thrd;
}


// ========================================== ManagerRef methods ================================

bool dabc::ManagerRef::CreateConnectionManager()
{
   ModuleRef m = FindModule(Manager::ConnMgrName());

   if (!m.null()) return true;

   return CreateModule("dabc::ConnectionManager", Manager::ConnMgrName(), Manager::MgrThrdName());
}

bool dabc::ManagerRef::CreateModule(const char* classname, const char* modulename, const char* thrdname)
{
   return Execute(CmdCreateModule(classname, modulename, thrdname));
}

dabc::ThreadRef dabc::ManagerRef::CreateThread(const std::string& thrdname, const char* classname)
{
   return GetObject() ? GetObject()->CreateThread(thrdname, classname) : dabc::ThreadRef();
}

dabc::ThreadRef dabc::ManagerRef::CurrentThread()
{
   return GetObject() ? GetObject()->CurrentThread() : dabc::ThreadRef();
}

bool dabc::ManagerRef::CreateDevice(const char* classname, const char* devname, const char* devthrd)
{
   return Execute(CmdCreateDevice(classname, devname, devthrd));
}

bool dabc::ManagerRef::DestroyDevice(const char* devname)
{
   return Execute(CmdDestroyDevice(devname));
}

dabc::WorkerRef dabc::ManagerRef::FindDevice(const std::string& name)
{
   return GetObject() ? GetObject()->FindDevice(name.c_str()) : dabc::WorkerRef();
}


dabc::Reference dabc::ManagerRef::GetAppFolder(bool force)
{
   return GetObject() ? GetObject()->GetAppFolder(force) : dabc::Reference();
}


dabc::ModuleRef dabc::ManagerRef::FindModule(const char* name)
{
   return GetObject() ? GetObject()->FindModule(name) : dabc::ModuleRef();
}

void dabc::ManagerRef::StartModule(const char* modulename)
{
   Execute(dabc::CmdStartModule(modulename));
}

void dabc::ManagerRef::StopModule(const char* modulename)
{
   Execute(dabc::CmdStopModule(modulename));
}

bool dabc::ManagerRef::StartAllModules()
{
   return Execute(CmdStartModule("*"));
}

bool dabc::ManagerRef::StopAllModules()
{
   return Execute(CmdStopModule("*"));
}

bool dabc::ManagerRef::DeleteModule(const char* name)
{
   return Execute(CmdDeleteModule(name));
}

dabc::Reference dabc::ManagerRef::FindItem(const std::string& name)
{
   return GetObject() ? GetObject()->FindItem(name) : Reference();
}

dabc::Reference dabc::ManagerRef::FindPort(const std::string& portname)
{
   return GetObject() ? GetObject()->FindPort(portname.c_str()) : Reference();
}

dabc::Parameter dabc::ManagerRef::FindPar(const std::string& parname)
{
   return GetObject() ? GetObject()->FindItem(parname) : Reference();
}

dabc::ApplicationRef dabc::ManagerRef::GetApp()
{
   return GetObject() ? GetObject()->GetApp() : dabc::ApplicationRef();
}

bool dabc::ManagerRef::CleanupApplication()
{
   // this method must delete all threads, modules and pools and clean device drivers

   return Execute(CmdCleanupApplication());
}

int dabc::ManagerRef::NodeId() const
{
   return GetObject() ? GetObject()->NodeId() : 0;
}

int dabc::ManagerRef::NumNodes() const
{
   return GetObject() ? GetObject()->NumNodes() : 0;
}

std::string dabc::ManagerRef::ComposeUrl(Object* ptr)
{
   return Url::ComposeItemName(NodeId(), ptr->ItemName());
}

bool dabc::ManagerRef::ParameterEventSubscription(Worker* ptr, bool subscribe, const std::string& mask, bool onlychangeevent)
{
   if (ptr == 0) return false;

   // TODO: by the subscription to remote node first register receiver on local node and
   //       only then submit registration to remote.
   //       One should avoid multiple parallel subscription to remote node

   int nodeid(-1);
   std::string itemname;

   if (!Url::DecomposeItemName(mask, nodeid, itemname)) {
      EOUT(("Wrong parameter mask %s", mask.c_str()));
      return false;
   }

   Command cmd("ParameterEventSubscription");

   cmd.SetBool("IsSubscribe", subscribe);
   cmd.SetStr("Mask", mask);
   cmd.SetBool("OnlyChange", onlychangeevent);

   if ((nodeid<0) || (nodeid==NodeId())) {
      // this is registration for local parameters
      cmd.SetPtr("Worker", ptr);
      return Execute(cmd);
   }

   cmd.SetStr("RemoteWorker", Url::ComposeItemName(NodeId(), ptr->ItemName()));
   cmd.SetReceiver(nodeid);

   // do registration asynchron
   return Submit(cmd);
}

bool dabc::ManagerRef::IsLocalItem(const std::string& name)
{
   std::string itemname;
   int nodeid(-1);
   if (!Url::DecomposeItemName(name, nodeid, itemname)) return true;
   return (nodeid<0) || (nodeid==NodeId());
}


dabc::ConnectionRequest dabc::ManagerRef::Connect(const std::string& port1name, const std::string& port2name)
{
   PortRef port1 = FindPort(port1name);

   PortRef port2 = FindPort(port2name);

   if (IsLocalItem(port1name) && port1.null()) {
      EOUT(("Did not found port %s", port1name.c_str()));
      return dabc::ConnectionRequest();
   }

   if (IsLocalItem(port2name) && port2.null()) {
      EOUT(("Did not found port %s", port2name.c_str()));
      return dabc::ConnectionRequest();
   }

   if (port1.null() && port2.null()) return dabc::ConnectionRequest();

   DOUT2(("Connect ports %s %p %s %p", port1name.c_str(), port1(), port2name.c_str(), port2()));

   if (!port1.null() && !port2.null()) {
      // make local connection immediately
      dabc::LocalTransport::ConnectPorts(port1, port2);
      return dabc::ConnectionRequest();
   }

   dabc::ConnectionRequest req;

   if (!port1.null())
      req = port1()->MakeConnReq(port2name, true);

   if (!port2.null())
      req = port2()->MakeConnReq(port1name, false);

   // if not configured differently, specify
   if (req.GetConnDevice().empty() && GetObject())
      req.SetConnDevice(GetObject()->GetLastCreatedDevName());

   return req;
}


bool dabc::ManagerRef::ActivateConnections(double tmout)
{
   // ensure that all commands are executed, for instance creation of connection manager is done
   SyncWorker();

   ModuleRef conn = FindModule(Manager::ConnMgrName());
   if (conn.null()) return true;

   dabc::Command cmd("ActivateConnections");
   cmd.SetTimeout(tmout);
   return conn.Execute(cmd);
}

bool dabc::ManagerRef::CreateTransport(const std::string& portname, const std::string& transportkind, const std::string& thrdname)
{
   return Execute(CmdCreateTransport(portname, transportkind, thrdname));
}

void dabc::ManagerRef::StopApplication()
{
   // Manager will be stopped regularly if it is in running
   if (GetObject())
      if (GetObject()->fMgrStoppedTime.null())
         GetObject()->fMgrStoppedTime = dabc::Now();
}

bool dabc::ManagerRef::CreateMemoryPool(const char* poolname,
                                     unsigned buffersize,
                                     unsigned numbuffers,
                                     unsigned refcoeff)
{

   CmdCreateMemoryPool cmd(poolname);

   cmd.SetMem(buffersize, numbuffers);

   cmd.SetRefs(refcoeff);

   return Execute(cmd);
}

dabc::Reference dabc::ManagerRef::CreateObject(const std::string& classname, const std::string& objname)
{
   CmdCreateObject cmd(classname, objname);

   if (!Execute(cmd)) return dabc::Reference();

   return cmd.GetRef("Object");
}


void dabc::ManagerRef::Sleep(double tmout, const char* prefix)
{
   if (GetObject())
      GetObject()->Sleep(tmout, prefix);
   else
      dabc::Sleep(tmout);
}
