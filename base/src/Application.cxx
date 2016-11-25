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
#include "dabc/Url.h"
#include "dabc/Iterator.h"
#include "dabc/Hierarchy.h"

dabc::Application::Application(const char* classname) :
   Worker(dabc::mgr(), xmlAppDfltName),
   fAppClass(classname ? classname : typeApplication),
   fInitFunc(0),
   fAnyModuleWasRunning(false),
   fSelfControl(true),
   fAppDevices(),
   fAppPools(),
   fAppModules()
{
   CreatePar(StateParName(), "state").SetSynchron(true, -1, true);

   CreateCmdDef(stcmdDoConfigure());
   CreateCmdDef(stcmdDoStart());
   CreateCmdDef(stcmdDoStop());
   CreateCmdDef(stcmdDoHalt());

   SetAppState(stHalted());

   fSelfControl = Cfg("self").AsBool(true);

   PublishPars("$CONTEXT$/App");
}

dabc::Application::~Application()
{
}

void dabc::Application::OnThreadAssigned()
{
   if (fSelfControl)
      Submit(dabc::CmdStateTransition(stRunning()));
}

void dabc::Application::ObjectCleanup()
{
   dabc::Worker::ObjectCleanup();
}

int dabc::Application::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName(CmdStateTransition::CmdName()))
      return cmd_bool(DoTransition(cmd.GetStr("State")));

   if (cmd.IsName(stcmdDoConfigure()))
      return cmd_bool(DoTransition(stReady()));

   if (cmd.IsName(stcmdDoStart()))
      return cmd_bool(DoTransition(stRunning()));

   if (cmd.IsName(stcmdDoStop()))
      return cmd_bool(DoTransition(stReady()));

   if (cmd.IsName(stcmdDoHalt()))
      return cmd_bool(DoTransition(stHalted()));


   if (cmd.IsName("AddAppObject")) {
      if (cmd.GetStr("kind") == "device")
         fAppDevices.push_back(cmd.GetStr("name"));
      else
      if (cmd.GetStr("kind") == "pool")
         fAppPools.push_back(cmd.GetStr("name"));
      else
      if (cmd.GetStr("kind") == "module")
         fAppModules.push_back(cmd.GetStr("name"));
      else
         return cmd_false;

      return cmd_true;
   }

   if (cmd.IsName("StartAllModules") || cmd.IsName(CmdStartModule::CmdName())) {
      StartModules();
      return cmd_true;
   }

   if (cmd.IsName("StopAllModules")  || cmd.IsName(CmdStopModule::CmdName())) {
      StopModules();
      return cmd_true;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}


void dabc::Application::SetInitFunc(ExternalFunction* initfunc)
{
   fInitFunc = initfunc;
}

void dabc::Application::SetAppState(const std::string& name)
{
   Parameter par = Par(StateParName());
   par.SetValue(name);

   Hierarchy chld = fWorkerHierarchy.FindChild(StateParName());
   if (!chld.null()) {
      par.ScanParamFields(&chld()->Fields());
      fWorkerHierarchy.MarkChangedItems();
   }
}

bool dabc::Application::DoTransition(const std::string& tgtstate)
{
   std::string currstate = GetState();

   DOUT3("DoTransition curr %s tgt %s", currstate.c_str(), tgtstate.c_str());

   if (currstate == tgtstate) return true;

   // it is not allowed to change transition state - it is internal
   if (currstate == stTransition()) return false;

   SetAppState(stTransition());

   bool res = true;

   // in case of failure state always bring application into halted state first
   if (currstate == stFailure()) {
      res = CleanupApplication();
      if (res)  currstate = stHalted();
   }

   if (tgtstate == stHalted()) {
      if (currstate == stRunning()) res = StopModules();
      if (!CleanupApplication()) res = false;
   } else
   if (tgtstate == stReady()) {
      if (currstate == stHalted()) res = CreateAppModules(); else
      if (currstate == stRunning()) res = StopModules();
   } else
   if (tgtstate == stRunning()) {
      if (currstate == stHalted()) {
         if (CreateAppModules()) currstate = stReady(); else res = false;
      }
      if (currstate == stReady())
         if (!StartModules()) res = false;

      // use timeout to control if application should be shutdown
      if (res && fSelfControl) ActivateTimeout(0.2);

   } else
   if (tgtstate == stFailure()) {
      StopModules();
   } else {
      EOUT("Unsupported state name %s", tgtstate.c_str());
      res = false;
   }

   if (res) SetAppState(tgtstate);
       else SetAppState(stFailure());

   return res;
}

