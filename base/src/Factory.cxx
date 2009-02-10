#include "dabc/Factory.h"

#include <dlfcn.h>

#include "dabc/Manager.h"
#include "dabc/Port.h"
#include "dabc/DataIO.h"
#include "dabc/DataIOTransport.h"


std::vector<dabc::Factory::LibEntry> dabc::Factory::fLibs;

const char* dabc::Factory::DfltAppClass(const char* newdefltclass)
{
   static std::string dflt = "UserApplication";
   if (newdefltclass!=0) dflt = newdefltclass;
   return dflt.c_str();
}

dabc::Factory* dabc::Factory::NextNewFactory()
{
   dabc::LockGuard lock(FactoriesMutex());
   return Factories()->Size()>0 ? Factories()->Pop() : 0;
}

bool dabc::Factory::LoadLibrary(const std::string& fname)
{
   void* lib = dlopen(fname.c_str(), RTLD_LAZY | RTLD_GLOBAL);

   if (lib==0) {
      EOUT(("Cannot load library %s err:%s", fname.c_str(), dlerror()));
      return false;
   }

   DOUT1(("Library loaded %s", fname.c_str()));

   dabc::LockGuard lock(FactoriesMutex());

   fLibs.push_back(LibEntry(lib, fname));

   return true;
}

void* dabc::Factory::FindSymbol(const char* symbol)
{
   dabc::LockGuard lock(FactoriesMutex());

   for (unsigned n=0;n<fLibs.size();n++) {
      void* res = dlsym(fLibs[n].fLib, symbol);
      if (res!=0) {
         DOUT3(("Found symbol %s in library %s", symbol, fLibs[n].fLibName.c_str()));
         return res;
      }
   }

   return 0;
}

dabc::Factory::Factory(const char* name) :
   Basic(0, name)
{
   DOUT1(("Factory %s is created", name));

   if (Manager::Instance())
      Manager::Instance()->AddFactory(this);
   else {
      dabc::LockGuard lock(FactoriesMutex());
      Factories()->Push(this);
   }
}

bool dabc::Factory::CreateManager(const char* kind, Configuration* cfg)
{
   if ((Factories()==0) || (cfg==0)) return false;

   if (dabc::mgr() != 0) {
      EOUT(("Manager instance already exists"));
      return false;
   }

   for (unsigned n=0;n<Factories()->Size();n++)
      if (Factories()->Item(n)->CreateManagerInstance(kind, cfg)) return true;

   return false;
}

dabc::Transport* dabc::Factory::CreateTransport(dabc::Port* port, const char* typ, const char* thrdname, dabc::Command* cmd)
{
   dabc::DataInput* inp = CreateDataInput(typ);
   if ((inp!=0) && !inp->Read_Init(cmd, port)) {
      EOUT(("Input object %s cannot be initialized", typ));
      delete inp;
      inp = 0;
   }

   dabc::DataOutput* out = CreateDataOutput(typ);
   if ((out!=0) && !out->Write_Init(cmd, port)) {
      EOUT(("Output object %s cannot be initialized", typ));
      delete out;
      out = 0;
   }

   if ((inp==0) && (out==0)) return 0;

   Device* dev = dabc::mgr()->FindLocalDevice();
   DataIOTransport* tr = new DataIOTransport(dev, port, inp, out);

   if ((thrdname==0) || (strlen(thrdname)==0))
      thrdname = port->ProcessorThreadName();

   if (!dabc::mgr()->MakeThreadFor(tr, thrdname)) {
      EOUT(("Fail to create thread for transport"));
      delete tr;
      tr = 0;
   }

   return tr;
}
