#ifndef MBS_MbsDevice
#define MBS_MbsDevice

#ifndef DABC_GenericDevice
#include "dabc/Device.h"
#endif

namespace mbs {
    
   class MbsDevice : public dabc::Device {
      public:
         MbsDevice(Basic* parent, const char* name = "MBS");
         
         virtual const char* RequiredThrdClass() const { return "SocketThread"; }
         
         virtual int CreateTransport(dabc::Command* cmd, dabc::Port* port);

         virtual const char* ClassName() const { return "MbsDevice"; }
   };
   
};

#endif
