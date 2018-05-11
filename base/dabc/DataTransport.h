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

   /** \brief Base class for input transport implementations
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    */


   class CmdDataInputClosed : public Command {
      DABC_COMMAND(CmdDataInputClosed, "DataInputClosed");
   };

   class CmdDataInputFailed : public Command {
      DABC_COMMAND(CmdDataInputFailed, "DataInputFailed");
   };


   class InputTransport : public Transport {

      // enum EDataEvents { evCallBack = evntModuleLast };

      enum EInputStates {
         inpInit,
         inpInitTimeout,   // waiting timeout read_size
         inpBegin,
         inpSizeCallBack,  // wait for call-back with buffer size
         inpCheckSize,     // in this state one should check return size argument
         inpNeedBuffer,
         inpWaitBuffer,    // in such state we are waiting for the buffer be delivered
         inpCheckBuffer,   // here size of buffer will be checked
         inpHasBuffer,     // buffer is ready for use
         inpCallBack,      // in this mode transport waits for call-back
         inpCompleting,    // one need to complete operation
         inpComplitTimeout,// waiting timeout after Read_Complete
         inpReady,
         inpError,
         inpEnd,           // at such state we need to generate EOF buffer and close input
         inpReconnect,     // reconnection state - transport tries to recreate input object
         inpClosed
      };

      protected:

         DataInput         *fInput;             //!< input object
         bool               fInputOwner;        //!< if true, fInput object must be destroyed
         EInputStates       fInpState;          //!< state of transport
         Buffer             fCurrentBuf;        //!< currently used buffer
         unsigned           fNextDataSize;      //!< indicate that input has data, but there is no buffer of required size
         unsigned           fPoolChangeCounter; //!<
         MemoryPoolRef      fPoolRef;
         unsigned           fExtraBufs;         //!< number of extra buffers provided to the transport addon
         bool               fActivateWorkaround;  //!< special flag for hadaq transport
         std::string        fReconnect;         //!< when specified, tried to reconnect
         bool               fStopRequested;     //!< if true transport will be stopped when next suitable state is achieved


         /** Method can be used in custom transport to start pool monitoring */
         void RequestPoolMonitoring();

         virtual bool StartTransport();
         virtual bool StopTransport();

         void ChangeState(EInputStates state);

         /** Returns true if state consider to be suitable to stop transport */
         bool SuitableStateForStartStop() {
            return (fInpState == inpInit) ||
                   (fInpState == inpBegin) ||
                   (fInpState == inpReady) ||
                   (fInpState == inpError) ||
                   (fInpState == inpClosed);
         }

         void CloseInput();

         virtual void TransportCleanup();

         virtual bool ProcessSend(unsigned port);

         virtual bool ProcessBuffer(unsigned pool);

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(Command cmd);

      public:

         InputTransport(dabc::Command cmd, const PortRef& inpport, DataInput* inp = 0, bool owner = false);
         virtual ~InputTransport();

         /** Assign input object, set addon if exists */
         void SetDataInput(DataInput* inp, bool owner);

         /** Set URL, use to reconnect data input */
         void EnableReconnect(const std::string &reconn);

         // in implementation user can get informed when something changed in the memory pool
         virtual void ProcessPoolChanged(MemoryPool* pool) {}

         // This method MUST be called by transport, when Read_Start returns di_CallBack
         // It is only way to "restart" event loop in the transport
         void Read_CallBack(unsigned compl_res = di_Ok);
   };

// ======================================================================================

   /** \brief Base class for output transport implementations
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    */

   class OutputTransport : public Transport {

      enum EDataEvents { evCallBack = evntModuleLast };

      enum EOutputStates {
         outReady,          // initial state, next buffer can be written
         outInitTimeout,   // state when timeout should be completed before next check can be done
         outWaitCallback,  // when waiting callback to inform when writing can be started
         outStartWriting,  // we can apply buffer for start writing
         outWaitFinishCallback, // when waiting when buffer writing is finished
         outFinishWriting,
         outError,
         outClosing,       // closing transport
         outClosed,
         outRetry          // waiting for retry
      };

      protected:

         DataOutput*     fOutput;
         bool            fOutputOwner;

         EOutputStates   fOutState;
         Buffer          fCurrentBuf;     //!< currently used buffer
         bool            fStopRequested;  //!< if true transport will be stopped when next suitable state is achieved
         double          fRetryPeriod;    //!< if retry option enabled, transport will try to reinit output

         void SetDataOutput(DataOutput* out, bool owner);

         void CloseOutput();

         /** Returns true if state consider to be suitable to stop transport */
         bool SuitableStateForStartStop()
         {
            return (fOutState == outReady) ||
                   (fOutState == outError) ||
                   (fOutState == outClosed);
         }

         /** Returns state in string form */
         std::string StateAsStr() const
         {
            switch (fOutState) {
               case outReady: return "Ready";
               case outInitTimeout: return "InitTimeout";
               case outWaitCallback: return "WaitCallback";
               case outStartWriting: return "StartWriting";
               case outWaitFinishCallback: return "WaitFinishCallback";
               case outFinishWriting: return "FinishWriting";
               case outError: return "Error";
               case outClosing: return "Closing";
               case outClosed: return "Closed";
               case outRetry: return "Retry";
            }
            return "undefined";
         }

         void ChangeState(EOutputStates state);

         void CloseOnError();

         virtual bool StartTransport();
         virtual bool StopTransport();

         virtual void TransportCleanup();

         virtual void ProcessEvent(const EventId&);

         virtual bool ProcessRecv(unsigned port);
         virtual void ProcessTimerEvent(unsigned);

         virtual int ExecuteCommand(dabc::Command cmd);

      public:

         OutputTransport(dabc::Command cmd, const PortRef& outport, DataOutput* out, bool owner);
         virtual ~OutputTransport();

         void Write_CallBack(unsigned arg) { FireEvent(evCallBack, arg); }
   };

};

#endif
