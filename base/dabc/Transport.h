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

      enum TransportStatus { stCreated, stAssigned, stNeedCleanup, stDoingCleanup };

      protected:
         Mutex           fPortMutex;
         Port*           fPort;
         Device*         fDevice;
         TransportStatus fTransportStatus; // this status used together with port mutex only
         bool            fErrorState;      // set when error is detected

         Transport(Device* dev);
         void FireInput();
         void FireOutput();

         virtual void PortChanged() {}

         /** Methods called from Port to inform, that module starts/stops data processing.
           * One should care that call come from other thread that transport has */
         virtual void StartTransport() {}
         virtual void StopTransport() {}

         virtual void DoTransportHalt() {}


         MemoryPool* GetPortPool();

         static long gNumTransports;

         virtual bool _IsReadyForCleanup();

         // call this method if you want to normally close transport
         void CloseTransport() { DettachPort(); }

         bool IsErrorState() const { return fErrorState; }

         // this method should be called if error is detected
         // and transport object must be cleaned up and destroyed
         virtual void ErrorCloseTransport();

         virtual int GetParameter(const char* name) { return 0; }

      public:
         virtual ~Transport();

         Device* GetDevice() const { return fDevice; }

         void AssignPort(Port* port);
         void DettachPort();
         bool CheckNeedCleanup(bool force);
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
