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

#ifndef DABC_ModuleSync
#define DABC_ModuleSync

#ifndef DABC_Module
#include "dabc/Module.h"
#endif

/** Subclass from Module class
  * Allows to use explicit main loop of the module
  * Provides blocking operation (Recv, Send, TakeBuffer, ...) with timeouts.
  * If timeout is happening, either exception is produced or function returns with false results.
  * One can specify usage of timeout exception by SetTmoutExcept() method (default - false).
  * Another source of exceptions - disconnecting of module ports. Will be
  * produced when port, used in operation (Send or Recv), disconnected.
  * Execution of module main loop can be suspended by Stop() function and resumed again
  * by Start() function. This can only happens in some of ModuleSync methods like Send, TakeBuffer and so on.
  * In case when module is destroyed, MainLoop will be leaved by stop exception Exception::IsStop().
  * User can catch this exception, but MUST throw it again that module can be safely deleted afterwards.
  * Typical MainLoop of the ModuleSync should look like this:
  *     void MainLoop()
  *     {
  *        while (ModuleWorking()) {
  *          // do something
  *        }
  *     }
  * ModuleWorking() function tests if module should continue to work, and will execute commands,
  * submitted to module. To execute commands immediately, synchron modus must be
  * specified by SetSyncCommands(true) call.
  * In case when main loop contains no any methods like Recv or Send, it will
  * consume 100% of CPU resources, therefore one better should use
  * WaitEvent() call for waiting on any events which may happen in module.
  */

namespace dabc {

   class ModuleSync : public Module {
      private:
         bool fTmoutExcept;
         bool fDisconnectExcept;
         bool fSyncCommands;
         CommandsQueue* fNewCommands;

         ModuleItem*   fWaitItem;
         uint16_t      fWaitId;
         bool          fWaitRes;

         bool          fInsideMainLoop;  //!< flag indicates if main loop is executing

         virtual void DoWorkerMainLoop();
         virtual void DoWorkerAfterMainLoop();

         bool WaitItemEvent(double& tmout, ModuleItem* item = 0, uint16_t *resevid = 0, ModuleItem** resitem = 0)
            throw (Exception);

         void AsyncProcessCommands();

         virtual int PreviewCommand(Command cmd);

         virtual void ProcessItemEvent(ModuleItem* item, uint16_t evid);

      protected:

         /** Call this method from main loop if one want suspend of module
           * execution until new Start of the module is called from outside */
         void StopUntilRestart();

         virtual void ObjectCleanup();

//         enum ELoopStatus { stInit, stRun, stSuspend };

         ModuleSync(const std::string& name, Command cmd = 0);

         virtual ~ModuleSync();

         bool ModuleWorking(double timeout = 0.) throw (Exception);

         virtual void MainLoop() = 0;

         uint16_t WaitEvent(double timeout = -1) throw (Exception);
         bool WaitConnect(const std::string& name, double timeout = -1) throw (Exception);
         bool WaitInput(unsigned indx, unsigned minqueuesize = 1, double timeout = -1) throw (Exception);

         /** Method return true if recv from specified port can be done */
         bool CanRecv(unsigned indx = 0) const
            { return indx < fInputs.size() ? fInputs[indx]->CanRecv() : false; }
         Buffer Recv(unsigned indx = 0, double timeout = -1) throw (Exception);
         bool Send(unsigned indx, Buffer& buf, double timeout = -1) throw (Exception);
         bool Send(Buffer& buf, double timeout = -1) throw (Exception)
            { return Send(0, buf, timeout); }

         Buffer RecvFromAny(unsigned* port = 0, double timeout = -1) throw (Exception);

         Buffer TakeBuffer(unsigned pool = 0, double timeout = -1) throw (Exception);

         void SetTmoutExcept(bool on = true) { fTmoutExcept = on; }
         bool IsTmoutExcept() const { return fTmoutExcept; }

         void SetDisconnectExcept(bool on = true) { fDisconnectExcept = on; }
         bool IsDisconnectExcept() const { return fDisconnectExcept; }

         void SetSyncCommands(bool on = true) { fSyncCommands = on; }
         bool IsSyncCommands() const { return fSyncCommands; }

      public:

         virtual const char* ClassName() const { return "ModuleSync"; }


   };
}

#endif
