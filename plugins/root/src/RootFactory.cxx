#include "dabc/RootFactory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"

dabc::RootFactory mbsfactory("root");

dabc::DataInput* dabc::RootFactory::CreateDataInput(const char* typ, const char* name, dabc::Command* cmd)
{
   return 0;
}


dabc::DataOutput* dabc::RootFactory::CreateDataOutput(const char* typ, const char* name, dabc::Command* cmd)
{
    
   DOUT3(("RootFactory::CreateDataOutput typ:%s", typ)); 

   if (strcmp(typ, "RootTree")==0) {
      return 0; 
   } 

   return 0;
}
