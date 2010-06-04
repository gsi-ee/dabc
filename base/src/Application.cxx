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
#include "dabc/Application.h"

#include <string.h>

#include "dabc/Manager.h"
#include "dabc/logging.h"
#include "dabc/Configuration.h"

dabc::Application::Application(const char* classname) :
   Folder(dabc::mgr(), xmlAppDfltName, true),
   WorkingProcessor(this),
   fAppClass(classname ? classname : typeApplication),
   fConnCmd(0),
   fConnTmout(0),
   fInitFunc(0)
{
   DOUT3(("Application %s created", ClassName()));
}

dabc::Application::Application(ExternalFunction* initfunc) :
   Folder(dabc::mgr(), xmlAppDfltName, true),
   WorkingProcessor(this),
   fAppClass(typeApplication),
   fConnCmd(0),
   fConnTmout(0),
   fInitFunc(initfunc)
{
   DOUT3(("Application %s created", ClassName()));
}

dabc::Application::~Application()
{
   DOUT3(("Start Application %s destructor", GetName()));

   dabc::Command::Reply(fConnCmd, false);
   fConnCmd = 0;

   // delete children (parameters) before destructor is finished that one
   // can correctly use GetParsHolder() in Manager::ParameterEvent()
   DestroyAllPars();

   DOUT3(("Did Application %s destructor", GetName()));
}

const char* dabc::Application::MasterClassName() const
{
   return xmlApplication;
}

bool dabc::Application::CreateAppModules()
{
   if (fInitFunc!=0) fInitFunc();
   return true;
}

int dabc::Application::ConnectAppModules(Command* cmd)
{
   if (fConnCmd!=0) {
      EOUT(("There is active connect command !!!"));
      return cmd_false;
   }

   DOUT2(("Call ConnectAppModules"));

   int res = IsAppModulesConnected();

   if (res == cmd_postponed) {
      fConnCmd = cmd;
      fConnTmout = SMCommandTimeout() - 0.5;
      if (fConnTmout<0.5) fConnTmout = 0.5;
      ActivateTimeout(0.2);
   }
   return res;
}

int dabc::Application::ExecuteCommand(dabc::Command* cmd)
{
   int cmd_res = cmd_false;

   if (cmd->IsName("CreateAppModules")) {
      cmd_res = cmd_bool(CreateAppModules());
   } else
   if (cmd->IsName("ConnectAppModules")) {
      cmd_res = ConnectAppModules(cmd);
   } else
   if (cmd->IsName("BeforeAppModulesStarted")) {
      cmd_res = cmd_bool(BeforeAppModulesStarted());
   } else
   if (cmd->IsName("AfterAppModulesStopped")) {
      cmd_res = cmd_bool(AfterAppModulesStopped());
   } else
   if (cmd->IsName("BeforeAppModulesDestroyed")) {
      cmd_res = cmd_bool(BeforeAppModulesDestroyed());
   } else
   if (cmd->IsName("CheckModulesRunning")) {
      if (!IsModulesRunning()) {
         if (dabc::mgr()->CurrentState() != dabc::Manager::stReady) {
            DOUT1(("!!!!! ******** !!!!!!!!  All main modules are stopped - we can switch to Stop state"));
            dabc::mgr()->InvokeStateTransition(dabc::Manager::stcmdDoStop);
         }
      }
   } else
      cmd_res = dabc::WorkingProcessor::ExecuteCommand(cmd);

   return cmd_res;
}

double dabc::Application::ProcessTimeout(double last_diff)
{
   // we using timeout events to check if connection is established

   if (fConnCmd==0) return -1;

   fConnTmout -= last_diff;

   int res = IsAppModulesConnected();

   if ((res==cmd_postponed) && (fConnTmout<0)) {
      EOUT(("Connection timedout"));
      res = cmd_false;
   }

   if ((res==cmd_false) || (res==cmd_true)) {
      DOUT0(("Modules connected res = %d", res));

      dabc::Command::Reply(fConnCmd, res);
      fConnCmd = 0;
      return -1.;
   }

   return 0.5;
}


bool dabc::Application::DoStateTransition(const char* state_trans_name)
{
   // method called from SM thread, one should use Execute at this place

   bool res = true;

   if (strcmp(state_trans_name, dabc::Manager::stcmdDoConfigure)==0) {
      res = Execute("CreateAppModules", SMCommandTimeout());
   } else
   if (strcmp(state_trans_name, dabc::Manager::stcmdDoEnable)==0) {
      res = Execute("ConnectAppModules", SMCommandTimeout());
   } else
   if (strcmp(state_trans_name, dabc::Manager::stcmdDoStart)==0) {
      res = Execute("BeforeAppModulesStarted", SMCommandTimeout());
      res = dabc::mgr()->StartAllModules() && res;
   } else
   if (strcmp(state_trans_name, dabc::Manager::stcmdDoStop)==0) {
      res = dabc::mgr()->StopAllModules();
      res = Execute("AfterAppModulesStopped", SMCommandTimeout()) && res;
   } else
   if (strcmp(state_trans_name, dabc::Manager::stcmdDoHalt)==0) {
      res = Execute("BeforeAppModulesDestroyed", SMCommandTimeout());
      res = dabc::mgr()->CleanupManager() && res;
   } else
   if (strcmp(state_trans_name, dabc::Manager::stcmdDoError)==0) {
      res = dabc::mgr()->StopAllModules();
   } else
      res = false;

   return res;
}

bool dabc::Application::IsModulesRunning()
{
   return dabc::mgr()->IsAnyModuleRunning();
}

void dabc::Application::InvokeCheckModulesCmd()
{
   Submit(new Command("CheckModulesRunning"));
}

bool dabc::Application::Store(ConfigIO &cfg)
{
   cfg.CreateItem(xmlApplication);

   if (strcmp(ClassName(), MasterClassName()) != 0)
      cfg.CreateAttr(xmlClassAttr, ClassName());

   StoreChilds(cfg);

   cfg.PopItem();

   return true;
}

bool dabc::Application::Find(ConfigIO &cfg)
{
   if (!cfg.FindItem(xmlApplication)) return false;

   if (strcmp(ClassName(), MasterClassName()) != 0)
      if (!cfg.CheckAttr(xmlClassAttr, ClassName())) return false;

   return true;
}
