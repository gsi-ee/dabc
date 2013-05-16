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

#include "dabc/Application.h"

#include <string.h>

#include "dabc/Manager.h"
#include "dabc/logging.h"
#include "dabc/Configuration.h"
#include "dabc/Iterator.h"


dabc::ApplicationBase::ApplicationBase() :
   Worker(dabc::mgr(), xmlAppDfltName, true),
   fInitFunc(0),
   fWasRunning(false)
{
   CreatePar(StateParName(), "state").SetSynchron(true, -1, true);

   CreateCmdDef(stcmdDoStart());
   CreateCmdDef(stcmdDoStop());

   SetState(stHalted());
}

dabc::ApplicationBase::~ApplicationBase()
{

}

int dabc::ApplicationBase::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName(stcmdDoStart()) ||
       cmd.IsName(stcmdDoStop())) {
      bool res = false;
      if (IsTransitionAllowed(cmd.GetName()))
         res = DoStateTransition(cmd.GetName());
      return cmd_bool(res);
   }


   if (cmd.IsName(InvokeAppRunCmd::CmdName())) {

      bool res = PerformApplicationRun();

      return cmd_bool(res);
   } else

   if (cmd.IsName(InvokeAppFinishCmd::CmdName())) {

      bool res = PerformApplicationFinish();

      return cmd_bool(res);
   } else

   if (cmd.IsName(InvokeTransitionCmd::CmdName())) {

      InvokeTransitionCmd tr = cmd;

      bool res = false;

      if (IsTransitionAllowed(tr.GetTransition()))
         res = DoStateTransition(tr.GetTransition());

      return cmd_bool(res);
   } else

   if (cmd.IsName("CheckWorkDone")) {
      DOUT0("Verify that application did its job running %s modules %s",
            DBOOL(GetState() == stRunning()), DBOOL(IsModulesRunning()));

      if (IsWorkDone()) {
         DOUT0("Stop application while work is completed");
         Submit(InvokeAppFinishCmd());
      }

      return cmd_true;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}

bool dabc::ApplicationBase::IsWorkDone()
{
   return (GetState() == stRunning()) && !IsModulesRunning();
}

bool dabc::ApplicationBase::IsFinished()
{
   std::string state = GetState();

   if (state == stFailure()) return true;

   if (state != stHalted()) return false;

   LockGuard lock(ObjectMutex());
   return fWasRunning;
}

void dabc::ApplicationBase::SetInitFunc(ExternalFunction* initfunc)
{
   fInitFunc = initfunc;
}

bool dabc::ApplicationBase::PerformApplicationRun()
{
   ExecuteStateTransition(stcmdDoStart(), SMCommandTimeout());

   DOUT3("@@@@@@@@ Start application done state = %s", GetState().c_str());

   return GetState() == stRunning();
}

bool dabc::ApplicationBase::PerformApplicationFinish()
{
   ExecuteStateTransition(stcmdDoStop(), SMCommandTimeout());

   return GetState() == stHalted();
}

bool dabc::ApplicationBase::IsTransitionAllowed(const std::string& cmd)
{
   if (cmd == stcmdDoStop()) return true;

   if ((cmd == stcmdDoStart()) && (GetState() == stHalted())) return true;

   return false;

}

bool dabc::ApplicationBase::DoStateTransition(const std::string& cmd)
{
   bool res = true;
   std::string tgtstate;

   if (cmd == stcmdDoStart()) {

      if (fInitFunc!=0)
         fInitFunc();
      else
         res = DefaultInitFunc();

      if (res && !dabc::mgr.ActivateConnections(20.)) res = false;

      if (res && !StartModules()) res = false;

      if (res) {
         LockGuard lock(ObjectMutex());
         fWasRunning = true;
      }

      tgtstate = stRunning();
   } else
   if (cmd == stcmdDoStop()) {

      StopModules();

      CleanupApplication();

      tgtstate = stHalted();

   } else
      res = false;

   if (res) SetState(tgtstate);
       else SetState(stFailure());

   return res;
}


