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
         virtual bool DoStart();

      protected:
         ModuleAsync(const std::string& name, Command cmd = 0) : Module(name, cmd) { }
         virtual ~ModuleAsync();

         /** Method return true if send over specified port can be done */
         bool CanSend(unsigned indx = 0) const
            { return indx < fOutputs.size() ? fOutputs[indx]->CanSend() : false; }

         /** Method return true if send over specified port can be done */
         unsigned NumCanSend(unsigned indx = 0) const
            { return indx < fOutputs.size() ? fOutputs[indx]->NumCanSend() : false; }

         /** Methods to send buffer via output port */
         bool Send(unsigned indx, Buffer& buf)
         {
            if (indx < fOutputs.size())
               return fOutputs[indx]->Send(buf);
            buf.Release();
            return false;
         }

         inline bool Send(Buffer& buf)
           { return Send(0, buf); }

         /** Method return true if recv from specified port can be done */
         bool CanRecv(unsigned indx = 0) const
            { return indx < fInputs.size() ? fInputs[indx]->CanRecv() : false; }

         /** Method return number of buffers which can be received via the port */
         unsigned NumCanRecv(unsigned indx = 0) const
           { return indx < fInputs.size() ? fInputs[indx]->NumCanRecv() : 0; }

         /** Methods receives buffers from the port */
         Buffer Recv(unsigned indx = 0)
            { return indx < fInputs.size() ? fInputs[indx]->Recv() : Buffer(); }

         /** Returns true if recv queue is full and block input */
         bool RecvQueueFull(unsigned port = 0);

         /** Let input signal only when queue is full */
         void SignalRecvWhenFull(unsigned port = 0);

         /** Returns buffer from recv queue */
         Buffer RecvQueueItem(unsigned port = 0, unsigned nbuf = 0);

         /** Remove items in input queue*/
         bool SkipInputBuffers(unsigned indx = 0, unsigned nbuf = 1)
         { return indx < fInputs.size() ? fInputs[indx]->SkipBuffers(nbuf) : false; }


         bool CanTakeBuffer(unsigned indx = 0)
         { return indx < fPools.size() ? fPools[indx]->CanTakeBuffer() : false; }

         Buffer TakeBuffer(unsigned indx = 0)
         { return (indx < fPools.size()) ? fPools[indx]->TakeBuffer() : Buffer(); }

         Buffer TakeEmpty(unsigned indx = 0)
         { return (indx < fPools.size()) ? fPools[indx]->TakeEmpty() : Buffer(); }


         // here is a list of methods,
         // which can be reimplemented in user code

         // Level 0: Either generic event processing function must be reimplemented
         virtual void ProcessItemEvent(ModuleItem* item, uint16_t evid);

         // Level 1: Or one can redefine one or several following methods to
         // react on specific events only
         virtual void ProcessInputEvent(unsigned port = 0);
         virtual void ProcessOutputEvent(unsigned port = 0);
         virtual void ProcessPoolEvent(unsigned pool = 0);
         virtual void ProcessConnectEvent(const std::string& name, bool on) {}
         virtual void ProcessTimerEvent(unsigned timer = 0) {}
         virtual void ProcessUserEvent(unsigned item = 0) {}

         // Level 2: One could process each buffer
         virtual bool ProcessRecv(unsigned port = 0) { return false; }
         virtual bool ProcessSend(unsigned port = 0) { return false; }
         virtual bool ProcessBuffer(unsigned pool = 0) { return false; }

      public:

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
