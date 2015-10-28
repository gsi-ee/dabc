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
#include "dabc/api.h"

#include "dabc/Transport.h"
#include "dabc/Module.h"
#include "dabc/Command.h"
#include "dabc/MemoryPool.h"
#include "dabc/Device.h"
#include "dabc/Url.h"
#include "dabc/Thread.h"
#include "dabc/Factory.h"
#include "dabc/Application.h"
#include "dabc/Iterator.h"
#include "dabc/BinaryFileIO.h"
#include "dabc/Configuration.h"
#include "dabc/ConnectionManager.h"
#include "dabc/ReferencesVector.h"
#include "dabc/CpuInfoModule.h"
#include "dabc/MultiplexerModule.h"
#include "dabc/SocketFactory.h"
#include "dabc/Hierarchy.h"
#include "dabc/Publisher.h"

namespace dabc {

   ManagerRef mgr;

   class StdManagerFactory : public Factory {
      public:
         StdManagerFactory(const std::string& name) : Factory(name) { }

         virtual Module* CreateModule(const std::string& classname, const std::string& modulename, Command cmd);

         virtual Reference CreateObject(const std::string& classname, const std::string& objname, dabc::Command cmd);

         virtual Reference CreateThread(Reference parent, const std::string& classname, const std::string& thrdname, const std::string& thrddev, Command cmd);

         virtual DataOutput* CreateDataOutput(const std::string& typ);

         virtual DataInput* CreateDataInput(const std::string& typ);
   };


   /** \brief Keeps dependency between two objects.
    *
    * When DependPair#tgt object want to be deleted,
    * ObjectDestroyed() method will be called in DependPair#src object to perform correct cleanup
    * It is supposed that DependPair#src has reference on DependPair#tgt and
    * this reference should be released, otherwise DependPair#tgt object will be not possible to destroy
    */
   struct DependPair {
      Reference src; ///< reference on object which want to be informed when DependPair#tgt object is deleted
      Object* tgt;   ///< when this object deleted, DependPair#src will be informed
      int fire;      ///< how to proceed pair 0 - remain, 1 - inform src, 2 - just delete

      DependPair() : src(), tgt(0), fire(0) {}
      DependPair(Object* _src, Object* _tgt) : src(_src), tgt(_tgt), fire(0) {}
      DependPair(const DependPair& d) : src(d.src), tgt(d.tgt), fire(d.fire) {}
   };

   class DependPairList : public std::list<DependPair> {};


   struct ParamEventReceiverRec {
      WorkerRef    recv;          ///< only workers can be receiver of the parameters events
      std::string  remote_recv;   ///< address of remote receiver of parameter events
      bool         only_change;   ///< specify if only parameter-change events are produced
      std::string  name_mask;     ///< mask only for parameter names, useful when only specific names are interested
      std::string  fullname_mask; ///< mask for parameter full names, necessary when full parameter name is important
      int          queue;         ///< number of parameters events submitted, if bigger than some limit events will be skipped

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

   class BlockingOutput : public DataOutput {
      protected:
         double    fBlockTm;
         bool      fSleep;
         int       fErrCounter;  ///< counter till error

      public:
         BlockingOutput(const dabc::Url& url) :
            DataOutput(url),
            fBlockTm(1.),
            fSleep(true),
            fErrCounter(0)
         {
            fBlockTm = url.GetOptionDouble("time", 1.);
            fSleep = url.GetOptionBool("sleep", true);
            fErrCounter = url.GetOptionInt("err", 0);
         }

