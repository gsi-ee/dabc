#ifndef MBS_Device
#define MBS_Device

#ifndef DABC_GenericDevice
#include "dabc/Device.h"
#endif

namespace mbs {

   class Device : public dabc::Device {
      public:
         Device(Basic* parent, const char* name = "MBS");

         virtual const char* RequiredThrdClass() const { return "SocketThread"; }

         virtual int CreateTransport(dabc::Command* cmd, dabc::Port* port);

         virtual const char* ClassName() const { return "mbs::Device"; }
   };

};

#endif
