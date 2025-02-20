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

#include "dabc/Worker.h"

#include <cstdlib>
#include <cmath>

#include "dabc/Manager.h"
#include "dabc/Publisher.h"
#include "dabc/HierarchyStore.h"
#include "dabc/Url.h"
#include "dabc/defines.h"

dabc::WorkerAddon::WorkerAddon(const std::string &name) :
   Object(nullptr, name),
   fWorker()
{
   SetFlag(flAutoDestroy, true);
}

dabc::WorkerAddon::~WorkerAddon()
{
}

void dabc::WorkerAddon::ObjectCleanup()
{
   fWorker.Release();
}

void dabc::WorkerAddon::DeleteWorker()
{
   if (fWorker.null()) DeleteThis();
                 else fWorker.Destroy();
}

void dabc::WorkerAddon::DeleteAddonItself()
{
   if (fWorker.null()) DeleteThis();
                else SubmitWorkerCmd(dabc::Command("DeleteAddon"));
}

void dabc::WorkerAddon::FireWorkerEvent(unsigned evid)
{
   dabc::Worker* wrk = (dabc::Worker*) fWorker();
   if (wrk) wrk->FireEvent(evid);
}

bool dabc::WorkerAddon::ActivateTimeout(double tmout_sec)
{
   dabc::Worker* wrk = (dabc::Worker*) fWorker();

   if (!wrk) return false;

   LockGuard lock(wrk->fThreadMutex);

   return wrk->fThread._ActivateAddonTimeout(wrk->fWorkerId, wrk->WorkerPriority(), tmout_sec);
}

void dabc::WorkerAddon::SubmitWorkerCmd(Command cmd)
{
   dabc::Worker* wrk = (dabc::Worker*) fWorker();

   if (wrk) wrk->Submit(cmd);
}

// ================================================================================


dabc::Worker::Worker(Reference parent, const std::string &name) :
   Object(parent, name),
   fThread(),
   fAddon(),
   fPublisher(),
   fWorkerId(0),
   fWorkerPriority(-1), // minimum priority per default

   fThreadMutex(nullptr),
   fWorkerActive(false),
   fWorkerFiredEvents(0),

   fWorkerCommands(CommandsQueue::kindNone),

   fWorkerCommandsLevel(0),
   fWorkerHierarchy(),
   fWorkerCfgId(-1)
{
   SetFlag(flHasThread, false);

#ifdef DABC_EXTRA_CHECKS
   DebugObject("Worker", this, 10);
#endif
}


dabc::Worker::Worker(const ConstructorPair &pair) :
   Object(pair),
   fThread(),
   fAddon(),
   fWorkerId(0),
   fWorkerPriority(-1), // minimum priority per default

   fThreadMutex(nullptr),
   fWorkerActive(false),
   fWorkerFiredEvents(0),

   fWorkerCommands(CommandsQueue::kindNone),

   fWorkerCommandsLevel(0),
   fWorkerHierarchy(),
   fWorkerCfgId(-1)
{
   SetFlag(flHasThread, false);

#ifdef DABC_EXTRA_CHECKS
   DebugObject("Worker", this, 10);
#endif
}

dabc::Worker::~Worker()
{
   DOUT3("~Worker %p %d thrd:%p 000 ", this, fWorkerId, fThread());

   CancelCommands();

   DOUT3("~Worker %p %d thrd:%p 111", this, fWorkerId, fThread());

   // just to ensure that reference is gone
   ClearThreadRef();

   DOUT3("~Worker %p %d thrd:%p done", this, fWorkerId, fThread());

   #ifdef DABC_EXTRA_CHECKS
   DebugObject("Worker", this, -10);
   #endif
}

bool dabc::Worker::HasThread() const
{
   LockGuard lock(fThreadMutex);

   return !fThread.null();
}

dabc::ThreadRef dabc::Worker::thread()
{

   dabc::ThreadRef res;

   {
      // this is lock over thread main mutex,
      // if we can get it, we also can increment ref counter directly by the thread
      LockGuard lock(fThreadMutex);

      // we can acquire new reference without additional lock of the mutex
      fThread._AcquireThreadRef(res);
   }

   return res;
}


bool dabc::Worker::IsOwnThread() const
{
   LockGuard lock(fThreadMutex);

   if (fThread.null()) return false;

   return fThread.IsItself();
}


