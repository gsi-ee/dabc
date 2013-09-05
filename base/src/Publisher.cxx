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

#include "dabc/Publisher.h"

#include "dabc/Manager.h"

dabc::Publisher::Publisher(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fHierarchy(),
   fList(),
   fCnt(0)
{
   fHierarchy.Create("Publisher");

   CreateTimer("Timer", 0.5);

   DOUT0("Create publisher");
}

void dabc::Publisher::ProcessTimerEvent(unsigned timer)
{
   for (EntriesList::iterator iter = fList.begin(); iter != fList.end(); iter++) {

      CmdPublisher cmd;
      cmd.SetReceiver(iter->worker);
      cmd.SetVersion(iter->version);
      cmd.SetUInt("recid", iter->id);

      dabc::mgr.Submit(Assign(cmd));

      // DOUT0("Submit command to worker %s id %u", iter->worker.c_str(), iter->id);
   }
}

bool dabc::Publisher::ReplyCommand(Command cmd)
{
   if (cmd.IsName(CmdPublisher::CmdName())) {
      CmdPublisher cmdp = cmd;

      dabc::Buffer diff = cmdp.GetRes();
      uint64_t v = cmdp.GetVersion();
      unsigned id = cmd.GetUInt("recid");

      //DOUT0("Get Reply from recid %u version:%u", id, (unsigned) v);

      for (EntriesList::iterator iter = fList.begin(); iter != fList.end(); iter++) {
         if (iter->id == id) {
            dabc::Hierarchy top = fHierarchy.GetFolder(iter->path);
            iter->version = v;
            top.UpdateFromBuffer(diff);

            //DOUT0("Apply diff for folder %s now\n%s", iter->path.c_str(), fHierarchy.SaveToXml().c_str());

            break;
         }
      }

      return true;
   }

   return dabc::ModuleAsync::ReplyCommand(cmd);
}


int dabc::Publisher::ExecuteCommand(Command cmd)
{
   if (cmd.IsName("Register")) {
      fList.push_back(PublisherEntry());

      std::string path = cmd.GetStr("Path");
      std::string worker = cmd.GetStr("Worker");

      for (EntriesList::iterator iter = fList.begin(); iter != fList.end(); iter++) {
         if (iter->path == path) {
            EOUT("Path %s already registered!!!", path.c_str());
            return cmd_false;
         }
      }

      dabc::Hierarchy h = fHierarchy.GetFolder(path);
      if (!h.null()) {
         EOUT("Path %s already present in the hierarchy", path.c_str());
         return cmd_false;
      }

      fList.back().id = fCnt++;
      fList.back().path = path;
      fList.back().worker = worker;

      fHierarchy.GetFolder(path, true);

      return cmd_true;
   } else
   if (cmd.IsName("Unregister")) {
      std::string path = cmd.GetStr("Path");
      std::string worker = cmd.GetStr("Worker");

      bool find = false;

      for (EntriesList::iterator iter = fList.begin(); iter != fList.end(); iter++) {
         if ((iter->path == path) && (iter->worker == worker)) {
            fList.erase(iter);
            find = true;
            break;
         }
      }

      dabc::Hierarchy h = fHierarchy.GetFolder(path);
      if (h.null()) { find = false; EOUT("No entry in the hierarchy"); }

      if (find) {

         dabc::Hierarchy prnt = h.GetParentRef();
         h.Destroy();
         while (!prnt.null() && (prnt != fHierarchy)) {
            h = prnt;
            prnt = prnt.GetParentRef();
            if (h.NumChilds() == 0) h.Destroy(); // delete as long no any other involved
         }

      } else {
         EOUT("Not found entry with path %s", path.c_str());
      }

      return cmd_bool(find);
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

