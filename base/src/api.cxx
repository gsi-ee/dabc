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

#include "dabc/api.h"

#include <cstdio>
#include <cstdlib>
#include <signal.h>

#include "dabc/Publisher.h"
#include "dabc/Configuration.h"
#include "dabc/Manager.h"

bool dabc::CreateManager(const std::string &name, int cmd_port)
{
   bool dofactories = false;

   if (dabc::mgr.null()) {
      static dabc::Configuration cfg;

      new dabc::Manager(name, &cfg);

      // ensure that all submitted events are processed
      dabc::mgr.SyncWorker();

      dofactories = true;
   }

   if (cmd_port>=0)
      dabc::mgr.CreateControl(cmd_port > 0, cmd_port);

   if (dofactories) {
      dabc::mgr.Execute("InitFactories");
   }

   return true;
}

dabc::Thread_t DABC_SigThrd = 0;
int DABC_SigCnt = 0;

void DABC_GLOBAL_CtrlCHandler(int number)
{
   if (DABC_SigThrd != dabc::PosixThread::Self()) return;

   DABC_SigCnt++;

   if ((DABC_SigCnt>2) || (dabc::mgr()==0)) {
      printf("Force application exit\n");
      if (dabc::lgr()!=0) dabc::lgr()->CloseFile();
      exit(0);
   }

   dabc::mgr()->ProcessCtrlCSignal();
}

void DABC_GLOBAL_SigPipeHandler(int number)
{
 // JAM2018:This was introduced to catch socket signals raised by external plugin libraries like LTSM/TSM client
 // note that DABC sockets have SIGPIPE disabled by default
 // todo: maybe one global signal handlers for all signals. For the moment, keep things rather separate...
  if (DABC_SigThrd != dabc::PosixThread::Self()) return;
  if (dabc::mgr()==0) {
     printf("DABC_GLOBAL_SigPipeHandler: no manager, Force application exit\n");
     if (dabc::lgr()!=0) dabc::lgr()->CloseFile();
     exit(0);
  }
  dabc::mgr()->ProcessPipeSignal();
}

bool dabc::InstallSignalHandlers()
{
   if (DABC_SigThrd!=0) {
      printf("Signal handler was already installed !!!\n");
      return false;
   }

   DABC_SigThrd = dabc::PosixThread::Self();

   if (signal(SIGINT, DABC_GLOBAL_CtrlCHandler)==SIG_ERR) {
      printf("Cannot change handler for SIGINT\n");
      return false;
   }

   if (signal(SIGPIPE, DABC_GLOBAL_SigPipeHandler)==SIG_ERR) {
        printf("Cannot change handler for SIGPIPE\n");
        return false;
    }

   return true;
}


bool dabc::CtrlCPressed()
{
   return DABC_SigCnt > 0;
}


bool dabc::DestroyManager()
{
   if (dabc::mgr.null()) return true;

   dabc::mgr()->HaltManager();

   dabc::mgr.Destroy();

   return true;
}


std::string dabc::MakeNodeName(const std::string &arg)
{
   size_t pos = arg.find("dabc://");
   if (pos==std::string::npos)
      return std::string("dabc://") + arg;
   if (pos == 0) return arg;

   return std::string();
}


bool dabc::ConnectDabcNode(const std::string &nodeaddr)
{
   if (dabc::mgr.null()) {
      EOUT("Manager was not created");
      return false;
   }

   std::string fullname = MakeNodeName(nodeaddr);
   if (fullname.empty()) {
      EOUT("Wrong address format %s", nodeaddr.c_str());
      return false;
   }

   dabc::mgr.CreateControl(false);

   dabc::Command cmd("Ping");
   cmd.SetReceiver(fullname);
   cmd.SetTimeout(10);

   if (dabc::mgr.GetCommandChannel().Execute(cmd) != dabc::cmd_true) {
      EOUT("FAIL connection to %s", nodeaddr.c_str());
      return false;
   }

   return true;
}

dabc::Hierarchy dabc::GetNodeHierarchy(const std::string &nodeaddr)
{
   dabc::Hierarchy res;

   std::string fullname = MakeNodeName(nodeaddr);
   if (fullname.empty()) {
      EOUT("Wrong address format %s", nodeaddr.c_str());
      return res;
   }

   dabc::CmdGetNamesList cmd;
   cmd.SetReceiver(fullname + dabc::Publisher::DfltName());
   cmd.SetTimeout(10);

   if (dabc::mgr.GetCommandChannel().Execute(cmd)!=dabc::cmd_true) {
      EOUT("Fail to get hierarchy from node %s", nodeaddr.c_str());
      return res;
   }

   res = dabc::CmdGetNamesList::GetResNamesList(cmd);

   return res;
}
