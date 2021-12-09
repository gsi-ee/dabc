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

#include <cstring>

#include "dabc/Manager.h"
#include "dabc/Configuration.h"
#include "dabc/Url.h"

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

   fConnTimeout = Cfg("ConnTimeout").AsDouble(5);
   fConnDebug = Cfg("ConnDebug").AsBool(false);

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


bool dabc::Application::ReplyCommand(Command cmd)
{
   Command statecmd = cmd.GetRef("StateCmd");
   if (statecmd.null()) return dabc::Worker::ReplyCommand(cmd);

   // this is finish of modules connection - we should complete state transition
   std::string tgtstate = cmd.GetStr("StateCmdTarget");
   int res = cmd.GetResult();

   if (tgtstate == stRunning() && (res == cmd_true)) {
      if (!StartModules()) res = cmd_false;
      // use timeout to control if application should be shutdown
      if ((res==cmd_true) && fSelfControl) ActivateTimeout(0.2);
   }

   if (res==cmd_true) SetAppState(tgtstate);
   if (res==cmd_false) SetAppState(stFailure());

   statecmd.Reply(res);
   return true;
}


int dabc::Application::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName(CmdStateTransition::CmdName()))
      return DoTransition(cmd.GetStr("State"), cmd);

   if (cmd.IsName(stcmdDoConfigure()))
      return DoTransition(stReady(), cmd);

   if (cmd.IsName(stcmdDoStart()))
      return DoTransition(stRunning(), cmd);

   if (cmd.IsName(stcmdDoStop()))
      return DoTransition(stReady(), cmd);

   if (cmd.IsName(stcmdDoHalt()))
      return DoTransition(stHalted(), cmd);

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

void dabc::Application::SetAppState(const std::string &name)
{
   SetParValue(StateParName(), name);
   if (name == stFailure()) DOUT0("Application switched to FAILURE state");
}

int dabc::Application::DoTransition(const std::string &tgtstate, Command cmd)
{
   std::string currstate = GetState();

   DOUT3("Doing transition curr %s tgt %s", currstate.c_str(), tgtstate.c_str());

   if (currstate == tgtstate) return cmd_true;

   // it is not allowed to change transition state - it is internal
   if (currstate == stTransition()) return cmd_false;

   SetAppState(stTransition());

   int res = cmd_true;

   // in case of failure state always bring application into halted state first
   if (currstate == stFailure()) {
      res = CleanupApplication();
      if (res)  currstate = stHalted();
   }

   if (tgtstate == stHalted()) {
      if (currstate == stRunning()) res = cmd_bool(StopModules());
      if (!CleanupApplication()) res = cmd_false;
   } else
   if (tgtstate == stReady()) {
      if (currstate == stHalted()) res = CallInitFunc(cmd, tgtstate); else
      if (currstate == stRunning()) res = cmd_bool(StopModules());
   } else
   if (tgtstate == stRunning()) {
      if (currstate == stHalted()) {
         res = CallInitFunc(cmd, tgtstate);
         if (res == cmd_true) currstate = stReady();
      }
      if (currstate == stReady())
         if (!StartModules()) res = cmd_false;

      // use timeout to control if application should be shutdown
      if ((res==cmd_true) && fSelfControl) ActivateTimeout(0.2);

   } else
   if (tgtstate == stFailure()) {
      StopModules();
   } else {
      EOUT("Unsupported state name %s", tgtstate.c_str());
      res = cmd_false;
   }

   if (res==cmd_true) SetAppState(tgtstate);
   if (res==cmd_false) SetAppState(stFailure());

   return res;
}