double dabc::Application::ProcessTimeout(double)
{
   if (!fSelfControl || (GetState() != stRunning())) return -1;

   if (IsModulesRunning()) { fAnyModuleWasRunning = true; return 0.2; }

   // if non modules was running, do not try automatic stop
   if (!fAnyModuleWasRunning) return 0.2;

   // application decide to stop manager main loop
   dabc::mgr.Submit(dabc::Command("StopManagerMainLoop"));

   return -1;
}


bool dabc::Application::IsModulesRunning()
{
   for (unsigned n=0;n<fAppModules.size();n++) {
      ModuleRef m = dabc::mgr.FindModule(fAppModules[n]);
      if (m.IsRunning()) return true;
   }

   return false;
}


bool dabc::Application::CreateAppModules()
{
   // if no init func was specified, default will be called
   if (fInitFunc==0)
      return DefaultInitFunc();

   fInitFunc();
   return true;
}

bool dabc::Application::StartModules()
{
   for (unsigned n=0;n<fAppModules.size();n++)
      dabc::mgr.StartModule(fAppModules[n]);

   return true;
}

bool dabc::Application::StopModules()
{
   for (unsigned n=0;n<fAppModules.size();n++)
      dabc::mgr.StopModule(fAppModules[n]);

   return true;
}

bool dabc::Application::CleanupApplication()
{
   for (unsigned n=0;n<fAppModules.size();n++)
      dabc::mgr.DeleteModule(fAppModules[n]);

   for (unsigned n=0;n<fAppPools.size();n++)
      dabc::mgr.DeletePool(fAppPools[n]);

   for (unsigned n=0;n<fAppDevices.size();n++)
      dabc::mgr.DeleteDevice(fAppDevices[n]);

   fAppModules.clear();
   fAppPools.clear();
   fAppDevices.clear();

/*
   ReferencesVector vect;

   GetAllChildRef(&vect);

   while (vect.GetSize()>0) {
      // by default, all workers are removed from the application
      WorkerRef w = vect.TakeRef(0);
      w.Destroy();
   }
*/
   return true;
}


bool dabc::Application::Find(ConfigIO &cfg)
{
   while (cfg.FindItem(xmlApplication)) {
      // if application has non-default class name, one should check additionally class attribute
      if ((strcmp(ClassName(), xmlApplication) == 0) ||
            cfg.CheckAttr(xmlClassAttr, ClassName())) return true;
   }

   return false;
}