std::string dabc::Worker::ThreadName() const
{
   LockGuard lock(fThreadMutex);

   return std::string(fThread() ? fThread()->GetName() : "");
}


bool dabc::Worker::DestroyByOwnThread()
{
   // method can be called from any thread therefore we should first ensure that
   // correct reference will exists on the thread
   // Once true is returned thread guarantees that DestroyCalledFromOwnThread() method
   // will be called from thread context
   // One should also remember that reference counter is granted to thread

   ThreadRef thrd = thread();

   if (!thrd())
      return false;

   if (IsLogging())
      DOUT0("Worker %p %s ask thread to destroy it ", this, GetName());

   return thrd()->InvokeWorkerDestroy(this);
}


void dabc::Worker::ClearThreadRef()
{
   ThreadRef ref;

   {
      LockGuard lock(fThreadMutex);
      ref << fThread;

      fThreadMutex = nullptr;
      fWorkerId = 0;
      fWorkerActive = false;
//      fWorkerFiredEvents = 0;
   }

   if (IsLogging())
      DOUT0("Worker %s releases thread reference %p", GetName(), ref());

   ref.Release();

   dabc::LockGuard lock(fObjectMutex);

   SetFlag(Object::flHasThread, false);

}

void dabc::Worker::ObjectCleanup()
{
   // TODO: that is correct sequence - first delete child, than clean ourself  (current) or vice-versa

   DOUT4("START worker %s class %s cleanup refcnt = %d thrd %s publ %p publthrd %s", GetName(), ClassName(), fObjectRefCnt, thread().GetName(), fPublisher(), WorkerRef(fPublisher).thread().GetName());

   CleanupPublisher(false);

   // we do standard object cleanup - remove all our childs and remove ourself from parent
   dabc::Object::ObjectCleanup();

   // from this moment on no new commands/events
   DettachFromThread();

   // now process old commands
   if (fWorkerCommandsLevel>0) {
      EOUT("We are in real trouble - when Worker %s %p executes command, cleanup is called", GetName(), this);
      exit(076);
   }

   // process all postponed and submitted commands
   do {
      Command cmd = fWorkerCommands.PopWithKind(CommandsQueue::kindPostponed);
      if (cmd.null()) cmd = fWorkerCommands.PopWithKind(CommandsQueue::kindSubmit);
      if (cmd.null()) break;
      ProcessCommand(cmd);
   } while (true);

   // here we cancel all commands we submit or get as replied
   CancelCommands();

   // here addon must be destroyed or at least cross-reference removed
   // DOUT0("Worker:%s Destroy addon:%p in ObjectCleanup", GetName(), fAddon());
   fAddon.Release();

   DOUT4("DID worker %s class %s cleanup refcnt = %d", GetName(), ClassName(), fObjectRefCnt);
}


void dabc::Worker::AssignAddon(WorkerAddon* addon)
{
   if (!fAddon.null()) {
      // first remove addon from thread itself

      if (!fThread.null() && (fWorkerId>0))
         fThread()->WorkerAddonChanged(this, false);

      // clean worker pointer
      fAddon()->fWorker.Release();

      // release addon - it should be destroyed as soon as possible
      fAddon.Release();
   }

   fAddon = addon;
   if (addon) addon->fWorker = this;

   if (!fThread.null() && (fWorkerId>0) && addon) {
      fThread()->WorkerAddonChanged(this, true);
      addon->OnThreadAssigned();
   }
}


bool dabc::Worker::MakeThreadForWorker(const std::string &thrdname)
{
   std::string newname = thrdname;

   if (newname.empty()) {
      DOUT2("Thread name not specified - generate default, for a moment - processor name");
      if (GetName()) newname = GetName();
   }

   if (newname.empty()) {
      EOUT("Still no thread name - used name Thread");
      newname = "Thread";
   }

   ThreadRef thrd = dabc::mgr.CreateThread(newname, RequiredThrdClass());

   return thrd.null() ? false : AssignToThread(thrd);
}



// FIXME: old code, need revising


