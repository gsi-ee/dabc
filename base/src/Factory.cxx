#include "dabc/Factory.h"

#include "dabc/Manager.h"

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

dabc::Factory::Factory(const char* name) :
   Basic(0, name)
{
   DOUT0(("Factory %s is created", name));

   if (Manager::Instance())
      Manager::Instance()->AddFactory(this);
   else {
      DOUT3(("Remember factory %s", name));
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
