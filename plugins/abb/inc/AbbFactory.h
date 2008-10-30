#ifndef DABC_AbbFactory
#define DABC_AbbFactory

#include "dabc/Factory.h"
#include "dabc/PCIBoardCommands.h"

// define parameter name strings at one place:
#define ABB_NAME_PLUGIN     "AbbFactory"

#define ABB_PAR_DEVICE     	 "ABB_Device_Name"
#define ABB_PAR_MODULE     	 "ABB_ReadoutModule_Name"
#define ABB_PAR_BOARDNUM     "ABB_BoardNumber"
#define ABB_PAR_BAR     	 "ABB_ReadoutBAR" 
#define ABB_PAR_ADDRESS      "ABB_ReadoutAddress"
#define ABB_PAR_LENGTH     	 "ABB_ReadoutLength"

namespace dabc {

class AbbDevice;

class AbbFactory: public dabc::Factory  {
   public: 
      
      dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command* cmd); 
  
      virtual dabc::Device* CreateDevice(dabc::Basic* parent, const char* classname, const char* devname, dabc::Command* com);
 
    private:
         AbbFactory();     
         
    static AbbFactory gAbbFactory;
 
};

}// namespace

#endif

