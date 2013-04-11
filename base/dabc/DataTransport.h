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

#ifndef DABC_DataTransport
#define DABC_DataTransport

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#ifndef DABC_Device
#include "dabc/Device.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

namespace dabc {

   class InputTransport : public Transport {

      // enum EDataEvents { evCallBack = evntModuleLast };

      enum EInputStates {
         inpInit,
         inpInitTimeout, // waiting timeout read_size
         inpBegin,
         inpSizeCallBack, // wait for call-back with buffer size
         inpCheckSize,    // in this state one should check return size argument
         inpNeedBuffer,
         inpWaitBuffer,  // in such state we are waiting for the buffer be delivered
         inpCheckBuffer, // here size of buffer will be checked
         inpHasBuffer,   // buffer is ready for use
         inpCallBack,    // in this mode transport waits for call-back
         inpCompliting,  // one need to complete operation
         inpComplitTimeout, // waiting timeout after Read_Complete
         inpReady,
         inpError,
         inpEnd,         // at such state we need to generate EOF buffer and close input
         inpClosed
      };

      protected:

         DataInput         *fInput;
         bool               fInputOwner; // if true, fInput object must be destroyed
         EInputStates       fInpState;
         Buffer             fCurrentBuf;   // currently used buffer
         unsigned           fNextDataSize;    // indicate that input has data, but there is no buffer of required size
         unsigned           fPoolChangeCounter;
         MemoryPoolRef      fPoolRef;
         unsigned           fExtraBufs;       // number of extra buffers provided to the transport addon


         void RequestPoolMonitoring();

         virtual bool StartTransport();
         virtual bool StopTransport();

         void CloseInput();

         virtual void ObjectCleanup();

         virtual void ProcessEvent(const EventId&);

         virtual bool ProcessSend(unsigned port);

         virtual bool ProcessBuffer(unsigned pool);
         virtual void ProcessTimerEvent(unsigned timer);



      public:

         InputTransport(dabc::Command cmd, const PortRef& inpport, DataInput* inp, bool owner, WorkerAddon* addon = 0);
         virtual ~InputTransport();

         // in implementation user can get informed when something changed in the memory pool
         virtual void ProcessPoolChanged(MemoryPool* pool) {}


         // This method MUST be called by transport, when Read_Start returns di_CallBack
         // It is only way to "restart" event loop in the transport
         void Read_CallBack(unsigned compl_res = di_Ok);
   };

// ======================================================================================


   class OutputTransport : public Transport {

      enum EDataEvents { evCallBack = evntModuleLast };

      enum EOutputStates {
         outInit,
         outInitTimeout,   // state when timeout should be completed before next check can be done
         outWaitCallback,  // when waiting callback to inform when writing can be started
         outStartWriting,  // we can apply buffer for start writing
         outWaitFinishCallback, // when waiting when buffer writing is finished
         outFinishWriting,
         outError,
         outClosing,   // closing transport
         outClosed
      };

      protected:

         DataOutput*        fOutput;
         bool               fOutputOwner;

         EOutputStates      fState;
         Buffer             fCurrentBuf;   // currently used buffer

         void CloseOutput();

         virtual bool StartTransport();
         virtual bool StopTransport();

         virtual void ObjectCleanup();

         virtual void ProcessEvent(const EventId&);

         virtual bool ProcessRecv(unsigned port);
         virtual void ProcessTimerEvent(unsigned timer);

      public:

         OutputTransport(dabc::Command cmd, const PortRef& outport, DataOutput* out, bool owner, WorkerAddon* addon = 0);
         virtual ~OutputTransport();

         void Write_CallBack(unsigned arg) { FireEvent(evCallBack, arg); }
   };

};

#endif
