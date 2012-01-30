/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#ifndef DABC_Device
#define DABC_Device

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

namespace dabc {

   class Port;
   class Transport;

   class Device : public Worker {
      friend class Transport;

      protected:
         enum EDeviceEvents { eventDeviceLast = evntFirstUser };

         Device(const char* name);

         // FIXME: device mutex should be normal object mutex - isn't it?
         Mutex* DeviceMutex() { return &fDeviceMutex; }

         Mutex           fDeviceMutex;

      public:
         virtual ~Device();

         virtual int ExecuteCommand(Command cmd);

         virtual Transport* CreateTransport(Command cmd, Reference port) { return 0; }

         virtual const char* ClassName() const { return "Device"; }
   };

   class DeviceRef : public WorkerRef {

      DABC_REFERENCE(DeviceRef, WorkerRef, Device)

   };

};

#endif
