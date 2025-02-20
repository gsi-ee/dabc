// $Id$

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

   class Transport;

   class DeviceRef;

   /** \brief Base class for device implementation
    *
    * \ingroup dabc_user_classes
    * \ingroup dabc_all_classes
    *
    *  Used when some transport-independent entity should be create and managed in the application.
    *  Device can be used to create transports for the port. One should specify in xml
    *  ~~~{.xml}
    *    <InputPort name="Input0" url="device://device_name"/>
    *  ~~~
    *  In such case transport will be created by Device::CreateTransport() method
    *  When device will be created, all related transports will be created as well
    */

   class Device : public Worker {

      friend class DeviceRef;

      protected:
         enum EDeviceEvents { eventDeviceLast = evntFirstUser };

         bool  fDeviceDestroyed;     //!<  if true, device and all transports should not be used

         Device(const std::string &name);

         bool Find(ConfigIO &) override;

         /** Returns device mutex - it is just object mutex */
         Mutex* DeviceMutex() const { return ObjectMutex(); }

         void ObjectCleanup() override;

      public:
         virtual ~Device();

         int ExecuteCommand(Command cmd) override;

         virtual Transport* CreateTransport(Command /* cmd */, const Reference & /* port */) { return nullptr; }

         const char *ClassName() const override { return dabc::typeDevice; }
   };


   /** \brief %Reference on \ref dabc::Device class
    *
    * \ingroup dabc_user_classes
    * \ingroup dabc_all_classes
    *
    */

   class DeviceRef : public WorkerRef {

      DABC_REFERENCE(DeviceRef, WorkerRef, Device)

      bool DeviceDestroyed() const { return null() ? false : GetObject()->fDeviceDestroyed; }
   };

};

#endif
