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
   class ProtocolWorker;

   extern const char* xmlMcastAddr;

   // ____________________________________________________________________

   class Device : public dabc::Device {
      friend class Transport;
      friend class Thread;
      friend class ProtocolWorker;

      protected:

         ContextRef  fIbContext;

      public:

         Device(const char* name);
         virtual ~Device();

         ContextRef IbContext() { return fIbContext; }

         dabc::ThreadRef MakeThread(const char* name, bool force = false);

         bool IsAllocateIndividualCQ() const { return fAllocateIndividualCQ; }
         void SetAllocateIndividualCQ(bool on) { fAllocateIndividualCQ = on; }

         virtual const char* ClassName() const { return "verbs::Device"; }

         static bool IsThreadSafeVerbs() { return fThreadSafeVerbs; }

      protected:

         virtual int ExecuteCommand(dabc::Command cmd);

         bool CreatePortQP(const char* thrd_name, dabc::Port* port, int conn_type,
                           dabc::ThreadRef &thrd, ComplQueue* &port_cq, QueuePair* &port_qp);
         void CreateVerbsTransport(dabc::ThreadRef thrd, dabc::Reference port, bool useackn, ComplQueue* cq, QueuePair* qp);

         virtual dabc::Transport* CreateTransport(dabc::Command cmd, dabc::Reference portref);
         
         virtual double ProcessTimeout(double last_diff);

         int HandleManagerConnectionRequest(dabc::Command cmd);
         
         virtual void ObjectCleanup();

         bool fAllocateIndividualCQ; // for connections individual CQ will be used
         
         static bool fThreadSafeVerbs;  // identifies if verbs is thread safe
   };

   class DeviceRef : public dabc::DeviceRef {

      DABC_REFERENCE(DeviceRef, dabc::DeviceRef, verbs::Device)

   };

   extern bool ConvertStrToGid(const std::string& s, ibv_gid &gid);
   extern std::string ConvertGidToStr(ibv_gid &gid);
}

#endif
