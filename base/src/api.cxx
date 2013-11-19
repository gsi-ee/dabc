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

#include "dabc/Publisher.h"
#include "dabc/Configuration.h"
#include "dabc/Manager.h"

bool dabc::CreateManager(const std::string& name, int cmd_port)
{
   if (dabc::mgr.null()) {
      static dabc::Configuration cfg;

      new dabc::Manager(name, &cfg);

      // ensure that all submitted events are processed
      dabc::mgr.SyncWorker();

      dabc::mgr.Execute("InitFactories");
   }

   if ((cmd_port>=0) && dabc::mgr.GetCommandChannel().null())
      dabc::mgr()->CreateControl(cmd_port > 0, cmd_port);

   return true;
}

bool dabc::DestroyManager()
{
   if (dabc::mgr.null()) return true;

   dabc::mgr()->HaltManager();

   dabc::mgr.Destroy();

   return true;
}

std::string dabc::MakeNodeName(const std::string& arg)
{
   size_t pos = arg.find("dabc://");
   if (pos==std::string::npos)
      return std::string("dabc://") + arg;
   if (pos == 0) return arg;

   return std::string();
}



bool dabc::ConnectDabcNode(const std::string& nodeaddr)
{
   if (dabc::mgr.null()) {
      EOUT("Manager was not created");
      return false;
   }

   std::string fullname = MakeNodeName(nodeaddr);
   if (fullname.empty()) {
      EOUT("Wrong address format %d", nodeaddr.c_str());
      return false;
   }

   if (dabc::mgr.GetCommandChannel().null())
      dabc::mgr()->CreateControl(false);

   dabc::Command cmd("Ping");
   cmd.SetReceiver(fullname);
   cmd.SetTimeout(10);

   if (dabc::mgr.GetCommandChannel().Execute(cmd) != dabc::cmd_true) {
      EOUT("FAIL connection to %s", nodeaddr.c_str());
      return false;
   }

   return true;
}

dabc::Hierarchy dabc::GetNodeHierarchy(const std::string& nodeaddr)
{
   dabc::Hierarchy res;

   std::string fullname = MakeNodeName(nodeaddr);
   if (fullname.empty()) {
      EOUT("Wrong address format %d", nodeaddr.c_str());
      return res;
   }

   dabc::Command cmd("GetGlobalNamesList");
   cmd.SetReceiver(fullname + dabc::Publisher::DfltName());
   cmd.SetTimeout(10);

   if (dabc::mgr.GetCommandChannel().Execute(cmd)!=dabc::cmd_true) {
      EOUT("Fail to get hierarchy from node %s", nodeaddr.c_str());
      return res;
   }

   dabc::Buffer buf = cmd.GetRawData();

   if (buf.null()) return res;

   // DOUT0("Get raw data %p %u", buf.SegmentPtr(), buf.GetTotalSize());
   if (!res.ReadFromBuffer(buf)) {
      EOUT("Error decoding hierarchy data from buffer");
      res.Release();
   }

   return res;
}