bool dabc::Worker::AssignToThread(ThreadRef thrd, bool sync)
{
   if (HasThread()) {
      EOUT("Worker %s already has assigned thred %s", GetName(), ThreadName().c_str());
      return false;
   }

   if (thrd.null()) {
      EOUT("Thread is not specified");
      return false;
   }

   std::string thrdcl = RequiredThrdClass();

   if (thrdcl.length()>0)
     if (thrdcl.compare(thrd.ClassName()) != 0) {
        EOUT("Processor requires class %s than thread %s of class %s" , thrdcl.c_str(), thrd.GetName(), thrd.ClassName());
        return false;
     }

   Reference ref(this);
   if (!ref()) {
      EOUT("Cannot obtain reference on itself");
      return false;
   }

   // this indicates that we are not in the thread, any events/commands will be rejected
   fWorkerId = 0;
   fWorkerActive = false;

   fWorkerFiredEvents = 0;

   fThread = thrd; // we copy reference, no extra locks required

   fThreadMutex = fThread()->ThreadMutex();

   {
      dabc::LockGuard lock(fObjectMutex);
      SetFlag(Object::flHasThread, true);
   }

   DOUT2("Assign worker %s to thread sync = %s", GetName(), DBOOL(sync));

   return fThread()->AddWorker(ref, sync);
}

bool dabc::Worker::DettachFromThread()
{
   ThreadRef thrd = thread();

   if (thrd.null()) return true;

   bool res = thrd()->HaltWorker(this);

   if (res) ClearThreadRef();

   return res;
}

bool dabc::Worker::ActivateTimeout(double tmout_sec)
{
   LockGuard lock(fThreadMutex);

   return fThread._ActivateWorkerTimeout(fWorkerId, WorkerPriority(), tmout_sec);
}

void dabc::Worker::ProcessCoreEvent(EventId evnt)
{
   DOUT3("Processor %p %u thrd %p CoreEvent %u", this, fWorkerId, fThread(), evnt.GetCode());

   switch (evnt.GetCode()) {
      case evntSubmitCommand: {

         DOUT4("Process evntSubmitCommand proc %p %u thrd %p arg %u", this, fWorkerId, fThread(), evnt.GetArg());

         Command cmd;
         {
            LockGuard lock(fThreadMutex);
            cmd = fWorkerCommands.PopWithId(evnt.GetArg());
         }

         DOUT4("Thread:%p Worker: %s Command process %s lvl %d", fThread(), GetName(), cmd.GetName(), fWorkerCommandsLevel);

         ProcessCommand(cmd);

         // check if we have postponed commands
         while(fWorkerCommandsLevel == 0) {
            {
               LockGuard lock(fThreadMutex);
               cmd = fWorkerCommands.PopWithKind(CommandsQueue::kindPostponed);
            }
            if (cmd.null()) break;
            ProcessCommand(cmd);
         }

         DOUT4("Process evntSubmitCommand done proc %p", this);

         break;
      }
      case evntReplyCommand: {

//         DOUT0("Process evntReplyCommand arg %u", evnt.GetArg());

         dabc::Command cmd;
         {
            LockGuard lock(fThreadMutex);
            cmd = fWorkerCommands.PopWithId(evnt.GetArg());
         }

         if (cmd.null())
            EOUT("evntReplyCommand: no command with specified id %u", evnt.GetArg());
         else {
            if (ReplyCommand(cmd)) cmd.Reply();
         }

         break;
      }

      default:
         EOUT("Core event %u arg:%u not processed", evnt.GetCode(), evnt.GetArg());
   }
}

int dabc::Worker::ProcessCommand(Command cmd)
{
   if (cmd.null() || !IsNormalState()) return cmd_false;

   DOUT3("ProcessCommand cmd %s lvl %d isync %s", cmd.GetName(), fWorkerCommandsLevel, DBOOL(cmd.IsLastCallerSync()));

   if (cmd.IsCanceled()) {
      cmd.Reply(cmd_canceled);
      return cmd_canceled;
   }

   // when other command is in processing state,
   // and this cmd not need to be synchronous,
   // one can process such command later
   // synchronous command must be processed immediately

   if ((fWorkerCommandsLevel > 0) && !cmd.IsLastCallerSync()) {
      LockGuard lock(fThreadMutex);
      fWorkerCommands.Push(cmd, CommandsQueue::kindPostponed);
      return cmd_postponed;
   }

   IntGuard guard(fWorkerCommandsLevel);

   if (IsLogging())
     DOUT0("Worker %p %s process command %s", this, GetName(), cmd.GetName());

   int cmd_res = PreviewCommand(cmd);

   if (IsLogging())
     DOUT0("Worker %p %s did preview command %s ignored %s", this, GetName(), cmd.GetName(), DBOOL(cmd_res == cmd_ignore));

   if (cmd_res == cmd_ignore)
      cmd_res = ExecuteCommand(cmd);

   if (cmd_res == cmd_ignore) {
      EOUT("Command ignored %s", cmd.GetName());
      cmd_res = cmd_false;
   }

   // FIXME: is it only postponed command is not completed ???
   bool completed = cmd_res!=cmd_postponed;

   DOUT3("Thrd: %p Worker: %s ProcessCommand cmd %s lvl %d done", fThread(), GetName(), cmd.GetName(), fWorkerCommandsLevel);

   if (completed) cmd.Reply(cmd_res);

   return cmd_res;
}