bool dabc::ApplicationBase::IsModulesRunning()
{
   ReferencesVector vect;

   GetAllChildRef(&vect);

   while (vect.GetSize()>0) {
      ModuleRef m = vect.TakeLast();
      if (m.IsRunning() && (std::string("MemoryPool")!=m.ClassName())) return true;
   }

   return false;
}


bool dabc::ApplicationBase::StartModules()
{
   ReferencesVector vect;

   GetAllChildRef(&vect);
   
   while (vect.GetSize()>0) {
      ModuleRef m = vect.TakeRef(0);
      m.Start();
   }

   return true;
}

bool dabc::ApplicationBase::StopModules()
{
   ReferencesVector vect;

   GetAllChildRef(&vect);

   while (vect.GetSize()>0) {
      ModuleRef m = vect.TakeRef(0);
      m.Stop();
   }

   DOUT2("Stop modules");

   return true;
}

bool dabc::ApplicationBase::CleanupApplication()
{
   ReferencesVector vect;

   GetAllChildRef(&vect);

   while (vect.GetSize()>0) {
      // by default, all workers are removed from the application
      WorkerRef w = vect.TakeRef(0);
      w.Destroy();
   }

   return true;
}

bool dabc::ApplicationBase::ExecuteStateTransition(const std::string& trans_cmd, double tmout)
{
   InvokeTransitionCmd cmd;
   cmd.SetTransition(trans_cmd);
   cmd.SetTimeout(tmout);
   return Execute(cmd);
}

bool dabc::ApplicationBase::InvokeStateTransition(const std::string& trans_cmd)
{
   InvokeTransitionCmd cmd;
   cmd.SetTransition(trans_cmd);
   return Submit(cmd);
}


bool dabc::ApplicationBase::CheckWorkDone()
{
   Submit(Command("CheckWorkDone"));
   return true;
}

bool dabc::ApplicationBase::Find(ConfigIO &cfg)
{
   while (cfg.FindItem(xmlApplication)) {
      // if application has non-default class name, one should check additionally class attribute
      if ((strcmp(ClassName(), xmlApplication) == 0) ||
            cfg.CheckAttr(xmlClassAttr, ClassName())) return true;
   }

   return false;
}