double dabc::Application::ProcessTimeout(double)
{
   if (!fSelfControl || (GetState() != stRunning())) return -1;

   if (IsModulesRunning()) { fAnyModuleWasRunning = true; return 0.2; }

   // if non modules was running, do not try automatic stop
   if (!fAnyModuleWasRunning) return 0.2;

   // application decide to stop manager main loop
   dabc::mgr.StopApplication();

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


int dabc::Application::CallInitFunc(Command statecmd, const std::string &tgtstate)
{
   if (fInitFunc) {
      fInitFunc();
      return cmd_true;
   }

   // TODO: check that function is redefined
   if (CreateAppModules()) return cmd_true;

   XMLNodePointer_t node = 0;
   dabc::Configuration* cfg = dabc::mgr()->cfg();

   while (cfg->NextCreationNode(node, xmlDeviceNode, true)) {
      const char* name = Xml::GetAttr(node, xmlNameAttr);
      const char* clname = Xml::GetAttr(node, xmlClassAttr);
      if ((name==0) || (clname==0)) continue;

      fAppDevices.push_back(name);

      if (!dabc::mgr.CreateDevice(clname, name)) {
         EOUT("Fail to create device %s class %s", name, clname);
         return cmd_false;
      }
   }

   while (cfg->NextCreationNode(node, xmlThreadNode, true)) {
      const char* name = Xml::GetAttr(node, xmlNameAttr);
      const char* clname = Xml::GetAttr(node, xmlClassAttr);
      const char* devname = Xml::GetAttr(node, xmlDeviceAttr);
      if (name==0) continue;
      if (clname==0) clname = dabc::typeThread;
      if (devname==0) devname = "";
      DOUT2("Create thread %s", name);
      dabc::mgr.CreateThread(name, clname, devname);
   }

   while (cfg->NextCreationNode(node, xmlMemoryPoolNode, true)) {
      const char* name = Xml::GetAttr(node, xmlNameAttr);
      fAppPools.push_back(name);
      DOUT2("Create memory pool %s", name);
      if (!dabc::mgr.CreateMemoryPool(name)) {
         EOUT("Fail to create memory pool %s", name);
         return cmd_false;
      }
   }

   while (cfg->NextCreationNode(node, xmlModuleNode, true)) {
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
         return cmd_false;
      }

      for (unsigned n = 0; n < m.NumInputs(); n++) {

         PortRef port = m.FindPort(m.InputName(n, false));
         if (!port.Cfg(xmlAutoAttr).AsBool(true)) continue;

         if (!dabc::mgr.CreateTransport(m.InputName(n))) {
            EOUT("Cannot create input transport for port %s", m.InputName(n).c_str());
            return cmd_false;
         }
      }

      for (unsigned n = 0; n < m.NumOutputs(); n++) {

         PortRef port = m.FindPort(m.OutputName(n, false));
         if (!port.Cfg(xmlAutoAttr).AsBool(true)) continue;

         if (!dabc::mgr.CreateTransport(m.OutputName(n))) {
            EOUT("Cannot create output transport for port %s", m.OutputName(n).c_str());
            return cmd_false;
         }
      }
   }

   int nconn = 0;

   while (cfg->NextCreationNode(node, xmlConnectionNode, false)) {

      const char* outputname = Xml::GetAttr(node, "output");
      const char* inputname = Xml::GetAttr(node, "input");

      // output and input should always be specified
      if ((outputname==0) || (inputname==0)) continue;

      const char* kind = Xml::GetAttr(node, "kind");
      const char* lst = Xml::GetAttr(node, "list");

      std::vector<std::string> arr;

      if (lst && *lst) {
         dabc::RecordField fld(cfg->ResolveEnv(lst));
         arr = fld.AsStrVect();
      }

      if (kind && (strcmp(kind,"all-to-all")==0)) {
         int numnodes = 1;
         if (arr.size() > 0)
            numnodes = arr.size();
         else
            dabc::mgr.NumNodes();

         DOUT2("Create all-to-all connections for %d nodes", numnodes);

         for (int nsender = 0; nsender < numnodes; nsender++)
            for (int nreceiver = 0; nreceiver<numnodes; nreceiver++) {
               std::string port1, port2;

               if (arr.size() == 0) {
                  port1 = dabc::Url::ComposePortName(nsender, dabc::format("%s/Output", outputname), nreceiver);
                  port2 = dabc::Url::ComposePortName(nreceiver, dabc::format("%s/Input", inputname), nsender);
               } else {
                  port1 = dabc::format("dabc://%s/%s/Output%d", arr[nsender].c_str(), outputname, nreceiver);
                  port2 = dabc::format("dabc://%s/%s/Input%d", arr[nreceiver].c_str(), inputname, nsender);
               }

               dabc::ConnectionRequest req = dabc::mgr.Connect(port1, port2);
               req.SetConfigFromXml(node);
               if (!req.null()) nconn++;
            }
      } else if (arr.size() > 0) {
         for (unsigned n = 0; n < arr.size(); ++n) {
            std::string out = dabc::replace_all(cfg->ResolveEnv(outputname), "%name%", arr[n]),
                        inp = dabc::replace_all(cfg->ResolveEnv(inputname), "%name%", arr[n]),
                        id = dabc::format("%u", n);
            out = dabc::replace_all(out, "%id%", id);
            inp = dabc::replace_all(inp, "%id%", id);

            dabc::ConnectionRequest req = dabc::mgr.Connect(out, inp);
            req.SetConfigFromXml(node);
            if (!req.null()) nconn++;
         }
      } else {
         dabc::ConnectionRequest req = dabc::mgr.Connect(outputname, inputname);
         req.SetConfigFromXml(node);
         if (!req.null()) nconn++;
      }
   }

   if (nconn==0) return cmd_true;

   dabc::Command cmd("ActivateConnections");
   cmd.SetTimeout(fConnTimeout);
   cmd.SetReceiver(dabc::Manager::ConnMgrName());
   cmd.SetBool("ConnDebug", fConnDebug);
   cmd.SetRef("StateCmd", statecmd);
   cmd.SetStr("StateCmdTarget", tgtstate);

   if (fConnDebug)

   dabc::mgr.Submit(Assign(cmd));

   return cmd_postponed;

//   return cmd_bool(dabc::mgr.ActivateConnections(5));
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

void dabc::Application::BuildFieldsMap(RecordFieldsMap* cont)
{
   cont->Field(dabc::prop_kind).SetStr("DABC.Application");
}

bool dabc::ApplicationRef::AddObject(const std::string &kind, const std::string &name)
{
   dabc::Command cmd("AddAppObject");
   cmd.SetStr("kind", kind);
   cmd.SetStr("name", name);
   return Execute(cmd);
}