void dabc::Worker::ProcessEvent(const EventId& evnt)
{
   EOUT("Event %u arg:%u not processed req:%s", evnt.GetCode(), evnt.GetArg(),
         RequiredThrdClass().c_str());
}

bool dabc::Worker::ActivateMainLoop()
{
   // call should be performed from the thread context
   if (fThread())
      return fThread()->SetExplicitLoop(this);
   else
      return false;
}

// all about parameters handling

dabc::Parameter dabc::Worker::Par(const std::string &name) const
{
   return FindChildRef(name.c_str());
}

dabc::RecordField dabc::Worker::Cfg(const std::string &name, Command cmd) const
{

   DOUT2("Worker %s Cfg %s", ItemName().c_str(), name.c_str());

   // first check in the command
   if (cmd.HasField(name)) return cmd.GetField(name);

   DOUT3("Check Cfg %s in own parameters", name.c_str());

   // second - as parameter
   dabc::RecordField res = Par(name).Value();
   if (!res.null()) return res;

   DOUT3("Check Cfg %s in xml file", name.c_str());

   // third - in xml file
   ConfigIO io(dabc::mgr()->cfg(), fWorkerCfgId);
   if (io.ReadRecordField((Object*) this, name, &res, nullptr)) {
      DOUT2("Worker %s Cfg %s xml %s", ItemName().c_str(), name.c_str(), res.AsStr().c_str());
      return res;
   }

   DOUT3("Check Cfg %s in parent parameters", name.c_str());

   // forth - in parameters of all parents
   auto prnt = GetParent();
   while (prnt) {
      res = WorkerRef(prnt).Par(name).Value();
      if (!res.null()) return res;

      prnt = prnt->GetParent();
   }

   return res;
}

dabc::Parameter dabc::Worker::CreatePar(const std::string &name, const std::string &kind)
{
   Parameter par = Par(name);
   if (par.null()) {

      bool hidden = (kind == "cmddef");

      auto cont = new ParameterContainer(this, name, kind, hidden);

      ConfigIO io(dabc::mgr()->cfg());

      io.ReadRecordField(this, name, nullptr, &(cont->Fields()));

      par = cont;

      cont->FireParEvent(parCreated);
   }
   return par;
}

void dabc::Worker::SetParValue(const std::string &name, const RecordField &v)
{
   Parameter par = Par(name);
   if (par.null()) return;

   par.SetValue(v);

   Hierarchy chld = fWorkerHierarchy.FindChild(name.c_str());
   if (!chld.null()) {
      par.ScanParamFields(&chld()->Fields());
      fWorkerHierarchy.MarkChangedItems();
   }
}

bool dabc::Worker::DestroyPar(const std::string &name)
{
   Parameter par = Par(name);

   if (par.null()) return false;

   par.Destroy();

   return true;
}

dabc::CommandDefinition dabc::Worker::CreateCmdDef(const std::string &name)
{
   return CreatePar(name, "cmddef");
}

bool dabc::Worker::Find(ConfigIO &cfg)
{
   DOUT3("Worker::Find %p name = %s parent %p", this, GetName(), GetParent());

   if (!GetParent()) return false;

   while (cfg.FindItem(ClassName()))
      if (cfg.CheckAttr(xmlNameAttr, GetName())) return true;

   return false;
}

void dabc::Worker::WorkerParameterChanged(bool force_call, ParameterContainer *par, const std::string &value)
{
   if (force_call || IsOwnThread()) {

      unsigned mask = par->ConfirmFromWorker();

      if (mask & 1)
         ProcessParameterRecording(par);

      if (mask & 2)
         ProcessParameterEvent(CmdParameterEvent(par->GetName(), value, parModified));
   } else {
      CmdParameterEvent evnt(par->GetName(), value, parModified);
      evnt.SetBool("local", true);
      Submit(evnt);
   }
}

