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

         DataInput         *fInput{nullptr};             //!< input object
         bool               fInputOwner{false};          //!< if true, fInput object must be destroyed
         EInputStates       fInpState{inpInit};          //!< state of transport
         Buffer             fCurrentBuf;                 //!< currently used buffer
         unsigned           fNextDataSize{0};            //!< indicate that input has data, but there is no buffer of required size
         unsigned           fPoolChangeCounter{0};       //!<
         MemoryPoolRef      fPoolRef;
         unsigned           fExtraBufs{0};               //!< number of extra buffers provided to the transport addon
         bool               fActivateWorkaround{false};  //!< special flag for hadaq transport
         std::string        fReconnect;                  //!< when specified, tried to reconnect
         bool               fStopRequested{false};       //!< if true transport will be stopped when next suitable state is achieved


         /** Method can be used in custom transport to start pool monitoring */
         void RequestPoolMonitoring();

         bool StartTransport() override;
         bool StopTransport() override;

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

         void TransportCleanup() override;

         bool ProcessSend(unsigned port) override;

         bool ProcessBuffer(unsigned pool) override;

         void ProcessTimerEvent(unsigned timer) override;

         int ExecuteCommand(Command cmd) override;

      public:

         InputTransport(dabc::Command cmd, const PortRef& inpport, DataInput *inp = nullptr, bool owner = false);
         virtual ~InputTransport();

         /** Assign input object, set addon if exists */
         void SetDataInput(DataInput* inp, bool owner);

         /** Set URL, use to reconnect data input */
         void EnableReconnect(const std::string &reconn);

         // in implementation user can get informed when something changed in the memory pool
         virtual void ProcessPoolChanged(MemoryPool */* pool */) {}

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

         bool StartTransport() override;
         bool StopTransport() override;

         void TransportCleanup() override;

         void ProcessEvent(const EventId&) override;

         bool ProcessRecv(unsigned port) override;
         void ProcessTimerEvent(unsigned) override;

         int ExecuteCommand(dabc::Command cmd) override;

      public:

         OutputTransport(dabc::Command cmd, const PortRef& outport, DataOutput* out, bool owner);
         virtual ~OutputTransport();

         void Write_CallBack(unsigned arg) { FireEvent(evCallBack, arg); }
   };

};

#endif
