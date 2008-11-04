#ifndef DABC_AplicationFactory
#define DABC_AplicationFactory

#ifndef DABC_Basic
#include "dabc/Basic.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

#ifndef DABC_threads
#include "dabc/threads.h"
#endif


namespace dabc {

   class Module;
   class FileIO;
   class DataInput;
   class DataOutput;
   class Command;
   class Device;
   class Folder;
   class WorkingThread;
   class Application;

   class Factory : public Basic {
      friend class Manager;

      public:
         Factory(const char* name);

         virtual Application* CreateApplication(Basic* parent, const char* classname, const char* appname, Command* cmd) { return 0; }

         virtual Device* CreateDevice(Basic* parent, const char* classname, const char* devname, Command* cmd) { return 0; }

         virtual WorkingThread* CreateThread(Basic* parent, const char* classname, const char* thrdname, const char* thrddev, Command* cmd) { return 0; }

         virtual Module* CreateModule(const char* classname, const char* modulename, Command* cmd) { return 0; }

         virtual FileIO* CreateFileIO(const char* typ, const char* name, int option) { return 0; }

         virtual Folder* ListMatchFiles(const char* typ, const char* filemask) { return 0; }

         virtual DataInput* CreateDataInput(const char* typ, const char* name, Command* cmd = 0) { return 0; }

         virtual DataOutput* CreateDataOutput(const char* typ, const char* name, Command* cmd = 0) { return 0; }

      protected:
         static const char* DfltAppClass(const char* newdefltclass = 0);

      private:
         static Queue<Factory*> *Factories()
         {
            static Queue<Factory*> f(16, true);
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