void dabc::Worker::ProcessParameterRecording(ParameterContainer *par)
{
   if (fWorkerHierarchy.null()) return;

   LockGuard guard(fWorkerHierarchy.GetHMutex());

   Hierarchy chld = fWorkerHierarchy.FindChild(par->GetName());

   if (!chld.null()) par->BuildFieldsMap(&chld()->Fields());
   fWorkerHierarchy.MarkChangedItems();
}

int dabc::Worker::PreviewCommand(Command cmd)
{
   int cmd_res = cmd_ignore;

   if (cmd.IsName(CmdSetParameter::CmdName())) {

      Parameter par = Par(cmd.GetStr(CmdSetParameter::ParName()));

      cmd_res = par.ExecuteChange(cmd);

   } else

   if (cmd.IsName(CmdParameterEvent::CmdName())) {

      ParameterEvent evnt(cmd);

      if (cmd.GetBool("local")) {
         Parameter par = Par(evnt.ParName());
         if (par.null()) {
            EOUT("FAIL to find local parameter %s", evnt.ParName().c_str());
            cmd_res = cmd_false;
         } else {
            WorkerParameterChanged(true, par(), evnt.ParValue());
            cmd_res = cmd_true;
         }
      } else {
         ProcessParameterEvent(evnt);
         cmd_res = cmd_true;
      }

   } else

   if (cmd.IsName("ObjectDestroyed")) {
      // process cleanup in worker thread
      ObjectDestroyed((Object*)cmd.GetPtr("#Object"));
      cmd_res = cmd_true;
   } else

   if (cmd.IsName("DestroyParameter")) {
      cmd_res = cmd_bool(DestroyPar(cmd.GetStr("ParName")));
   } else

   if (cmd.IsName("SyncWorker")) {
      // this is just dummy command, which is submitted with minimum priority
      cmd_res = cmd_true;
   } else

   if (cmd.IsName("DeleteAddon")) {
     // this is way to delete addon
     AssignAddon(nullptr);
     cmd_res = cmd_true;
   } else

   if (cmd.IsName(CmdPublisher::CmdName())) {
      dabc::Hierarchy h = (HierarchyContainer*) cmd.GetPtr("hierarchy");
      HierarchyStore* store = (HierarchyStore*) cmd.GetPtr("store");
      unsigned version = cmd.GetUInt("version");

      if (!h.null()) {

         BeforeHierarchyScan(h);

         LockGuard lock(h.GetHMutex());

         Buffer diff = h.SaveToBuffer(dabc::stream_NamesList, version);
         cmd.SetRawData(diff);

         cmd.SetUInt("version", h.GetVersion());

         if (store) store->ExtractData(h);

         cmd_res = cmd_true;
      }

      if (store) store->WriteExtractedData();

   } else
   if (cmd.IsName(dabc::CmdGetBinary::CmdName())) {

      dabc::Hierarchy h = (HierarchyContainer*) cmd.GetPtr("hierarchy");
      if (h.null()) return cmd_ignore;

      std::string item = cmd.GetStr("subitem");
      std::string fullname = cmd.GetStr("Item");
      std::string binkind = cmd.GetStr("Kind");
      std::string query = cmd.GetStr("Query");

      DOUT4("Worker %s Process CmdGetBinary item:%s full:%s kind %s", GetName(), item.c_str(), fullname.c_str(), binkind.c_str());

      std::string surl = "getitem";
      if (query.length()>0) { surl.append("?"); surl.append(query); }

      dabc::Url url(surl);
      if (!url.IsValid()) {
         EOUT("Cannot decode query url %s", query.c_str());
         return cmd_ignore;
      }

      unsigned hlimit = 0;
      uint64_t version = 0;
      int compact = 0;
      bool with_childs = false;

      if (url.HasOption("history")) {
         int hist = url.GetOptionInt("history", 0);
         if (hist > 0) hlimit = (unsigned) hist;
      }
      if (url.HasOption("version")) {
         int v = url.GetOptionInt("version", 0);
         if (v > 0) version = (unsigned) v;
      }
      if (url.HasOption("compact"))
         compact = url.GetOptionInt("compact", 3);
      if (url.HasOption("childs"))
         with_childs = true;

      LockGuard lock(h.GetHMutex());

      dabc::Hierarchy sub = h.GetFolder(item);

      if (binkind=="hierarchy") {
         if (sub.null()) return cmd_ignore;
         // we record only fields, everything else is ignored - even name of entry is not stored
         Buffer raw = sub.SaveToBuffer(with_childs ? dabc::stream_Full : dabc::stream_Value, version, hlimit);
         if (raw.null()) return cmd_ignore;

         cmd.SetRawData(raw);
         cmd.SetUInt("version", sub.GetVersion());
         cmd_res = cmd_true;
      } else
      if (binkind=="cmd.json") {
         std::string cmdname = url.GetOptionStr("command");
         // make protection - append prefix to exclude misuse of interface
         if (!cmdname.empty()) cmdname = std::string("HCMD_") + cmdname; else
         if (sub.HasField("_numargs")) cmdname = "ROOTCMD";
         if (cmdname.empty()) return cmd_ignore;
         dabc::Command subcmd(cmdname);
         subcmd.SetRef("item", sub);
         subcmd.SetStr("query", query);

         // we misuse command interface for executing actions together with subitem
         // one could have problems if any
         if (ExecuteCommand(subcmd)!=cmd_true) return cmd_ignore;

         Buffer raw = subcmd.GetRawData();
         cmd.SetRawData(raw);

         cmd_res = cmd_true;
      } else
      if ((binkind=="get.json") || (binkind=="value.json") || (binkind=="item.json") || (binkind=="get.xml") || (binkind=="dabc.json") || (binkind=="dabc.xml")) {
         std::string field = "";
         if (binkind=="value.json") field = "value";
                               else field = url.GetOptionStr("field", "");

         if (sub.null() && field.empty()) {
            size_t separ = item.find_last_of('/');
            if ((separ != std::string::npos) && (separ>0) && (separ < item.length()-1)) {
               field = item.substr(separ+1);
               item.resize(separ);
               sub = h.GetFolder(item);
            }
         }

         if (sub.null()) return cmd_ignore;

         // DOUT0("Request JSON for item %s field %s compact %d", item.c_str(), field.c_str(), compact);

         bool isxml = binkind.find(".xml")!=std::string::npos;

         std::string replybuf;

         dabc::NumericLocale loc; // ensure correct locale for conversion

         if (field.empty()) {
            if (compact < 0)
               compact = 0;
            else if (compact > storemask_Compact)
               compact = storemask_Compact;
            unsigned mask = compact;
            if (hlimit>0) mask |= storemask_NoChilds | dabc::storemask_History | storemask_TopVersion;
            if (isxml) mask |= dabc::storemask_AsXML;

            dabc::HStore store(mask);
            store.SetLimits(version, hlimit);
            if (sub.SaveTo(store))
               replybuf = store.GetResult();

         } else {
            if (!sub.HasField(field)) return cmd_ignore;

            replybuf = sub.GetField(field).AsJson();
         }

         cmd.SetStrRawData(replybuf);

         cmd_res = cmd_true;
      }
   }

   return cmd_res;
}


