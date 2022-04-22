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

#ifndef DABC_Transport
#define DABC_Transport

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_Device
#include "dabc/Device.h"
#endif

namespace dabc {

   extern const unsigned AcknoledgeQueueLength;

   /** \brief Base class for transport implementations
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    */

   class Transport : public ModuleAsync {

      friend class Device;

      protected:
         enum ETransportState {
            stInit,         /** When created */
            stRunning,      /** When working */
            stStopped,      /** When stopped */
            stError         /** Transport failed and will be destroyed */
         };

         DeviceRef        fTransportDevice;    //!< device, used to create transport
         ETransportState  fTransportState{stInit};
         bool             fIsInputTransport{false};
         bool             fIsOutputTransport{false};
         std::string      fTransportInfoName;
         double           fTransportInfoInterval{0};
         TimeStamp        fTransportInfoTm;

         ETransportState GetTransportState() const { return fTransportState; }

         bool isTransportRunning() const { return GetTransportState() == stRunning; }
         bool isTransportError() const { return GetTransportState() == stError; }

         /** Method called when module on other side is started */
         void ProcessConnectionActivated(const std::string &name, bool on) override;

         void ProcessConnectEvent(const std::string &name, bool on) override;

         int ExecuteCommand(Command cmd) override;

         /** \brief Returns true when next info can be provided
          *  If info parameter was configured, one could use it regularly to provide information
          *  about transport */
         bool InfoExpected() const;

         /** Reimplemented method from module */
         void ModuleCleanup() override;

         virtual void TransportCleanup() {}

         /** \brief Method provides transport info to specified parameter */
         void ProvideInfo(int lvl, const std::string &info);

         static std::string MakeName(const PortRef& inpport, const PortRef& outport);

      public:
         const char* ClassName() const override { return "Transport"; }

         Transport(dabc::Command cmd, const PortRef& inpport = nullptr, const PortRef& outport = nullptr);
         virtual ~Transport();

         /** Methods activated by Port, when transport starts/stops. */
         virtual bool StartTransport();
         virtual bool StopTransport();

         bool IsInputTransport() const { return fIsInputTransport; }
         bool IsOutputTransport() const { return fIsOutputTransport; }

         // method must be called from the transport thread
         virtual void CloseTransport(bool witherr = false);
   };

   // __________________________________________________


   /** \brief %Reference on \ref dabc::Transport class
    *
    * \ingroup dabc_all_classes
    */

   class TransportRef : public ModuleAsyncRef {
      DABC_REFERENCE(TransportRef, ModuleAsyncRef, Transport)

      PortRef OutputPort() { return (GetObject() && GetObject()->IsInputTransport()) ? FindPort("Output") : PortRef(); }

      PortRef InputPort() { return (GetObject() && GetObject()->IsOutputTransport()) ? FindPort("Input") : PortRef(); }
   };

};

#endif
