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

#ifndef DABC_Publisher
#define DABC_Publisher

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#include <list>

namespace dabc {

   class CmdPublisher : public Command {

      DABC_COMMAND(CmdPublisher, "CmdPublisher");

      void SetVersion(uint64_t v) { SetUInt("version",v); }
      uint64_t GetVersion() { return GetUInt("version"); }

      void SetRes(Reference& res) { SetRef("Res", res); }
      Reference GetRes() { return GetRef("Res"); }
   };


   struct PublisherEntry {
      public:
         std::string path;     // absolute path in hierarchy
         std::string worker;   // worker which is manages hierarchy
         unsigned id;          // unique id in the worker
         uint64_t version;     // last requested version

         PublisherEntry() : path(), worker(), id(0), version(0) {}
   };


   /** \brief %Module manages published hierarchies and provide optimize access to them
    *
    * \ingroup dabc_all_classes
    */

   class Publisher : public dabc::ModuleAsync {
      protected:

         Hierarchy fHierarchy;

         typedef std::list<PublisherEntry> EntriesList;

         EntriesList fList;

         unsigned fCnt;

         virtual int ExecuteCommand(Command cmd);

         virtual bool ReplyCommand(Command cmd);

         virtual void ProcessTimerEvent(unsigned timer);

      public:


         Publisher(const std::string& name, dabc::Command cmd = 0);

         static const char* DfltName() { return "Publisher"; }
   };



   class PublisherRef : public ModuleAsyncRef {
      DABC_REFERENCE(PublisherRef, ModuleAsyncRef, Publisher)

      bool Register(const std::string& path, const std::string& workername)
      {
         if (null()) return false;

         Command cmd("Register");
         cmd.SetStr("Path", path);
         cmd.SetStr("Worker", workername);

         return Execute(cmd) == cmd_true;

      }

      bool Unregister(const std::string& path, const std::string& workername)
      {
         if (null()) return false;

         Command cmd("Unregister");
         cmd.SetStr("Path", path);
         cmd.SetStr("Worker", workername);

         return Execute(cmd) == cmd_true;
      }
   };

}

#endif