int dabc::Worker::ExecuteCommand(Command)
{
   return cmd_false;
}

bool dabc::Worker::ReplyCommand(Command)
{
   return true;
}


/** This method should be used to execute command synchronously from processor itself.
 *  Method let thread event loop running.
 */

/*
bool dabc::Worker::ExecuteIn(dabc::Worker *dest, dabc::Command cmd)
{
   // this is pointer of thread from which command is called
   ThreadRef thrd = thread();

   // we must be sure that call is done from thread itself - otherwise it is wrong
   if (!thrd.null()) return false;

   return thrd()->RunCommandInTheThread(this, dest, cmd) > 0;
}
*/

bool dabc::Worker::Execute(Command cmd, double tmout)
{
   if (cmd.null()) return false;

   if (tmout > 0.) cmd.SetTimeout(tmout);

   ThreadRef thrd;
   bool exe_direct = false;

   if (IsLogging())
      DOUT0("Worker %p %s Executes command %s", this, GetName(), cmd.GetName());

   {
      LockGuard lock(fThreadMutex);

      if (fThread.null() || (fWorkerId == 0)) {

         // command execution possible without thread,
         // but only manager allows to do it without warnings
         if (this != dabc::mgr())
            EOUT("warning: Cannot execute command %s without working thread, execute directly", cmd.GetName());
         exe_direct = true;
      } else
      if (fThread.IsItself()) {
         // DOUT0("Mutex = %p thrdmutex %p locked %s", fThreadMutex, fThread()->ThreadMutex(), DBOOL(fThreadMutex->IsLocked()));
         fThread._AcquireThreadRef(thrd);
      }
   }

   if (exe_direct)
      return ProcessCommand(cmd) > 0;

   // if command was called not from the same thread,
   // find handle of caller thread to let run its event loop
   if (thrd.null())
      thrd = dabc::mgr.CurrentThread();

   if (thrd()) {
      return thrd()->RunCommandInTheThread(nullptr, this, cmd) > 0;
      //if (res!=cmd_ignore) return res>0;
      //return ProcessCommand(cmd) > 0;
   }

   // if there is no Thread with such id (most probably, some user-managed thrd)
   // than we create fake object only to handle commands and events,
   // but not really take over thread mainloop
   // This object is not seen from the manager, therefore many such instances may exist in parallel

   DOUT3("**** We really creating dummy thread for cmd %s worker:%p %s", cmd.GetName(), this, GetName());

   Thread curr(Reference(), "Current");

   curr.Start(0, false);

   return curr.RunCommandInTheThread(nullptr, this, cmd) > 0;
}

