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


#include <stdint.h>

namespace dabc {

   class Buffer;

   extern const unsigned AcknoledgeQueueLength;

   /** \brief Base class for transport implementations
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    */

   class Transport : public ModuleAsync {
      protected:
         enum ETransportState {
            stInit,         /** When created */
            stRunning,      /** When working */
            stStopped,      /** When stopped */
            stError         /** Transport failed and will be destroyed */
         };

         ETransportState fState;
         bool fIsInputTransport;
         bool fIsOutputTransport;
         std::string fTransportInfoName;
         double fTransportInfoInterval;
         TimeStamp fTransportInfoTm;

         ETransportState GetState() const { return fState; }

         bool isTransportRunning() const { return fState == stRunning; }
         bool isTransportError() const { return fState == stError; }

         /** Method called when module on other side is started */
         virtual void ProcessConnectionActivated(const std::string& name, bool on);

         virtual void ProcessConnectEvent(const std::string& name, bool on);

         virtual int ExecuteCommand(Command cmd);

         /** \brief Returns true when next info can be provided
          *  If info parameter was configured, one could use it regularly to provide information
          *  about transport */
         bool InfoExpected() const;

         /** \brief Method provides transport info to specified parameter */
         void ProvideInfo(int lvl, const std::string& info);

         static std::string MakeName(const PortRef& inpport, const PortRef& outport);

      public:
         virtual const char* ClassName() const { return "Transport"; }

         Transport(dabc::Command cmd, const PortRef& inpport = 0, const PortRef& outport = 0);
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
