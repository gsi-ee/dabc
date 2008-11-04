#include "dabc/Factory.h"

#include "dabc/Manager.h"

const char* dabc::Factory::DfltAppClass(const char* newdefltclass)
{
   static dabc::String dflt = "UserApplication";
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