dabc::Command dabc::Worker::Assign(dabc::Command cmd)
{
   if (cmd.null()) return cmd;

   {
      LockGuard lock(fThreadMutex);

      if (!fThread()) {
         EOUT("Worker: %s - command %s cannot be assigned without thread", GetName(), cmd.GetName());
         return Command();
      }

      fWorkerCommands.Push(cmd, CommandsQueue::kindAssign);
   }

   cmd.AddCaller(this, nullptr);

   return cmd;
}

bool dabc::Worker::CanSubmitCommand() const
{
   LockGuard lock(fThreadMutex);

   return !fThread.null() && fWorkerActive;
}

bool dabc::Worker::Submit(dabc::Command cmd)
{
   if (cmd.null()) return false;

   int priority = cmd.GetPriority();

   {
      LockGuard lock(fThreadMutex);

      // we can submit commands when worker is active (does not tries to halt)
      if (!fThread.null())
         if (fWorkerActive || (priority == priorityMagic)) {

            if (priority == priorityMagic) priority = fWorkerPriority; else
            if (priority == priorityDefault) priority = 0; else
            if (priority == priorityMinimum) priority = -1;

            uint32_t arg = fWorkerCommands.Push(cmd, CommandsQueue::kindSubmit);

            DOUT5("Fire event for worker %d with priority %d", fWorkerId, priority);

            fThread()->_Fire(EventId(evntSubmitCommand, fWorkerId, arg), priority);

            fWorkerFiredEvents++;

            return true;
      }
   }

   DOUT0("Worker:%s Command %s cannot be submitted - thread is not assigned", GetName(), cmd.GetName());

   cmd.ReplyFalse();

   return false;
}

bool dabc::Worker::GetCommandReply(dabc::Command& cmd, bool* exe_ready)
{
   if (cmd.null()) return false;

   LockGuard lock(fThreadMutex);

   DOUT3("GetCommandReply %s exe_ready %p Thread %p %s", cmd.GetName(), exe_ready, fThread(), fThread()->GetName());

   if (exe_ready) {
      if (_FireDoNothingEvent()) {
         // we must be sure that event is accepted, it can be that worker is just break all its activity and pointer is no longer valid
         *exe_ready = true;
         return true;
      }

      return false;
   }

   // here command will be overtaken from the reference
   uint32_t id = fWorkerCommands.ChangeKind(cmd, CommandsQueue::kindReply);

   if (!_FireEvent(evntReplyCommand, id, 0)) {
      fWorkerCommands.PopWithId(id);
      if (!cmd.HasField("#no_warnings")) EOUT("Worker %s don't want get command for reply", GetName());
      return false;
   }

   return true;
}

void dabc::Worker::CancelCommands()
{
   fWorkerCommands.Cleanup(fThreadMutex, this, cmd_false);
}

void dabc::Worker::WorkerSleep(double tmout)
{
   ThreadRef thrd = thread();

   if (!thrd.null() && thrd.IsItself()) {
      thrd.RunEventLoop(tmout);
      return;
   }

   thrd = dabc::mgr.CurrentThread();
   if (!thrd.null()) {
      thrd.RunEventLoop(tmout);
      return;
   }

   dabc::Sleep(tmout);
}


