#include "dabc_root/Factory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"

dabc_root::Factory mbsfactory("dabc_root");

dabc::DataInput* dabc_root::Factory::CreateDataInput(const char* typ, const char* name, dabc::Command* cmd)
{
   return 0;
}


dabc::DataOutput* dabc_root::Factory::CreateDataOutput(const char* typ, const char* name, dabc::Command* cmd)
{

   DOUT3(("dabc_root::Factory::CreateDataOutput typ:%s", typ));

   if (strcmp(typ, "RootTree")==0) {
      return 0;
   }

   return 0;
}