         unsigned Write_Buffer(Buffer& buf)
         {
            buf.Release();
            if (fErrCounter > 0)
               if (--fErrCounter == 0) return do_Error;

            if (fSleep) {
               dabc::Sleep(fBlockTm);
            } else {
               TimeStamp tm;
               tm.GetNow();
               int cnt(0);
               while (!tm.Expired(fBlockTm)) cnt++;
            }
            return do_Ok;
         }
   };
}

dabc::Module* dabc::StdManagerFactory::CreateModule(const std::string& classname, const std::string& modulename, Command cmd)
{
   if (classname == "dabc::CpuInfoModule")
      return new CpuInfoModule(modulename, cmd);

   if (classname == "dabc::ConnectionManager")
      return new ConnectionManager(modulename, cmd);

   if (classname == "dabc::MultiplexerModule")
      return new MultiplexerModule(modulename, cmd);

   if (classname == "dabc::RepeaterModule")
      return new dabc::RepeaterModule(modulename, cmd);

   return 0;
}

dabc::Reference dabc::StdManagerFactory::CreateObject(const std::string& classname, const std::string& objname, dabc::Command cmd)
{
   if (classname == "dabc::Publisher")
      return new dabc::Publisher(objname, cmd);

   return dabc::Factory::CreateObject(classname, objname, cmd);
}


dabc::Reference dabc::StdManagerFactory::CreateThread(Reference parent, const std::string& classname, const std::string& thrdname, const std::string& thrddev, Command cmd)
{
   dabc::Thread* thrd = 0;

   if (classname.empty() || (classname == typeThread))
      thrd = new Thread(parent, thrdname, cmd);

   return Reference(thrd);
}


dabc::DataOutput* dabc::StdManagerFactory::CreateDataOutput(const std::string& typ)
{
   dabc::Url url(typ);
   if (url.GetProtocol()=="bin") {
      return new dabc::BinaryFileOutput(url);
   } else
   if (url.GetProtocol() == "block") {
      return new dabc::BlockingOutput(url);
   }

   return 0;
}

dabc::DataInput* dabc::StdManagerFactory::CreateDataInput(const std::string& typ)
{
   dabc::Url url(typ);
   if (url.GetProtocol()=="bin") {
      return new dabc::BinaryFileInput(url);
   }

   return 0;
}


/** Helper class to destroy manager when finishing process
 * TODO: Check it precisely */
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
               printf("Destroy manager\n");
               dabc::mgr()->HaltManager();
               dabc::mgr.Destroy();
               printf("Destroy manager done\n");
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


dabc::Manager::Manager(const std::string& managername, Configuration* cfg) :
   Worker(0, managername),
   fMgrStoppedTime(),
   fAppFinished(false),
   fMgrMutex(0),
   fDestroyQueue(0),
   fParsQueue(1024),
   fTimedPars(0),
   fParEventsReceivers(0),
   fDepend(0),
   fCfg(cfg),
   fCfgHost(),
   fNodeId(0),
   fNumNodes(1),
   fLocalAddress(),
   fThrLayout(layoutBalanced),
   fLastCreatedDevName()
{
   fInstance = this;
   fInstanceId = MagicInstanceId;

   if (dabc::mgr.null()) {
      dabc::mgr = dabc::ManagerRef(this);
      dabc::SetDebugPrefix(GetName());
   }

   if (cfg) {
      fCfgHost = cfg->MgrHost();
      fNodeId = cfg->MgrNodeId();
      fNumNodes = cfg->MgrNumNodes();

      std::string layout = cfg->ThreadsLayout();

      if (layout=="minimal") fThrLayout = layoutMinimalistic; else
      if (layout=="permodule") fThrLayout = layoutPerModule; else
      if (layout=="balanced") fThrLayout = layoutBalanced; else
      if (layout=="maximal") fThrLayout = layoutMaximal; else layout.clear();

      if (!layout.empty()) DOUT0("Set threads layout to %s", layout.c_str());
   }

   // we create recursive mutex to avoid problem in Folder::GetFolder method,
   // where constructor is called under locked mutex,
   // let say - there is no a big problem, when mutex locked several times from one thread

   fMgrMutex = new Mutex(true);

   fDepend = new DependPairList;

   fLastCreatedDevName.clear();

   fParEventsReceivers = new ParamEventReceiverList;

   // this should automatically add all factories to the manager
   ProcessFactory(new dabc::StdManagerFactory("std"));

   ProcessFactory(new dabc::SocketFactory("sockets"));

   // append factories, which are created too fast
   if (fFirstFactoriesId == MagicInstanceId) {
      for (unsigned n=0;n<sizeof(fFirstFactories)/sizeof(void*); n++)
         if (fFirstFactories[n]) {
            ProcessFactory(fFirstFactories[n]);
            fFirstFactories[n] = 0;
         }
   }

   MakeThreadForWorker(MgrThrdName());

   ActivateTimeout(1.);
}

dabc::Manager::~Manager()
{
   // stop thread that it is not try to access objects which we are directly deleting
   // normally, as last operation in the main() program must be HaltManeger(true)
   // call, which suspend and erase all items in manager

//   dabc::SetDebugLevel(3);
//   dabc::SetDebugLevel(3);

   DOUT3("~Manager -> DestroyQueue");

   if (fDestroyQueue && (fDestroyQueue->GetSize()>0))
      EOUT("Manager destroy queue is not empty");

   delete fParEventsReceivers; fParEventsReceivers = 0;

   delete fDestroyQueue; fDestroyQueue = 0;

   if (fTimedPars!=0) {
      if (fTimedPars->GetSize() > 0) {
         EOUT("Manager timed parameters list not empty");
      }

      delete fTimedPars;
      fTimedPars = 0;
   }

   DOUT3("~Manager -> ~fDepend");

   if (fDepend && (fDepend->size()>0))
      EOUT("Dependencies parameters list not empty");

   delete fDepend; fDepend = 0;

   DOUT3("~Manager -> ~fMgrMutex");

   delete fMgrMutex; fMgrMutex = 0;

   if (dabc::mgr()==this) {
      DOUT1("Normal EXIT");
      if (dabc::Logger::Instance())
         dabc::Logger::Instance()->LogFile(0);
   } else {
      EOUT("What ??? !!!");
   }

   fInstance = 0;
   fInstanceId = 0;
}


void dabc::Manager::HaltManager()
{
   ThreadRef thrd = thread();
   // only for case, when manager thread does not run its own main loop,
   // we could help it to finish processing
   if (thrd.IsRealThrd()) thrd.Release();

   FindModule(ConnMgrName()).Destroy();

   DettachFromThread();

   RemoveChilds();

   // run dummy event loop several seconds to complete events which may be submitted there

   int cnt = 0;
   TimeStamp tm1 = dabc::Now();
   double halttime = fCfg ? fCfg->GetHaltTime() : 0.;
   if (halttime<=0.) halttime = 0.7;

   do {

      cnt++;

      ProcessParameterEvents();

      ProcessDestroyQueue();

      if (!thrd.null()) {
         thrd()->SingleLoop(0, 0.001);
         if ((thrd()->TotalNumberOfEvents()==0) || tm1.Expired(halttime*0.7)) thrd.Release();
      } else {
         dabc::Sleep(0.001);
      }

   } while ((dabc::Thread::NumThreadInstances() > 0) && !tm1.Expired(halttime));

   TimeStamp tm2 = dabc::Now();

   if (dabc::Thread::NumThreadInstances() > 0) {
      EOUT("!!!!!!!!! There are still %u threads - anyway declare manager halted spent: %5.3f s!!!!!!", dabc::Thread::NumThreadInstances(), tm2 - tm1);
   } else {
      DOUT1("All threads stopped after %5.3f s (loop count = %d)", tm2-tm1, cnt);
   }

#ifdef DABC_EXTRA_CHECKS
   dabc::Object::DebugObject();
#endif

   dabc::Object::InspectGarbageCollector();

   DOUT3("dabc::Manager::HaltManager done refcnt = %u", fObjectRefCnt);
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
            DOUT0("Submit worker %p to thread", obj);
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

   DOUT3("MGR: Process destroy QUEUE vect = %u", (vect ? vect->GetSize() : 0));

   // FIXME: remove timed parameters, otherwise it is not possible to delete it
   // if (fTimedPars!=0)
   //      for(unsigned n=0;n<vect->GetSize();n++)
   //         fTimedPars->Remove(vect->GetObject(n));

   vect->Clear(true);
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

      //LockGuard lock(ObjectMutex());
      DABC_LOCKGUARD(ObjectMutex(), "Inserting new event into fParsQueue");

      fire = fParsQueue.Size() == 0;

      // add parameter event to the queue
      ParamRec* rec = fParsQueue.PushEmpty();

      if (rec!=0) {
         // memset(rec, 0, sizeof(ParamRec));
         rec->par << parref; // we are trying to avoid parameter locking under locked queue mutex
         rec->event = evid;
      }

   }

//   DOUT0("FireParamEvent id %d par %s", evid, par->GetName());

   if (fire) FireEvent(evntManagerParam);
}

