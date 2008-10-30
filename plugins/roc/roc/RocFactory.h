#ifndef ROC_RocFactory
#define ROC_RocFactory

#include "dabc/Factory.h"
#include "roc/RocCommands.h"

// define parameter name strings at one place:
#define DABC_ROC_NAME_PLUGIN     "RocFactory"

#define DABC_ROC_PAR_DEVICE       "ROC_Device_Name"
#define DABC_ROC_PAR_MODULE       "ROC_ReadoutModule_Name"

namespace roc {

class RocDevice;

class RocFactory: public dabc::Factory  {
   public: 
      
      dabc::Module* CreateModule(const char* classname, const char* modulename, dabc::Command* cmd); 
  
      virtual dabc::Device* CreateDevice(dabc::Basic* parent, const char* classname, const char* devname, dabc::Command* com);

      virtual dabc::DataOutput* CreateDataOutput(const char* typ, const char* name, dabc::Command* cmd = 0);
 
    private:
         RocFactory();     
         
    static RocFactory gRocFactory;
 
};

}// namespace

#endif

