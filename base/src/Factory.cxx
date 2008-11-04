#include "dabc/Factory.h"

#include "dabc/Manager.h"

dabc::String dabc::Factory::fDfltAppClass = "UserApplication";

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
