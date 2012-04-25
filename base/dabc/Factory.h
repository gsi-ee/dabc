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

#ifndef DABC_Factory
#define DABC_Factory

#ifndef DABC_Object
#include "dabc/Object.h"
#endif

#ifndef DABC_Queue
#include "dabc/Queue.h"
#endif

#ifndef DABC_Command
#include "dabc/Command.h"
#endif

#ifndef DABC_threads
#include "dabc/threads.h"
#endif

#include <vector>

namespace dabc {

   class Module;
   class FileIO;
   class DataInput;
   class DataOutput;
   class Command;
   class Device;
   class Thread;
   class Application;
   class Configuration;
   class Transport;
   class Port;
   class FactoryPlugin;

   /** \brief Factory class provides interfaces to add user-specific
    *    classes to the DABC code
    */

   class Factory : public Object {
      friend class Manager;
      friend class FactoryPlugin;

      struct LibEntry {
         void*        fLib;
         std::string  fLibName;

         LibEntry() : fLib(0), fLibName() {}
         LibEntry(void* lib, const std::string& name) : fLib(lib), fLibName(name) {}
         LibEntry(const LibEntry& src) : fLib(src.fLib), fLibName(src.fLibName) {}
      };

      static Mutex* LibsMutex()
      {
         static Mutex m;
         return &m;
      }

      static std::vector<LibEntry> fLibs;

      public:
         Factory(const char* name);

         virtual Application* CreateApplication(const char* classname, Command cmd) { return 0; }

         virtual Reference CreateObject(const char* classname, const char* objname, Command cmd) { return 0; }

         virtual Device* CreateDevice(const char* classname, const char* devname, Command cmd) { return 0; }

         virtual Reference CreateThread(Reference parent, const char* classname, const char* thrdname, const char* thrddev, Command cmd) { return Reference(); }

         virtual Module* CreateModule(const char* classname, const char* modulename, Command cmd) { return 0; }

         virtual Transport* CreateTransport(Reference port, const char* typ, Command cmd);

         virtual FileIO* CreateFileIO(const char* typ, const char* name, int option) { return 0; }

         virtual Object* ListMatchFiles(const char* typ, const char* filemask) { return 0; }

         virtual DataInput* CreateDataInput(const char* typ) { return 0; }

         virtual DataOutput* CreateDataOutput(const char* typ) { return 0; }

         static bool LoadLibrary(const std::string& fname);

         static void* FindSymbol(const std::string& symbol);

         static bool CreateManager(const std::string& name = "mgr", Configuration* cfg = 0);

      protected:

         /** Method called by the manager during application start.
          * One can put arbitrary initialization code here - for instance, create some control instances */
         virtual void Initialize() {}
   };


   // ==============================================================================

   class FactoryPlugin {
      friend class Manager;

      public:
         FactoryPlugin(dabc::Factory* f);
         ~FactoryPlugin();

      private:
         static PointersQueue<Factory> *Factories()
         {
            static PointersQueue<Factory> f(16);
            return &f;
         }
         static Mutex* FactoriesMutex()
         {
            static Mutex m;
            return &m;
         }

         static Factory* NextNewFactory();
   };

}

#endif
