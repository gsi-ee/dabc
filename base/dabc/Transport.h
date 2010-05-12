/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#ifndef DABC_Transport
#define DABC_Transport

#ifndef DABC_threads
#include "dabc/threads.h"
#endif

#include <stdint.h>

namespace dabc {

   class Buffer;
   class Port;
   class Device;
   class MemoryPool;

   extern const unsigned AcknoledgeQueueLength;

   class Transport {

      friend class Port;
      friend class Device;

      enum ETransportState {
            stCreated,                 /** After transport created */
            stDoingAttach,             /** Transport call port->AssignTransport and now waits for assignment */
            stAssigned,                /** Normal working mode for transport */
            stDoingDettach,            /** This is time when transport want to remove port pointer and waits reply from port */
            stNeedCleanup,             /** State which requires cleanup of the transport, should be performed from device */
            stDoingCleanup,            /** Indicates that CleanupTransport is running */
            stDidCleanup               /** Final state, transport can be halted and destroyed */
      };

      protected:
         Mutex           fPortMutex;
         Port*           fPort;
         Device*         fDevice;
         ETransportState fTransportState; // this status used together with port mutex only
         bool            fTransportErrorFlag;     // set when error is detected

         Transport(Device* dev);
         void FireInput();
         void FireOutput();

         virtual void PortChanged() {}

         /** Methods called from Port to inform, that module starts/stops data processing.
           * One should care that call come from other thread that transport has */
         virtual void StartTransport() {}
         virtual void StopTransport() {}

         /** Method is used to asynchronously destroy transport instance */
         virtual void HaltTransport();

         /** Method called at the moment when port pointer is set to 0
          * Normally called from the Port thread. */
         virtual void CleanupTransport() {}

         /** Method called from port thread to set fPort pointer
          * If return false, command rejected and therefore transport pointer should not be used by port
          * Normally method initiated via AttachPort/DettachPort calls.
          * /param  called_by_port indicates if called from port thread (default) */
         bool SetPort(Port* port, bool called_by_port = true);

         /** Method called by device to check if transport can be destroyed,
          * if /paran force specified, one should tries to dettach port in any cases */
         bool CanCleanupTransport(bool force);

         /** Method called to complete transport cleanup (if necessary) before
          * transport will be halted and destroyed */
         void MakeTranportCleanup();


         MemoryPool* GetPortPool();

         static long gNumTransports;

         // call this method if you want to normally close transport
         void CloseTransport() { DettachPort(); }

         bool IsTransportErrorFlag() const { return fTransportErrorFlag; }

         // this method should be called if error is detected
         // and transport object must be cleaned up and destroyed
         virtual void ErrorCloseTransport();

         virtual int GetParameter(const char* name) { return 0; }

      public:
         virtual ~Transport();

         Device* GetDevice() const { return fDevice; }

         /** This should be called after transport constructor to
          * associate transport with port. Runs asynchronousely */
         bool AttachPort(Port* port, bool sync = false);

         void DettachPort();
         bool IsDoingCleanup();

         bool IsPortAssigned() const;

         virtual bool ProvidesInput() { return false; }
         virtual bool ProvidesOutput() { return false; }

         virtual bool Recv(Buffer* &buf) = 0;
         virtual unsigned  RecvQueueSize() const = 0;
         virtual Buffer* RecvBuffer(unsigned indx) const = 0;
         virtual bool Send(Buffer* buf) = 0;
         virtual unsigned SendQueueSize() = 0;
         virtual unsigned MaxSendSegments() = 0;

         static long NumTransports() { return gNumTransports; }
   };
};

#endif
