#ifndef MBS_Factory
#define MBS_Factory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

namespace mbs {
    
   class Factory : public dabc::Factory {
      public:  
         Factory(const char* name) : dabc::Factory(name) {}
         
         static const char* FileType() { return "MbsFile"; }
         static const char* NewFileType() { return "MbsNewFile"; }
         static const char* NewTransportType() { return "MbsNewTransport"; }
         static const char* LmdFileType() { return "LmdFile"; }
            
         virtual dabc::Device* CreateDevice(dabc::Basic* parent, const char* classname, const char* devname, dabc::Command*);

         virtual dabc::DataInput* CreateDataInput(const char* typ, const char* name, dabc::Command* cmd = 0);

         virtual dabc::DataOutput* CreateDataOutput(const char* typ, const char* name, dabc::Command* cmd = 0);
      
      protected:
       
   };   
   
}

#endif
