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
#ifndef DABC_DataTransport
#define DABC_DataTransport

#ifndef DABC_Basic
#include "dabc/Basic.h"
#endif

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_Device
#include "dabc/Device.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

#ifndef DABC_MemoryPool
#include "dabc/MemoryPool.h"
#endif

#ifndef DABC_WorkingProcessor
#include "dabc/WorkingProcessor.h"
#endif

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

namespace dabc {

   class Buffer;
   class MemoryPool;

   class DataTransport : public Transport,
                         public WorkingProcessor,
                         protected MemoryPoolRequester {

      enum EDataEvents { evDataInput = 1, evDataOutput };

      protected:

         // this is interface methods,
         // which must be reimplemented in derived classes

         // meaning of the next methods are the same as from DataInput/DataOutput classes
         virtual unsigned Read_Size() { return di_RepeatTimeOut; }
         // In addition to DataInput, can returns:
         //    di_CallBack      - read must be confirmed by Read_CallBack
         virtual unsigned Read_Start(Buffer* buf) { return di_Ok; }
         virtual unsigned Read_Complete(Buffer* buf) { return di_EndOfStream; }

         // Defines timeout for operation
         virtual double Read_Timeout() { return 0.1; }

         // This method MUST be called by transport, when Read_Start returns di_CallBack
         // It is only way to "restart" event loop in the transport
         void Read_CallBack(unsigned compl_res = di_Ok);

         virtual bool WriteBuffer(Buffer* buf) { return false; }
         virtual void FlushOutput() {}

         // these are extra methods for handling inputs/outputs
         virtual void CloseInput() {}
         virtual void CloseOutput() {}

         virtual void ProcessPoolChanged(MemoryPool* pool) {}

      public:
         DataTransport(Device* dev, Port* port, bool doiunput = true, bool dooutput = false);
         virtual ~DataTransport();

         virtual bool ProvidesInput() { return fDoInput; }
         virtual bool ProvidesOutput() { return fDoOutput; }

         virtual bool Recv(Buffer* &buf);
         virtual unsigned RecvQueueSize() const;
         virtual Buffer* RecvBuffer(unsigned indx) const;
         virtual bool Send(Buffer* mem);
         virtual unsigned SendQueueSize();
         virtual unsigned MaxSendSegments() { return 9999; }
      protected:

         enum EInputStates { inpWorking, inpReady, inpBegin, inpNeedBuffer, inpPrepare, inpError, inpClosed };

         virtual void PortChanged();

         virtual void ProcessEvent(EventId);

         virtual bool ProcessPoolRequest();

         virtual double ProcessTimeout(double);

         double ProcessInputEvent(bool norm_call = true);
         void ProcessOutputEvent();

         virtual void StartTransport();
         virtual void StopTransport();

         virtual int ExecuteCommand(Command* cmd);

         Mutex              fMutex;
         BuffersQueue       fInpQueue;
         BuffersQueue       fOutQueue;
         bool               fActive;
         MemoryPool        *fPool;
         bool               fDoInput;
         bool               fDoOutput;
         EInputStates       fInpState;
         bool               fInpLoopActive; // indicate if loop around inp is active means inp event or timeout should appear soon
         unsigned           fNextDataSize; // indicate that input has data, but there is no buffer of required size
         Buffer*            fCurrentBuf;   // currently used buffer
         unsigned           fComplRes;     // result, assigned to completion operation
         unsigned           fPoolChangeCounter;
   };

};

#endif