bool dabc::Manager::ProcessParameterEvents()
{
   // do not process more than 100 events a time
   int maxcnt = 100;

   while (maxcnt-->0) {

      ParamRec rec;

      {
         DABC_LOCKGUARD(ObjectMutex(), "Extracting event from fParsQueue");
         // LockGuard lock(ObjectMutex());
         if (fParsQueue.Size()==0) return false;
         rec.par << fParsQueue.Front().par;
         rec.event = fParsQueue.Front().event;
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
            DOUT2("Parameter %s configured checkadd %s", rec.par.GetName(), DBOOL(checkadd));
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
         if (!fTimedPars->HasObject(rec.par())) {
            Reference ref = rec.par;
            fTimedPars->Add(ref);
         }
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
            value = rec.par.Value().AsStr();

         if (iter->queue > 1000) {
            EOUT("Too many events for receiver %s - block any following", iter->recv.GetName());
            continue;
         }

         if (!iter->recv.null() && !iter->recv.CanSubmitCommand()) {
            DOUT4("receiver %s cannot be used to submit command - ignore", iter->recv.GetName());
            continue;
         }

         // TODO: probably one could use special objects and not command to deliver parameter events to receivers
         CmdParameterEvent evnt(fullname, value, rec.event, attrmodified);

         evnt.SetPtr("#Iterator", &(*iter));

         iter->queue++;

         Assign(evnt);

         if (iter->remote_recv.length() > 0) {
            evnt.SetReceiver(iter->remote_recv);
            GetCommandChannel().Submit(evnt);
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
   DOUT5("MGR::ProcessEvent %s", evnt.asstring().c_str());

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
   DOUT5("MGR::ProcessEvent %s done", evnt.asstring().c_str());
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
   std::string server, itemname;

   if (islocal) {
      itemname = name;
   } else {
      if (!DecomposeAddress(name, islocal, server, itemname)) return 0;
      if (!islocal) return 0;
   }

   if (itemname.empty()) return 0;

   return FindChildRef(itemname.c_str());
}

dabc::ModuleRef dabc::Manager::FindModule(const std::string& name)
{
   return FindItem(name);
}


dabc::Reference dabc::Manager::FindPort(const std::string& name)
{
   PortRef ref = FindItem(name);

   return ref;
}


dabc::Reference dabc::Manager::FindPool(const std::string& name)
{
   MemoryPoolRef ref = FindItem(name);

   return ref;
}

dabc::WorkerRef dabc::Manager::FindDevice(const std::string& name)
{
   DeviceRef ref = FindItem(name);

   return ref;
}

dabc::ApplicationRef dabc::Manager::app()
{
   return FindChildRef(xmlAppDfltName);
}

void dabc::Manager::FillItemName(const Object* ptr, std::string& itemname, bool compact)
{
   itemname.clear();

   if (ptr==0) return;

   if (!compact) itemname = "/";
   ptr->FillFullName(itemname, this);
}

int dabc::Manager::PreviewCommand(Command cmd)
{
   // check if command dedicated for other node, module and so on
   // returns: cmd_ignore - command will be processed in manager itself
   //          cmd_postponed - command redirected to other instance
   //          ..            - command is executed

   std::string url = cmd.GetReceiver();

   bool islocal(true);
   std::string server,itemname;

   if (!url.empty() && DecomposeAddress(url, islocal, server, itemname)) {

      if (cmd.GetBool("#local_cmd")) islocal = true;

      DOUT5("MGR: Preview command %s item %s tgtnode %d", cmd.GetName(), url.c_str(), tgtnode);

      if (!islocal) {

         if (GetCommandChannel().Submit(cmd)) return cmd_postponed;

         EOUT("Cannot submit command to remote server %s", server.c_str());
         return cmd_false;

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

         EOUT("Did not found receiver %s for command %s", itemname.c_str(), cmd.GetName());

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

dabc::WorkerRef dabc::Manager::DoCreateModule(const std::string& classname, const std::string& modulename, Command cmd)
{
   ModuleRef mdl = FindModule(modulename);

   if (!mdl.null()) {
      DOUT4("Module name %s already exists", modulename.c_str());

   } else {

      FOR_EACH_FACTORY(
         mdl = factory->CreateModule(classname, modulename, cmd);
         if (!mdl.null()) break;
      )

      if (mdl.null()) {
         EOUT("Cannot create module of type %s", classname.c_str());
         return mdl;
      }

      std::string thrdname = mdl.Cfg(xmlThreadAttr, cmd).AsStr();

      if (thrdname.empty())
         switch (GetThreadsLayout()) {
            case layoutMinimalistic: thrdname = ThreadName(); break;
            case layoutPerModule: thrdname = modulename + "Thrd"; break;
            case layoutBalanced: thrdname = modulename + "Thrd"; break;
            case layoutMaximal: thrdname = modulename + "Thrd"; break;
            default: thrdname = modulename + "Thrd"; break;
         }

      mdl.MakeThreadForWorker(thrdname);

      mdl.ConnectPoolHandles();
   }

   return mdl;
}


dabc::Reference dabc::Manager::DoCreateObject(const std::string& classname, const std::string& objname, Command cmd)
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
   DOUT5("MGR: Execute %s\n%s", cmd.GetName(), cmd.SaveToXml(false).c_str());

   int cmd_res = cmd_true;

   if (cmd.IsName(CmdCreateModule::CmdName())) {
      std::string classname = cmd.GetStr(xmlClassAttr);
      std::string modulename = cmd.GetStr(CmdCreateModule::ModuleArg());

      ModuleRef ref = DoCreateModule(classname, modulename, cmd);
      cmd_res = cmd_bool(!ref.null());

   } else
   if (cmd.IsName("InitFactories")) {
      FOR_EACH_FACTORY(
         factory->Initialize();
      )
      cmd_res = cmd_true;
   } else
   if (cmd.IsName(CmdCreateApplication::CmdName())) {
      std::string classname = cmd.GetStr("AppClass");

      if (classname.empty()) classname = typeApplication;

      ApplicationRef appref = app();

      if (!appref.null() && (classname == appref.ClassName())) {
         DOUT2("Application of class %s already exists", classname.c_str());
      } else
      if (classname != typeApplication) {
         appref.Destroy();

         FOR_EACH_FACTORY(
            appref = factory->CreateApplication(classname, cmd);
            if (!appref.null()) break;
         )
      } else {
         appref.Destroy();

         void* func = dabc::Factory::FindSymbol(fCfg->InitFuncName());

         appref = new Application();

         if (func) appref()->SetInitFunc((Application::ExternalFunction*)func);
      }


      std::string appthrd = appref.Cfg(xmlThreadAttr, cmd).AsStr();

      if (appthrd.empty())
         switch (GetThreadsLayout()) {
            case layoutMinimalistic: appthrd = ThreadName(); break;
            default: appthrd = AppThrdName(); break;
         }


      appref.MakeThreadForWorker(appthrd);

      cmd_res = cmd_bool(!appref.null());

      if (appref.null()) EOUT("Cannot create application of class %s", classname.c_str());
                    else DOUT2("Application of class %s thrd %s created", classname.c_str(), appthrd.c_str());

   } else

   if (cmd.IsName(CmdCreateDevice::CmdName())) {
      std::string classname = cmd.GetStr("DevClass");
      std::string devname = cmd.GetStr("DevName");
      if (devname.empty()) devname = classname;

      WorkerRef dev = FindDevice(devname);

      cmd_res = cmd_false;

      if (!dev.null()) {
        if (classname == dev.ClassName()) {
           DOUT4("Device %s of class %s already exists", devname.c_str(), classname.c_str());
           cmd_res = cmd_true;
        } else {
           EOUT("Device %s has other class name %s than requested", devname.c_str(), dev.ClassName(), classname.c_str());
        }
      } else {
         FOR_EACH_FACTORY(
            dev = factory->CreateDevice(classname, devname, cmd);
            if (!dev.null()) break;
         )

         if (dev.null()) {
            EOUT("Cannot create device %s of class %s", devname.c_str(), classname.c_str());
         } else {

            std::string thrdname = dev.Cfg(xmlThreadAttr, cmd).AsStr();

            if (thrdname.empty())
               switch (GetThreadsLayout()) {
                  case layoutMinimalistic: thrdname = ThreadName(); break;
                  case layoutPerModule:
                  case layoutBalanced:
                  case layoutMaximal: thrdname = devname + "Thrd"; break;
                  default: thrdname = devname + "Thrd"; break;
               }

            dev.MakeThreadForWorker(thrdname);

            cmd_res = cmd_true;
         }
      }

      if ((cmd_res==cmd_true) && !devname.empty()) {
         LockGuard guard(fMgrMutex);
         fLastCreatedDevName = devname;
      }
   } else
   if (cmd.IsName(CmdDestroyDevice::CmdName())) {
      std::string devname = cmd.GetStr("DevName");

      Reference dev = FindDevice(devname);

      cmd_res = cmd_bool(!dev.null());

      dev.Destroy();
   } else
   if (cmd.IsName(CmdCreateTransport::CmdName())) {

      CmdCreateTransport crcmd = cmd;
      // TODO: make url xml node attribute as for parameter

      std::string trkind = crcmd.TransportKind();
      std::string portname = crcmd.PortName();

      PortRef port = FindPort(portname);
      if (trkind.empty()) {
         trkind = port.Cfg("url", cmd).AsStr();

         Url url(trkind);
         if (url.IsValid()) {

            bool hasoptions = url.GetOptions().length() > 0;

            for (int cnt = 0; cnt < 3; cnt++) {
               std::string optname = "urlopt";
               if (cnt>0) dabc::formats(optname,"urlopt%d",cnt);
               std::string tropt = port.Cfg(optname, cmd).AsStr();
               if (tropt.length() > 0) {
                  trkind.append(hasoptions ? "&" : "?");
                  trkind.append(tropt);
                  hasoptions = true;
               }
            }
         }
      }

      if (port.null()) {
         EOUT("Ports %s not found - cannot create transport", crcmd.PortName().c_str());
         return cmd_false;
      }

      if (trkind.empty()) {
         // disconnect will be done via special command
         port.Disconnect();
         return cmd_true;
      }

      WorkerRef dev = FindDevice(trkind);

      Url url(trkind);
      if (dev.null() && url.IsValid() && ((url.GetProtocol()=="dev") || (url.GetProtocol()=="device")))
         dev = FindDevice(url.GetFullName());

      if (!dev.null()) {
         cmd.SetStr("url", trkind); // provide complete url to the device
         dev.Submit(cmd);
         return cmd_postponed;
      }

      PortRef port2 = FindPort(trkind);
      if (!port2.null()) {
         // this is local connection between two ports
         cmd_res = dabc::LocalTransport::ConnectPorts(port2, port, cmd);
         // connect also bind ports (if exists)
         if (cmd_res == cmd_true)
            cmd_res = dabc::LocalTransport::ConnectPorts(port.GetBindPort(), port2.GetBindPort(), cmd);

         return cmd_res;
      }


      cmd_res = cmd_false;

      DOUT1("Request transport for port %s kind %s", port.ItemName().c_str(), trkind.c_str());

      TransportRef tr;
      FOR_EACH_FACTORY(
         tr = factory->CreateTransport(port, trkind, cmd);
         if (!tr.null()) break;
      )

      if (!tr.null()) {

         if (portname != crcmd.PortName()) {
            portname = crcmd.PortName();
            DOUT0("Port name for created transport was changed on the fly %s", portname.c_str());
            port = FindPort(portname);
            if (port.null()) { tr.Destroy(); return cmd_false; }
         }

         std::string thrdname = port.Cfg(xmlThreadAttr, cmd).AsStr();
         if (thrdname.empty())
            switch (GetThreadsLayout()) {
               case layoutMinimalistic: thrdname = ThreadName(); break;
               case layoutPerModule: thrdname = port.GetModule().ThreadName(); break;
               case layoutBalanced: thrdname = port.GetModule().ThreadName() + (port.IsInput() ? "Inp" : "Out"); break;
               case layoutMaximal: thrdname = port.GetModule().ThreadName() + port.GetName(); break;
               default: thrdname = port.GetModule().ThreadName(); break;
            }

         DOUT3("Creating thread %s for transport", thrdname.c_str());

         if (!tr.MakeThreadForWorker(thrdname)) {
            EOUT("Fail to create thread for transport");
            tr.Destroy();
         } else {
            tr.ConnectPoolHandles();
            cmd_res = cmd_true;
            if (port.IsInput())
               dabc::LocalTransport::ConnectPorts(tr.OutputPort(), port, cmd);
            if (port.IsOutput())
               dabc::LocalTransport::ConnectPorts(port, tr.InputPort(), cmd);
         }

         DOUT3("Created transport for port %s is port connected %s", port.ItemName().c_str(), DBOOL(port.IsConnected()));
      }
   } else

   if (cmd.IsName(CmdDestroyTransport::CmdName())) {
      PortRef portref = FindPort(cmd.GetStr("PortName"));
      if (!portref.Disconnect())
        cmd_res = cmd_false;
   } else

   if (cmd.IsName(CmdCreateAny::CmdName())) {
      void* res = 0;
      FOR_EACH_FACTORY(
         res = factory->CreateAny(cmd.GetStr("ClassName"), cmd.GetStr("ObjectName"), cmd);
         if (res != 0) break;
      )
      cmd.SetPtr("ObjectPtr", res);
      cmd_res = cmd_true;
   } else

   if (cmd.IsName(CmdCreateThread::CmdName())) {
      const std::string& thrdname = cmd.GetStr(CmdCreateThread::ThrdNameArg());
      const std::string& thrdclass = cmd.GetStr("ThrdClass");
      const std::string& thrddev = cmd.GetStr("ThrdDev");

      ThreadRef thrd = DoCreateThread(thrdname, thrdclass, thrddev, cmd);

      if (thrd.null()) {
         cmd_res = cmd_false;
      } else {
         cmd.SetStr(CmdCreateThread::ThrdNameArg(), thrd.GetName());
         cmd_res = cmd_true;
      }

   } else
   if (cmd.IsName(CmdCreateMemoryPool::CmdName())) {
      cmd_res = cmd_bool(DoCreateMemoryPool(cmd));
   } else
   if (cmd.IsName(CmdCreateObject::CmdName())) {
      cmd.SetRef("Object", DoCreateObject(cmd.GetStr("ClassName"), cmd.GetStr("ObjName"), cmd));
      cmd_res = cmd_true;
   } else
   if (cmd.IsName(CmdCreateDataInput::CmdName())) {
      std::string kind = cmd.GetStr("Kind");
      DataInput* res = 0;
      FOR_EACH_FACTORY(
         res = factory->CreateDataInput(kind);
         if (res != 0) break;
      )
      cmd.SetPtr("DataInput", res);
      cmd_res = cmd_true;
   } else
   if (cmd.IsName(CmdCleanupApplication::CmdName())) {
      cmd_res = DoCleanupApplication();
   } else
   if (cmd.IsName(CmdStartModule::CmdName()) || cmd.IsName(CmdStopModule::CmdName())) {

      std::string name = cmd.GetStr(CmdModule::ModuleArg());
      dabc::WorkerRef ref;

      if (name.compare("*")==0)
         ref = app();
      else
         ref = FindModule(name);

      if (ref.Submit(cmd))
         cmd_res = cmd_postponed;
      else
         cmd_res = cmd_false;
   } else
   if (cmd.IsName(CmdDeleteModule::CmdName())) {
      ModuleRef ref = FindModule(cmd.GetStr(CmdDeleteModule::ModuleArg()));

      cmd_res = cmd_bool(!ref.null());

      DOUT2("Stop and delete module %s", ref.GetName());

      ref.Destroy();

      DOUT2("Stop and delete module done");

   } else
   if (cmd.IsName(CmdDeletePool::CmdName())) {
      FindPool(cmd.GetStr("PoolName")).Destroy();
   } else
   if (cmd.IsName("Print")) {
      dabc::Iterator iter1(GetThreadsFolder());
      Thread* thrd = 0;
      while (iter1.next_cast(thrd, false))
         DOUT1("Thrd: %s", thrd->GetName());

      dabc::Iterator iter2(this);
      Module* m = 0;
      while (iter2.next_cast(m, false))
         DOUT1("Module: %s", m->GetName());
   } else

   // these are two special commands with postponed execution
   if (cmd.IsName(CmdGetNodesState::CmdName())) {
      GetCommandChannel().Submit(cmd);

      cmd_res = cmd_postponed;
   } else
   if (cmd.IsName("Ping")) {
      DOUT2("!!! PING !!!");
      cmd_res = cmd_true;
   } else
   if (cmd.IsName("ParameterEventSubscription")) {
      Worker* worker = (Worker*) cmd.GetPtr("Worker");
      std::string mask = cmd.GetStr("Mask");
      std::string remote = cmd.GetStr("RemoteWorker");

      DOUT2("Subscription with mask %s", mask.c_str());

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
      if (fMgrStoppedTime.null()) fMgrStoppedTime.GetNow();
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

   std::string poolname = cmd.GetStr(xmlPoolName);
   if (poolname.empty()) {
      EOUT("Pool name is not specified");
      return false;
   }

   MemoryPoolRef ref = FindPool(poolname);
   if (ref.null()) {
      ref = new dabc::MemoryPool(poolname, true);

      ref()->Reconstruct(cmd);

      // TODO: make thread name for pool configurable
      ref()->AssignToThread(thread());
      ref.Start();
   }

   return !ref.null();
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
               DOUT2("Internal error - parameters event queue negative");
            return true;
         }
      }

      DOUT2("Did not find original record with receiver for parameter events");

      return true;
   }

   if (cmd.IsName(CmdStateTransition::CmdName())) {
      // manager receive reply on this command only during normal shutdown
      fAppFinished = true;
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
      DOUT0("dabc::Manager::DestroyObject %p %s", obj, obj->GetName());

   bool fire = false;

   {
      LockGuard lock(fMgrMutex);

      // analyze that object presented in some dependency lists and mark record to process it
      DependPairList::iterator iter = fDepend->begin();
      while (iter != fDepend->end()) {
         if (iter->src() == obj) {
            iter->fire = iter->fire | 1;
            if (obj->IsLogging())
               DOUT0("Find object %p as dependency source", obj);
         }

         if (iter->tgt == obj) {
            iter->fire = iter->fire | 2;
            if (obj->IsLogging())
               DOUT0("Find object %p as dependency target", obj);
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
   // destroy application if exist
   app().Destroy();

   ProcessDestroyQueue();

   Object::InspectGarbageCollector();

   return true;
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
         fprintf(stdout, "%s    ",  prefix);
         fflush(stdout);
      }

      TimeStamp finish = dabc::Now() + tmout;
      double remain;

      while ((remain = finish - dabc::Now()) > 0) {

         if (prefix) {
            fprintf(stdout, "\b\b\b%3d", (int) lrint(remain));
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


std::string dabc::Manager::GetNodeAddress(int nodeid)
{
   LockGuard lock(fMgrMutex);

   if ((nodeid<0) || (nodeid>=fNumNodes)) return std::string();

   if (fCfg==0) return std::string();

   if ((nodeid==fNodeId) && !fLocalAddress.empty()) return fLocalAddress;

   Url url(fCfg->NodeName(nodeid));
   return url.GetHostNameWithPort(defaultDabcPort);
}


std::string dabc::Manager::GetLocalAddress()
{
   LockGuard lock(fMgrMutex);
   return fLocalAddress;
}

std::string dabc::Manager::ComposeAddress(const std::string& server, const std::string& itemname)
{
   std::string res = server;
   if (res.empty()) res = GetLocalAddress();
   if (res.empty()) res = "localhost";

   if (res.find("dabc://")!=0) res = std::string("dabc://") + res;

   if (!itemname.empty()) {
      if (itemname[0]!='/') res += "/";
      res += itemname;
   }
   return res;
}

bool dabc::Manager::DecomposeAddress(const std::string& addr, bool& islocal, std::string& server, std::string& itemtname)
{

   dabc::Url url;

//   DOUT0("Url %s valid %d protocol %s host %s file %s", name, url.IsValid(), url.GetProtocol().c_str(), url.GetHostName().c_str(), url.GetFileName().c_str());

   if (!url.SetUrl(addr, false)) return false;

   if (url.GetProtocol().length()==0) {
      islocal = true;
      server.clear();
      itemtname = addr;
      return true;
   }

   if (url.GetProtocol().compare("dabc")!=0) return false;

   islocal = false;
   server = url.GetHostNameWithPort();
   itemtname = url.GetFileName();

   int nodeid = -1;
   if (server.compare(0, 4, "node")==0) {
      if (!str_to_int(server.c_str() + 4, &nodeid)) nodeid = -1;
   }

   if (nodeid>=0) server = GetNodeAddress(nodeid);

   LockGuard lock(fMgrMutex);

   if ((nodeid>=0) && (fNodeId==nodeid)) islocal = true; else
   if (server == fLocalAddress) islocal = true; else
   if (server == "localhost") islocal = true;

   if (islocal) server = fLocalAddress;

   return true;
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

dabc::Manager* dabc::Manager::fInstance = 0;
int dabc::Manager::fInstanceId = 0;

dabc::Factory* dabc::Manager::fFirstFactories[10];
int dabc::Manager::fFirstFactoriesId;

void dabc::Manager::ProcessFactory(Factory* factory)
{
   if (factory==0) return;

   DOUT2("Instantiate factory %s", factory->GetName());

   if (fInstance && (fInstanceId==MagicInstanceId)) {
      fInstance->GetFactoriesFolder(true).AddChild(factory);
      return;
   }

   // printf("Manager is not exists when factory %s is created\n", factory->GetName());

   if (fFirstFactoriesId == MagicInstanceId) {
      for (unsigned n=0;n<sizeof(fFirstFactories)/sizeof(void*); n++)
         if (fFirstFactories[n] == 0) {
            fFirstFactories[n] = factory;
            break;
         }
   } else {
      // printf("Init first factories arrary %u\n", (unsigned) (sizeof(fFirstFactories)/sizeof(void*)));
      fFirstFactoriesId = MagicInstanceId;
      fFirstFactories[0] = factory;
      for (unsigned n=1;n<sizeof(fFirstFactories)/sizeof(void*); n++) fFirstFactories[n] = 0;
   }

}

void dabc::Manager::ProcessCtrlCSignal()
{
   DOUT0("Process CTRL-C signal");

   if (fMgrStoppedTime.null()) {
      fMgrStoppedTime.GetNow();
      return;
   }

   double spent = fMgrStoppedTime.SpentTillNow();

   // TODO: make 10 second configurable
   if (spent<10.) return;

   DOUT0("Ctrl-C repeated more than after 10 sec out of main loop - force manager halt");

   HaltManager();

   DOUT0("Exit after Ctrl-C");

   exit(0);
}

void dabc::Manager::RunManagerMainLoop(double runtime)
{
   DOUT2("Enter dabc::Manager::RunManagerMainLoop");

   ThreadRef thrd = thread();
   if (thrd.null()) return;

   if (!fMgrStoppedTime.null()) {
      DOUT1("Manager stopped before entering to the mainloop - stop running");
      return;
   }

   if (runtime>0)
      DOUT0("Application mainloop will run for %3.1f s", runtime);
   else
      DOUT0("Application mainloop is now running");
   DOUT0("       Press Ctrl-C for stop");

   if (thrd.IsRealThrd()) {
      DOUT3("Manager has normal thread - just wait until application modules are stopped");
   } else {
      DOUT3("Run manager thread mainloop inside main process");

      // to be sure that timeout processing is active
      // only via timeout one should be able to stop processing of main loop
      ActivateTimeout(1.);
   }

   TimeStamp starttm = dabc::Now();

   bool appstopped = false;

   ApplicationRef appref = app();

   // we run even loop in units of 0.1 sec
   // TODO: make 0.1 sec configurable
   double period = 0.1;

   while (true) {

      if (thrd.IsRealThrd())
         dabc::Sleep(period);
      else
         thrd.RunEventLoop(period);

      if (appstopped && fAppFinished) break;

      // check if stop time was not set
      if (fMgrStoppedTime.null()) {
         if ((runtime <= 0) || !starttm.Expired(runtime)) continue;
         DOUT0("++++++++ Set stop time while runtime expired");
         fMgrStoppedTime.GetNow();
      }

      period = 0.001; // perform checks more often

      if (!appstopped) {
         appstopped = true;
         appref.Submit(Assign(dabc::CmdStateTransition(dabc::Application::stHalted())));
      }

      if (fMgrStoppedTime.Expired(10.)) break; // TODO: make 10 second configurable
   }

   DOUT2("Exit dabc::Manager::RunManagerMainLoop");
}


int inputAvailable()
{
  struct timeval tv;
  fd_set fds;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
  return (FD_ISSET(STDIN_FILENO, &fds));
}

#include <iostream>

void dabc::Manager::RunManagerCmdLoop(double runtime, const std::string& remnode)
{
   DOUT0("Enter dabc::Manager::RunManagerCmdLoop");

   ThreadRef thrd = thread();

   if (thrd.null()) return;

   if (GetCommandChannel().null()) {
      EOUT("No command channel found");
      return;
   }

   if (thrd.IsRealThrd()) {
      DOUT3("Manager has normal thread - just wait until application modules are stopped");
   } else {
      DOUT3("Run manager thread mainloop inside main process");

      // to be sure that timeout processing is active
      // only via timeout one should be able to stop processing of main loop
      ActivateTimeout(1.);
   }

   TimeStamp start = dabc::Now();

   std::string tgtnode;

   Hierarchy rem_hierarchy;

   bool first(true);

   while (true) {

      // we run even loop in units of 0.1 sec
      // TODO: make 0.1 sec configurable
      if (thrd.IsRealThrd())
         dabc::Sleep(0.001);
      else
         thrd.RunEventLoop(0.001);

      if ((runtime>0) && start.Expired(runtime)) {
         DOUT0("run time is over");
         break;
      }

      if (!fMgrStoppedTime.null()) {
         DOUT0("break command shell");
         break;
      }

      std::string str;

      if (first && !remnode.empty()) {
         first = false;
         str = std::string("connect ") + remnode;
         printf("cmd>%s\n",str.c_str());
      } else {
         first = false;
         if (inputAvailable()<=0) continue;
         printf("cmd>"); fflush(stdout);
         std::getline(std::cin, str);
      }

      if (str.empty()) continue;

      if ((str=="quit") || (str=="exit") || (str==".q") || (str=="q")) break;

      dabc::Command cmd;
      if (!cmd.ReadFromCmdString(str)) {
         EOUT("Wrong syntax %s", str.c_str());
         continue;
      }

      if (cmd.IsName("connect")) {
         std::string node = dabc::MakeNodeName(cmd.GetStr("Arg0"));

         if (node.empty()) {
            EOUT("Node name not specified correctly");
            continue;
         }

         tgtnode = node;

         dabc::Command cmd2("Ping");
         cmd2.SetReceiver(tgtnode);
         cmd2.SetTimeout(5.);

         int res = GetCommandChannel().Execute(cmd2);

         if (res == cmd_true) {
            DOUT0("Connection to %s done", tgtnode.c_str());
         } else {
            DOUT0("FAIL connection to %s", tgtnode.c_str());
            tgtnode.clear();
         }

         continue;
      }

      if (tgtnode.empty()) {
         DOUT0("Tgt node not connected, command %s not executed", cmd.GetName());
         continue;
      }

      if (cmd.IsName("close") || cmd.IsName("disconnect")) {
         cmd.SetStr("host", tgtnode);
         GetCommandChannel().Execute(cmd);
         tgtnode.clear();
         rem_hierarchy.Release();
         continue;
      }

      if (cmd.IsName("update")) {

         rem_hierarchy = dabc::GetNodeHierarchy(tgtnode);

         continue;
      }

      if (cmd.IsName("ls")) {
         if (!rem_hierarchy.null())
            DOUT0("xml = ver %u \n%s", (unsigned) rem_hierarchy.GetVersion(), rem_hierarchy.SaveToXml().c_str());
         else
            DOUT0("No hierarchy available");
         continue;
      }

      if (cmd.IsName("get")) {
         std::string path = cmd.GetStr("Arg0");
         int hlimit = cmd.GetInt("Arg1");

         std::string query;
         if (hlimit>0) query = dabc::format("history=%d",hlimit);

         CmdGetBinary cmd2(path, "hierarchy", query);
         cmd2.SetTimeout(5);

         if (GetCommandChannel().Execute(cmd2)!=cmd_true) {
            DOUT0("Fail to get item %s", path.c_str());
            continue;
         }

         dabc::Hierarchy res;
         res.Create("get");
         res.SetVersion(cmd2.GetUInt("version"));
         res.ReadFromBuffer(cmd2.GetRawData());

         DOUT0("GET:%s len:%d RES = \n%s", path.c_str(), hlimit, res.SaveToXml().c_str());

         continue;
      }


      cmd.SetReceiver(tgtnode);
      cmd.SetTimeout(5.);

      int res = GetCommandChannel().Execute(cmd);

      if (res == cmd_timedout)
         DOUT0("Command %s timeout", cmd.GetName());
      else
         DOUT0("Command %s res = %d", cmd.GetName(), res);
   }
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


// =================================== classes from ManagerRef class ==================================


dabc::ThreadRef dabc::Manager::FindThread(const std::string& name, const std::string& required_class)
{
   ThreadRef ref = GetThreadsFolder().FindChild(name.c_str());

   if (ref.null()) return ref;

   if (!required_class.empty() && !ref()->CompatibleClass(required_class)) ref.Release();

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

dabc::ThreadRef dabc::Manager::DoCreateThread(const std::string& thrdname, const std::string& thrdclass, const std::string& thrddev, Command cmd)
{
   std::string newname = thrdname;
   int basecnt = 0;

   ThreadRef thrd = FindThread(newname);

   while ((basecnt<1000) && !thrd.null()) {
      if (thrd()->CompatibleClass(thrdclass)) return thrd;

      EOUT("Thread %s of class %s exists and incompatible with %s class",
          thrdname.c_str(), thrd()->ClassName(), (thrdclass.empty() ? "---" : thrdclass.c_str()) );

      newname = dabc::format("%s_%d", thrdname.c_str(), basecnt++);
      thrd = FindThread(newname);
   }

   if (!thrd.null()) {
      EOUT("Too many incompatible threads with name %s", thrdname.c_str());
      exit(765);
   }

   DOUT3("CreateThread %s of class %s, is any %p", newname.c_str(), (thrdclass.empty() ? "---" : thrdclass.c_str()), thrd());

   FOR_EACH_FACTORY(
      thrd = factory->CreateThread(GetThreadsFolder(true), thrdclass, newname, thrddev, cmd);
      if (!thrd.null()) break;
   )

   DOUT3("CreateThread %s done %p", newname.c_str(), thrd());

   bool noraml_thread = true;
   if ((newname == MgrThrdName()) && cfg())
      noraml_thread = cfg()->NormalMainThread();

   DOUT3("Starting thread %s as normal %s refcnt %d", thrd.GetName(), DBOOL(noraml_thread), thrd.NumReferences());

   if (!thrd.null())
      if (!thrd()->Start(10, noraml_thread)) {
         EOUT("Thread %s cannot be started!!!", newname.c_str());
         thrd.Destroy();
      }

   DOUT3("Create thread %s of class %s thrd %p refcnt %d done", newname.c_str(), (thrdclass.empty() ? "---" : thrdclass.c_str()), thrd(), thrd.NumReferences());

   return thrd;
}


// ========================================== ManagerRef methods ================================


bool dabc::ManagerRef::CreateApplication(const std::string& classname, const std::string& appthrd)
{
   return Execute(CmdCreateApplication(classname, appthrd));
}


dabc::ModuleRef dabc::ManagerRef::CreateModule(const std::string& classname, const std::string& modulename, const std::string& thrdname)
{
   CmdCreateModule cmd(classname, modulename, thrdname);

   return Execute(cmd) ? FindModule(modulename) : dabc::ModuleRef();
}

dabc::ThreadRef dabc::ManagerRef::CreateThread(const std::string& thrdname, const std::string& classname, const std::string& devname)
{
   CmdCreateThread cmd(thrdname,  classname, devname);

   return Execute(cmd) == cmd_true ? FindThread(cmd.GetThrdName()) : dabc::ThreadRef();
}

dabc::ThreadRef dabc::ManagerRef::CurrentThread()
{
   return GetObject() ? GetObject()->CurrentThread() : dabc::ThreadRef();
}


bool dabc::ManagerRef::CreateDevice(const std::string& classname, const std::string& devname)
{
   return Execute(CmdCreateDevice(classname, devname));
}


bool dabc::ManagerRef::DeleteDevice(const std::string& devname)
{
   return Execute(CmdDestroyDevice(devname));
}


dabc::WorkerRef dabc::ManagerRef::FindDevice(const std::string& name)
{
   return GetObject() ? GetObject()->FindDevice(name) : dabc::WorkerRef();
}

dabc::ModuleRef dabc::ManagerRef::FindModule(const std::string& name)
{
   return GetObject() ? GetObject()->FindModule(name) : dabc::ModuleRef();
}

void dabc::ManagerRef::StartModule(const std::string& modulename)
{
   Execute(dabc::CmdStartModule(modulename));
}

void dabc::ManagerRef::StopModule(const std::string& modulename)
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

bool dabc::ManagerRef::DeleteModule(const std::string& name)
{
   return Execute(CmdDeleteModule(name));
}

bool dabc::ManagerRef::DeletePool(const std::string& name)
{
   return Execute(CmdDeletePool(name));
}


dabc::Reference dabc::ManagerRef::FindItem(const std::string& name)
{
   return GetObject() ? GetObject()->FindItem(name) : Reference();
}

dabc::Reference dabc::ManagerRef::FindPort(const std::string& portname)
{
   return GetObject() ? GetObject()->FindPort(portname) : Reference();
}

dabc::Reference dabc::ManagerRef::FindPool(const std::string& name)
{
   return GetObject() ? GetObject()->FindPool(name) : Reference();
}

dabc::Parameter dabc::ManagerRef::FindPar(const std::string& parname)
{
   return GetObject() ? GetObject()->FindItem(parname) : Reference();
}

dabc::ApplicationRef dabc::ManagerRef::app()
{
   return GetObject() ? GetObject()->app() : dabc::ApplicationRef();
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

bool dabc::ManagerRef::ParameterEventSubscription(Worker* ptr, bool subscribe, const std::string& mask, bool onlychangeevent)
{
   if (ptr == 0) return false;

   // TODO: by the subscription to remote node first register receiver on local node and
   //       only then submit registration to remote.
   //       One should avoid multiple parallel subscription to remote node

   std::string server, itemname;
   bool islocal(true);

   if (!DecomposeAddress(mask, islocal, server, itemname)) {
      EOUT("Wrong parameter mask %s", mask.c_str());
      return false;
   }

   Command cmd("ParameterEventSubscription");

   cmd.SetBool("IsSubscribe", subscribe);
   cmd.SetStr("Mask", mask);
   cmd.SetBool("OnlyChange", onlychangeevent);

   if (islocal) {
      // this is registration for local parameters
      cmd.SetPtr("Worker", ptr);
      return Execute(cmd);
   }

   cmd.SetStr("RemoteWorker", ComposeAddress("", ptr->ItemName()));
   cmd.SetReceiver(ComposeAddress(server));

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
   if (GetObject()==0) return dabc::ConnectionRequest();

   PortRef port1 = FindPort(port1name);
   PortRef port2 = FindPort(port2name);

   if (!port1.null() && !port2.null()) {
      // make local connection immediately
      dabc::LocalTransport::ConnectPorts(port1, port2);
      // connect also bind ports (if exists)
      dabc::LocalTransport::ConnectPorts(port2.GetBindPort(), port1.GetBindPort());
      return dabc::ConnectionRequest();
   }

   if (IsLocalItem(port1name) && port1.null()) {
      EOUT("Did not found port %s", port1name.c_str());
      return dabc::ConnectionRequest();
   }

   if (IsLocalItem(port2name) && port2.null()) {
      EOUT("Did not found port %s", port2name.c_str());
      return dabc::ConnectionRequest();
   }

   if (port1.null() && port2.null()) return dabc::ConnectionRequest();

   if (GetObject()->GetCommandChannel().null()) {
      EOUT("Not possible to establish remote connection without command channel");
      return dabc::ConnectionRequest();
   }

   ModuleRef m = FindModule(Manager::ConnMgrName());

   if (m.null())
      CreateModule("dabc::ConnectionManager", Manager::ConnMgrName(), Manager::MgrThrdName());

   DOUT2("Connect ports %s %p %s %p", port1name.c_str(), port1(), port2name.c_str(), port2());

   dabc::ConnectionRequest req;

   if (!port1.null())
      req = port1.MakeConnReq(port2name, true);

   if (!port2.null())
      req = port2.MakeConnReq(port1name, false);

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
   PortRef port = FindPort(portname);

   if (port.null()) return false;

   return Execute(CmdCreateTransport(portname, transportkind, thrdname));
}

void* dabc::ManagerRef::CreateAny(const std::string& classname, const std::string& objname)
{
   CmdCreateAny cmd;
   cmd.SetStr("ClassName", classname);
   cmd.SetStr("ObjectName", objname);

   if (Execute(cmd) != cmd_true) return 0;

   return cmd.GetPtr("ObjectPtr");
}


void dabc::ManagerRef::StopApplication()
{
   // Manager will be stopped regularly if it is in running

   Submit(dabc::Command("StopManagerMainLoop"));

//   if (GetObject())
//      if (GetObject()->fMgrStoppedTime.null())
//         GetObject()->fMgrStoppedTime = dabc::Now();
}

bool dabc::ManagerRef::CreateMemoryPool(const std::string& poolname,
                                        unsigned buffersize,
                                        unsigned numbuffers)
{

   CmdCreateMemoryPool cmd(poolname);

   cmd.SetMem(buffersize, numbuffers);

   return Execute(cmd);
}

dabc::Reference dabc::ManagerRef::CreateObject(const std::string& classname, const std::string& objname)
{
   CmdCreateObject cmd(classname, objname);

   if (!Execute(cmd)) return dabc::Reference();

   return cmd.GetRef("Object");
}

dabc::DataInput* dabc::ManagerRef::CreateDataInput(const std::string& kind)
{
   CmdCreateDataInput cmd;
   cmd.SetStr("Kind", kind);
   if (!Execute(cmd)) return 0;

   return (dabc::DataInput*) cmd.GetPtr("DataInput");
}

void dabc::ManagerRef::Sleep(double tmout, const char* prefix)
{
   if (GetObject())
      GetObject()->Sleep(tmout, prefix);
   else
      dabc::Sleep(tmout);
}

bool dabc::ManagerRef::CreateControl(bool withserver, int serv_port, bool allow_clients)
{
   if (null()) return false;

   WorkerRef ref = GetCommandChannel();
   if (!ref.null()) return true;

   dabc::CmdCreateObject cmd("SocketCommandChannel", dabc::Manager::CmdChlName());
   cmd.SetBool("WithServer", withserver);
   cmd.SetBool("ClientsAllowed", allow_clients);
   if (withserver) {
      int port = 0;
      if (GetObject()->cfg()) port = GetObject()->cfg()->MgrPort();
      if (port<=0) port = serv_port;
      if (port<=0) port = defaultDabcPort;
      cmd.SetInt("ServerPort", port);
   }

   ref = GetObject()->DoCreateObject("SocketCommandChannel", dabc::Manager::CmdChlName(), cmd);

   if (ref.null()) return false;

   GetObject()->fLocalAddress = cmd.GetStr("localaddr");

   DOUT0("Set Local addr %s", GetObject()->fLocalAddress.c_str());

   ref.MakeThreadForWorker("CmdThrd");

   return true;
}


bool dabc::ManagerRef::CreatePublisher()
{
   PublisherRef ref = FindItem(dabc::Publisher::DfltName());
   if (!ref.null()) return true;

   ref = new dabc::Publisher(dabc::Publisher::DfltName());
   ref.MakeThreadForWorker("PublisherThrd");

   return true;
}