bool dabc::ApplicationBase::DefaultInitFunc()
{
   XMLNodePointer_t node = 0;

   while (dabc::mgr()->cfg()->NextCreationNode(node, xmlDeviceNode, true)) {
      const char* name = Xml::GetAttr(node, xmlNameAttr);
      const char* clname = Xml::GetAttr(node, xmlClassAttr);
      if ((name!=0) && (clname!=0)) dabc::mgr.CreateDevice(clname, name);
   }

   while (dabc::mgr()->cfg()->NextCreationNode(node, xmlThreadNode, true)) {
      const char* name = Xml::GetAttr(node, xmlNameAttr);
      const char* clname = Xml::GetAttr(node, xmlClassAttr);
      const char* devname = Xml::GetAttr(node, xmlDeviceAttr);
      if (name==0) continue;
      if (clname==0) clname = dabc::typeThread;
      if (devname==0) devname = "";
      DOUT0("Create thread %s", name);
      dabc::mgr.CreateThread(name, clname, devname);
   }

   while (dabc::mgr()->cfg()->NextCreationNode(node, xmlMemoryPoolNode, true)) {
      const char* name = Xml::GetAttr(node, xmlNameAttr);
      DOUT0("Create memory pool %s", name);
      if (!dabc::mgr.CreateMemoryPool(name)) return false;
   }

   while (dabc::mgr()->cfg()->NextCreationNode(node, xmlModuleNode, true)) {
      const char* name = Xml::GetAttr(node, xmlNameAttr);
      const char* clname = Xml::GetAttr(node, xmlClassAttr);
      const char* thrdname = Xml::GetAttr(node, xmlThreadAttr);
      if (clname==0) continue;
      DOUT0("Create module %s class %s", name, clname);
      if (thrdname==0) thrdname="";

      dabc::ModuleRef m = dabc::mgr.CreateModule(clname, name, thrdname);

      if (m.null()) return false;

      for (unsigned n = 0; n < m.NumInputs(); n++) {

         PortRef port = m.FindPort(m.InputName(n, false));
         if (!port.Cfg(xmlAutoAttr).AsBool(true)) continue;

         if (!dabc::mgr.CreateTransport(m.InputName(n))) {
            EOUT("Cannot create input transport for port %s", m.InputName(n).c_str());
            return false;
         }
      }

      for (unsigned n=0; n < m.NumOutputs(); n++) {

         PortRef port = m.FindPort(m.OutputName(n, false));
         if (!port.Cfg(xmlAutoAttr).AsBool(true)) continue;

         if (!dabc::mgr.CreateTransport(m.OutputName(n))) {
            EOUT("Cannot create output transport for port %s", m.OutputName(n).c_str());
            return false;
         }
      }
   }

   while (dabc::mgr()->cfg()->NextCreationNode(node, xmlConnectionNode, false)) {
      const char* outputname = Xml::GetAttr(node, "output");
      const char* inputname = Xml::GetAttr(node, "input");

      if ((outputname==0) || (inputname==0)) continue;
      dabc::ConnectionRequest req = dabc::mgr.Connect(outputname, inputname);

      if (req.null()) continue;

      const char* thrdname = Xml::GetAttr(node, xmlThreadAttr);
      req.SetConnThread(thrdname);

      const char* useackn = Xml::GetAttr(node, xmlUseacknAttr);
      if (useackn!=0)
         req.SetUseAckn(strcmp(useackn, xmlTrueValue)==0);

      const char* optional = Xml::GetAttr(node, xmlOptionalAttr);
      if (optional!=0)
         req.SetOptional(strcmp(optional, xmlTrueValue)==0);

      const char* devname = Xml::GetAttr(node, xmlDeviceAttr);
      if (devname!=0) req.SetConnDevice(devname);

      double val(10.);
      const char* tmout = Xml::GetAttr(node, xmlTimeoutAttr);
      if ((tmout!=0) && str_to_double(tmout, &val))
         req.SetConnTimeout(val);
   }

   return true;
}

// ==============================================================


dabc::Application::Application(const char* classname) :
   dabc::ApplicationBase(),
   fAppClass(classname ? classname : typeApplication),
   fNodes()
{
   DOUT3("Application %s created", ClassName());

   CreateCmdDef(stcmdDoConfigure());
   CreateCmdDef(stcmdDoEnable());
   CreateCmdDef(stcmdDoError());
   CreateCmdDef(stcmdDoHalt());

   GetFirstNodesConfig();
}

dabc::Application::~Application()
{
   DOUT3("Start Application %s destructor", GetName());

   while (fNodes.size()>0)
      delete (NodeStateRec*) fNodes.pop();

   DOUT3("Did Application %s destructor", GetName());
}

int dabc::Application::ExecuteCommand(Command cmd)
{
   if (cmd.IsName(stcmdDoConfigure()) ||
       cmd.IsName(stcmdDoEnable()) ||
       cmd.IsName(stcmdDoStart()) ||
       cmd.IsName(stcmdDoStop()) ||
       cmd.IsName(stcmdDoError()) ||
       cmd.IsName(stcmdDoHalt()))
   {
      bool res = false;
      if (IsTransitionAllowed(cmd.GetName()))
         res = DoStateTransition(cmd.GetName());
      return cmd_bool(res);
   }

   return ApplicationBase::ExecuteCommand(cmd);
}


void dabc::Application::GetFirstNodesConfig()
{
   unsigned num = dabc::mgr()->cfg()->NumNodes();

   for (unsigned n=0;n<num;n++) {
      NodeStateRec* rec = new NodeStateRec;

      rec->active = dabc::mgr()->cfg()->NodeActive(n);
      fNodes.push_back(rec);
   }
}

bool dabc::Application::CreateAppModules()
{
   // if no init func was specified, default will be called
   if (fInitFunc==0)
      return DefaultInitFunc();

   fInitFunc();
   return true;
}

