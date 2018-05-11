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

#ifndef DABC_ModuleAsync
#define DABC_ModuleAsync

#ifndef DABC_Module
#include "dabc/Module.h"
#endif

namespace dabc {

   /** \brief Base class for user-derived code, implementing event-processing.
     *
     * \ingroup dabc_user_classes
     * \ingroup dabc_all_classes
     *
     */

   class ModuleAsync : public Module {
      private:
         /** Activate module [internal].
          * Generates appropriate number of events for each input, output and pool handle */
         virtual bool DoStart();

         /** Generic event processing method [internal].
          * Here all events are processed and delivered to the user.
          * Made private to exclude possibility to redefine it */
         virtual void ProcessItemEvent(ModuleItem* item, uint16_t evid);

      protected:
         /** Constructor of ModuleAsync class.
          * Made protected to exclude possibility to create instance of class itself */
         ModuleAsync(const std::string &name, Command cmd = nullptr) : Module(name, cmd) { }

         /** Destructor of ModuleAsync class. */
         virtual ~ModuleAsync();

         /** Method return true if send over specified port can be done */
         bool CanSend(unsigned indx = 0) const
            { return indx < fOutputs.size() ? fOutputs[indx]->CanSend() : false; }

         /** Method return number of send operation can be done for specified port.
          * It is number of free entries in the port output queue.
          * Actual number of send operations could be bigger, while buffers can be immediately
          * transported further */
         unsigned NumCanSend(unsigned indx = 0) const
            { return indx < fOutputs.size() ? fOutputs[indx]->NumCanSend() : false; }

         /** Produces event for specified output port.
          * Should be used when processing was stopped due to return false in ProcessSend method */
         void ActivateOutput(unsigned port = 0);

         /** \brief Methods to send buffer via specified output port */
         bool Send(unsigned indx, Buffer& buf)
         {
            if (indx < fOutputs.size())
               return fOutputs[indx]->Send(buf);
            buf.Release();
            return false;
         }

         /** \brief Send buffer over first output port */
         inline bool Send(Buffer& buf)
           { return Send(0, buf); }

         /** \brief Method return true if recv from specified port can be done */
         bool CanRecv(unsigned indx = 0) const
            { return indx < fInputs.size() ? fInputs[indx]->CanRecv() : false; }

         /** \brief Method return number of buffers which can be received via the port */
         unsigned NumCanRecv(unsigned indx = 0) const
           { return indx < fInputs.size() ? fInputs[indx]->NumCanRecv() : 0; }

         /** \brief Methods receives buffers from the port */
         Buffer Recv(unsigned indx = 0)
            { return indx < fInputs.size() ? fInputs[indx]->Recv() : Buffer(); }

         /** \brief Returns true if receive queue is full and block input */
         bool RecvQueueFull(unsigned port = 0);

         /** \brief Let input signal only when queue is full */
         void SignalRecvWhenFull(unsigned port = 0);

         /** \brief Produces event for specified input port
          * Should be used when processing was stopped due to return false in ProcessRecv method */
         void ActivateInput(unsigned port = 0);

         /** \brief Returns buffer from receive queue of the input port */
         Buffer RecvQueueItem(unsigned port = 0, unsigned nbuf = 0);

         /** \brief Remove items in input queue*/
         bool SkipInputBuffers(unsigned indx = 0, unsigned nbuf = 1)
         { return indx < fInputs.size() ? fInputs[indx]->SkipBuffers(nbuf) : false; }

         /** \brief Returns true if memory pool can provide buffer */
         bool CanTakeBuffer(unsigned pool = 0)
         { return pool < fPools.size() ? fPools[pool]->CanTakeBuffer() : false; }

         /** Method return number of buffers which could be taken from data queue assigned with the pool.
          * These buffers already provided by the pool therefore directly available */
         unsigned NumCanTake(unsigned pool = 0) const
            { return pool < fPools.size() ? fPools[pool]->NumCanTake() : 0; }

         /** \brief Take buffer from memory pool */
         Buffer TakeBuffer(unsigned pool = 0)
         { return (pool < fPools.size()) ? fPools[pool]->TakeBuffer() : ((pool==0) ? TakeDfltBuffer() : Buffer()); }

         /** \brief Returns buffer from queue assigned with the pool */
         Buffer PoolQueueItem(unsigned pool = 0, unsigned nbuf = 0);

         /** \brief Produces event for specified pool handle.
          * \details Should be used when processing was stopped due to return false in ProcessBuffer method */
         void ActivatePool(unsigned pool);

         // here is a list of methods,
         // which can be reimplemented in user code

         // Level 1: Or one can redefine one or several following methods to
         // react on specific events only

         /** \brief Method called by framework when input event is produced.
          * \details Can be reimplemented by the user.
          * Depending from port configuration it can happen for every new buffer
          * in input queue or only when after previous event was processed */
         virtual void ProcessInputEvent(unsigned port);


         /** \brief Method called by framework when output event is produced.
           * \details Can be reimplemented by the user. */
         virtual void ProcessOutputEvent(unsigned port);


         /** \brief Method called by framework when pool event is produced.
           * \details Can be reimplemented by the user. */
         virtual void ProcessPoolEvent(unsigned pool);

         /** \brief Method called by framework when connection state of the item is changed.
           * \details Can be reimplemented by the user.
           * Called when input/output port or pool handle are connected or disconnected diregard
           * of running state of the module */
         virtual void ProcessConnectEvent(const std::string &name, bool on) {}

         /** \brief Method called by framework when timer event is produced.
           * \details Can be reimplemented by the user. */
         virtual void ProcessTimerEvent(unsigned timer) {}

         /** \brief Method called by framework when custom user event is produced.
           * \details Can be reimplemented by the user. */
         virtual void ProcessUserEvent(unsigned item) {}


         // Level 2: One could process each buffer
         // at this level user gets process calls for each new buffer

         /** \brief Method called by framework when at least one buffer available in input port.
          * \details Can be reimplemented by the user.
          * Method should return true is user want method to be called again for next buffer in input port */
         virtual bool ProcessRecv(unsigned port = 0) { return false; }

         /** \brief Method called by framework when at least one buffer can be send to output port.
          * \details Can be reimplemented by the user.
          * Method should return true is user want method to be called again for next possibility to send buffer */
         virtual bool ProcessSend(unsigned port = 0) { return false; }

         /** \brief Method called by framework when at least one buffer available in pool handle.
          * \details Can be reimplemented by the user.
          * Method should return true is user want method to be called again for next buffer in pool handle */
         virtual bool ProcessBuffer(unsigned pool = 0) { return false; }

      public:

         /** \brief Returns class name of the object */
         virtual const char* ClassName() const { return "ModuleAsync"; }

   };

   // ____________________________________________________________

   /** \brief %Reference on \ref dabc::ModuleAsync class
    *
    * \ingroup dabc_all_classes
    */

   class ModuleAsyncRef : public ModuleRef {
      DABC_REFERENCE(ModuleAsyncRef, ModuleRef, ModuleAsync)
   };

};

#endif
