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

#ifndef VERBS_Device
#define VERBS_Device

#ifndef DABC_Device
#include "dabc/Device.h"
#endif

#ifndef VERBS_Context
#include "verbs/Context.h"
#endif

namespace verbs {

   class OpenSM;
   class QueuePair;
   class ComplQueue;
   class Device;

   class Transport;
   class Thread;
   class ProtocolAddon;

   extern const char* xmlMcastAddr;

   // ____________________________________________________________________

   /** \brief %Device for VERBS */

   class Device : public dabc::Device {
      friend class Transport;
      friend class Thread;
      friend class ProtocolAddon;

      protected:

         ContextRef  fIbContext;

      public:

         Device(const std::string& name);
         virtual ~Device();

         ContextRef IbContext() { return fIbContext; }

         dabc::ThreadRef MakeThread(const char* name, bool force = false);

         bool IsAllocateIndividualCQ() const { return fAllocateIndividualCQ; }
         void SetAllocateIndividualCQ(bool on) { fAllocateIndividualCQ = on; }

         virtual const char* ClassName() const { return verbs::typeDevice; }

         static bool IsThreadSafeVerbs() { return fThreadSafeVerbs; }

      protected:

         virtual int ExecuteCommand(dabc::Command cmd);

         QueuePair* CreatePortQP(const std::string& thrd_name, dabc::Reference port, int conn_type,
                                 dabc::ThreadRef &thrd);

         virtual dabc::Transport* CreateTransport(dabc::Command cmd, const dabc::Reference& port);

         virtual double ProcessTimeout(double last_diff);

         int HandleManagerConnectionRequest(dabc::Command cmd);

         virtual void ObjectCleanup();

         bool fAllocateIndividualCQ; // for connections individual CQ will be used

         static bool fThreadSafeVerbs;  // identifies if verbs is thread safe
   };

   // _________________________________________________________

   /** \brief %Refernce on \ref verbs::Device */

   class DeviceRef : public dabc::DeviceRef {

      DABC_REFERENCE(DeviceRef, dabc::DeviceRef, verbs::Device)

   };

   extern bool ConvertStrToGid(const std::string& s, ibv_gid &gid);
   extern std::string ConvertGidToStr(ibv_gid &gid);
}

#endif