bool dabc::Application::DefaultInitFunc()
{
   XMLNodePointer_t node = 0;

   while (dabc::mgr()->cfg()->NextCreationNode(node, xmlDeviceNode, true)) {
      const char* name = Xml::GetAttr(node, xmlNameAttr);
      const char* clname = Xml::GetAttr(node, xmlClassAttr);
      if ((name==0) || (clname==0)) continue;

      fAppDevices.push_back(name);

      if (!dabc::mgr.CreateDevice(clname, name)) {
         EOUT("Fail to create device %s class %s", name, clname);
         return false;
      }
   }

   while (dabc::mgr()->cfg()->NextCreationNode(node, xmlThreadNode, true)) {
      const char* name = Xml::GetAttr(node, xmlNameAttr);
      const char* clname = Xml::GetAttr(node, xmlClassAttr);
      const char* devname = Xml::GetAttr(node, xmlDeviceAttr);
      if (name==0) continue;
      if (clname==0) clname = dabc::typeThread;
      if (devname==0) devname = "";
      DOUT2("Create thread %s", name);
      dabc::mgr.CreateThread(name, clname, devname);
   }

   while (dabc::mgr()->cfg()->NextCreationNode(node, xmlMemoryPoolNode, true)) {
      const char* name = Xml::GetAttr(node, xmlNameAttr);
      fAppPools.push_back(name);
      DOUT2("Create memory pool %s", name);
      if (!dabc::mgr.CreateMemoryPool(name)) {
         EOUT("Fail to create memory pool %s", name);
         return false;
      }
   }

   while (dabc::mgr()->cfg()->NextCreationNode(node, xmlModuleNode, true)) {
      const char* name = Xml::GetAttr(node, xmlNameAttr);
      const char* clname = Xml::GetAttr(node, xmlClassAttr);
      const char* thrdname = Xml::GetAttr(node, xmlThreadAttr);
      if (clname==0) continue;
      if (thrdname==0) thrdname="";

      // check that module with such name exists
      dabc::ModuleRef m = dabc::mgr.FindModule(name);
      if (!m.null()) continue;

      // FIXME: for old xml files, remove after 12.2014
      if (strcmp(clname, "dabc::Publisher")==0) continue;

      fAppModules.push_back(name);

      DOUT2("Create module %s class %s", name, clname);

      m = dabc::mgr.CreateModule(clname, name, thrdname);

      if (m.null()) {
         EOUT("Fail to create module %s class %s", name, clname);
         return false;
      }

      for (unsigned n = 0; n < m.NumInputs(); n++) {

         PortRef port = m.FindPort(m.InputName(n, false));
         if (!port.Cfg(xmlAutoAttr).AsBool(true)) continue;

         if (!dabc::mgr.CreateTransport(m.InputName(n))) {
            EOUT("Cannot create input transport for port %s", m.InputName(n).c_str());
            return false;
         }
      }

      for (unsigned n = 0; n < m.NumOutputs(); n++) {

         PortRef port = m.FindPort(m.OutputName(n, false));
         if (!port.Cfg(xmlAutoAttr).AsBool(true)) continue;

         if (!dabc::mgr.CreateTransport(m.OutputName(n))) {
            EOUT("Cannot create output transport for port %s", m.OutputName(n).c_str());
            return false;
         }
      }
   }

   int nconn = 0;

   while (dabc::mgr()->cfg()->NextCreationNode(node, xmlConnectionNode, false)) {

      const char* outputname = Xml::GetAttr(node, "output");
      const char* inputname = Xml::GetAttr(node, "input");

      // output and input should always be specified
      if ((outputname==0) || (inputname==0)) continue;

      const char* kind = Xml::GetAttr(node, "kind");

      if ((kind==0) || (strcmp(kind,"all-to-all")!=0)) {
         dabc::ConnectionRequest req = dabc::mgr.Connect(outputname, inputname);
         req.SetConfigFromXml(node);
         if (!req.null()) nconn++;
      } else {
         nconn++;
         int numnodes = dabc::mgr.NumNodes();

         DOUT2("Create all-to-all connections for %d nodes", numnodes);

         for (int nsender=0; nsender<numnodes; nsender++)
            for (int nreceiver=0;nreceiver<numnodes;nreceiver++) {
               std::string port1 = dabc::Url::ComposePortName(nsender, dabc::format("%s/Output", outputname), nreceiver);

               std::string port2 = dabc::Url::ComposePortName(nreceiver, dabc::format("%s/Input", inputname), nsender);

               dabc::ConnectionRequest req = dabc::mgr.Connect(port1, port2);
               req.SetConfigFromXml(node);
            }
      }
   }

   if (nconn>0) dabc::mgr.ActivateConnections(5);

   return true;
}


void dabc::Application::BuildFieldsMap(RecordFieldsMap* cont)
{
   cont->Field(dabc::prop_kind).SetStr("DABC.Application");
}

bool dabc::ApplicationRef::AddObject(const std::string& kind, const std::string& name)
{
   dabc::Command cmd("AddAppObject");
   cmd.SetStr("kind", kind);
   cmd.SetStr("name", name);
   return Execute(cmd);
}