bool dabc::Worker::RegisterForParameterEvent(const std::string &mask, bool onlychangeevent)
{
   return dabc::mgr.ParameterEventSubscription(this, true, mask, onlychangeevent);
}


bool dabc::Worker::UnregisterForParameterEvent(const std::string &mask)
{
   return dabc::mgr.ParameterEventSubscription(this, false, mask);
}


std::string dabc::Worker::WorkerAddress(bool full)
{
   return full ? dabc::mgr.ComposeAddress("",ItemName()) : ItemName();
}


dabc::Reference dabc::Worker::GetPublisher()
{
   if (fPublisher.null()) fPublisher = dabc::mgr.FindItem(dabc::Publisher::DfltName());
   return fPublisher;
}


bool dabc::Worker::Publish(const Hierarchy& h, const std::string &path)
{
   return PublisherRef(GetPublisher()).Register(path, ItemName(), h());
}


bool dabc::Worker::PublishPars(const std::string &path)
{
   // no need to publish if publisher not exists
   if (GetPublisher().null()) return true;

   if (fWorkerHierarchy.null())
      fWorkerHierarchy.Create("Worker");

   for (unsigned n = 0; n < NumChilds(); n++) {
      Parameter par = dynamic_cast<ParameterContainer*> (GetChild(n));
      if (par.null()) continue;

      Hierarchy chld = fWorkerHierarchy.FindChild(par.GetName());
      if (!chld.null()) continue;

      chld = fWorkerHierarchy.CreateHChild(par.GetName());

      if (par.IsRatemeter() || (par.Kind() == "info") || par.GetField("#record").AsBool()) {
         chld.EnableHistory(100);
         par()->fRecorded = true;
      }

      par.ScanParamFields(&chld()->Fields());
   }
   fWorkerHierarchy.MarkChangedItems();

   return Publish(fWorkerHierarchy, path);
}


bool dabc::Worker::Unpublish(const Hierarchy& h, const std::string &path)
{
   return PublisherRef(GetPublisher()).Unregister(path, ItemName(), h());
}

bool dabc::Worker::Subscribe(const std::string &path)
{
   return PublisherRef(GetPublisher()).Subscribe(path, ItemName());
}

bool dabc::Worker::Unsubscribe(const std::string &path)
{
   return PublisherRef(GetPublisher()).Unsubscribe(path, ItemName());
}

void dabc::Worker::CleanupPublisher(bool sync)
{
   if (fPublisher.null()) return;

   PublisherRef ref;

   ref << fPublisher;

   if (!ref.thread().null()) ref.RemoveWorker(ItemName(), sync);
}

// ===========================================================================================

bool dabc::WorkerRef::Submit(dabc::Command cmd)
{
   if (GetObject()) return GetObject()->Submit(cmd);

   cmd.ReplyFalse();
   return false;
}

bool dabc::WorkerRef::Execute(Command cmd, double tmout)
{
   if (!GetObject()) return false;
   return GetObject()->Execute(cmd, tmout);
}

bool dabc::WorkerRef::Execute(const std::string &cmd, double tmout)
{
   if (!GetObject()) return false;
   return GetObject()->Execute(cmd, tmout);
}

dabc::Parameter dabc::WorkerRef::Par(const std::string &name) const
{
   if (!GetObject()) return dabc::Parameter();
   return GetObject()->Par(name);
}

bool dabc::WorkerRef::HasThread() const
{
   return GetObject() ? GetObject()->HasThread() : false;
}

bool dabc::WorkerRef::IsSameThread(const WorkerRef& ref)
{
   if (GetObject() && ref.GetObject())
      return GetObject()->thread()() == ref.GetObject()->thread()();

   return false;
}


bool dabc::WorkerRef::CanSubmitCommand() const
{
   return GetObject() ? GetObject()->CanSubmitCommand() : false;
}

bool dabc::WorkerRef::SyncWorker(double tmout)
{
   if (!GetObject()) return true;

   dabc::Command cmd("SyncWorker");
   cmd.SetPriority(dabc::Worker::priorityMinimum);
   if (tmout >= 0.) cmd.SetTimeout(tmout);
   return Execute(cmd) == cmd_true;
}