bool dabc::Application::PerformApplicationRun()
{
   DOUT3("@@@@@@@@@@ Start application state = %s", GetState().c_str());

   // Halt application in any case
   // ExecuteStateTransition(stcmdDoHalt(), SMCommandTimeout());

   ExecuteStateTransition(stcmdDoConfigure(), SMCommandTimeout());

   ExecuteStateTransition(stcmdDoEnable(), SMCommandTimeout());

   ExecuteStateTransition(stcmdDoStart(), SMCommandTimeout());

   DOUT3("@@@@@@@@ Start application done state = %s", GetState().c_str());

   return GetState() == stRunning();
}

bool dabc::Application::PerformApplicationFinish()
{
   ExecuteStateTransition(stcmdDoStop(), SMCommandTimeout());

   // do halt will be done in any case
   ExecuteStateTransition(stcmdDoHalt(), SMCommandTimeout());

//   DOUT0("Finish application state = %s", GetState().c_str());

   return GetState() == stHalted();
}

bool dabc::Application::IsFinished()
{
   if (GetState() == stError()) return true;

   return dabc::ApplicationBase::IsFinished();
}


bool dabc::Application::IsTransitionAllowed(const std::string& cmd)
{
   std::string state = GetState();

   if ((state == stHalted()) && (cmd == stcmdDoConfigure())) return true;

   if ((state == stConfigured()) && (cmd == stcmdDoEnable())) return true;

   if ((state == stReady()) && (cmd == stcmdDoStart())) return true;

   if ((state == stRunning()) && (cmd == stcmdDoStop())) return true;

   if (cmd == stcmdDoHalt()) return true;

   if (cmd == stcmdDoError()) return true;

   return false;
}


bool dabc::Application::DoStateTransition(const std::string& cmd)
{
   // method called from application thread

   bool res = true;

   std::string tgtstate;

   if (cmd == stcmdDoConfigure()) {
      res = CreateAppModules();
      tgtstate = stConfigured();
      DOUT2("Configure res = %s", DBOOL(res));
   } else
   if (cmd == stcmdDoEnable()) {
      res = dabc::mgr.ActivateConnections(SMCommandTimeout());
      tgtstate = stReady();
      DOUT2("Enable res = %s", DBOOL(res));
   } else
   if (cmd == stcmdDoStart()) {
      res = BeforeAppModulesStarted();
      res = StartModules() && res;
      if (res) {
         LockGuard lock(ObjectMutex());
         fWasRunning = true;
      }
      DOUT2("Start res = %s", DBOOL(res));
      tgtstate = stRunning();
   } else
   if (cmd == stcmdDoStop()) {
      res = StopModules();
      res = AfterAppModulesStopped() && res;
      DOUT2("Stop res = %s", DBOOL(res));
      tgtstate = stReady();
   } else
   if (cmd == stcmdDoHalt()) {
      res = BeforeAppModulesDestroyed();
      res = CleanupApplication() && res;
      DOUT2("Halt res = %s", DBOOL(res));
      tgtstate = stHalted();
   } else
   if (cmd == stcmdDoError()) {
      res = StopModules();
      tgtstate = stError();
   } else
      res = false;

   if (res) SetState(tgtstate);
       else SetState(stFailure());

   return res;
}


bool dabc::Application::MakeSystemSnapshot(double tmout)
{
   dabc::CmdGetNodesState cmd;
   cmd.SetTimeout(tmout);

   if (!Execute(cmd)) return false;

   std::string sbuf = cmd.GetStdStr(CmdGetNodesState::States());

   for (unsigned n=0; n<NumNodes(); n++)
      NodeRec(n)->active = dabc::CmdGetNodesState::GetState(sbuf, n);

   return true;
}


void dabc::ApplicationRef::CheckWorkDone()
{
   Submit(Command("CheckWorkDone"));
}


bool dabc::ApplicationRef::IsWorkDone()
{
   return GetObject() ? GetObject()->IsWorkDone() : false;
}

bool dabc::ApplicationRef::IsFinished()
{
   return GetObject() ? GetObject()->IsFinished() : false;
}
