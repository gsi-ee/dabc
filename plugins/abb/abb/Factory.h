#ifndef ABB_Factory
#define ABB_Factory

#include "dabc/Factory.h"
#include "pci/BoardCommands.h"

// define parameter name strings at one place:
#define ABB_PAR_DEVICE   	 "ABB_Device_Name"
#define ABB_PAR_MODULE    	 "ABB_ReadoutModule_Name"
#define ABB_PAR_BOARDNUM    "ABB_BoardNumber"
#define ABB_PAR_BAR     	 "ABB_ReadoutBAR"
#define ABB_PAR_ADDRESS     "ABB_ReadoutAddress"
#define ABB_PAR_LENGTH   	 "ABB_ReadoutLength"

namespace abb {

   class Factory: public dabc::Factory  {
      public:

         Factory(const char* name) : dabc::Factory(name) {}

         virtual dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command* cmd);

         virtual dabc::Device* CreateDevice(const char* classname, const char* devname, dabc::Command* com);
   };

}// namespace

#endif

