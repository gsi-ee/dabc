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


namespace dabc {

   /** \brief Base class for user-derived code, implementing main loop.
     *
     * \ingroup dabc_user_classes
     *
     * Allows to use explicit main loop of the module
     * Provides blocking operation (Recv, Send, TakeBuffer, ...) with timeouts.
     * If timeout is happening, either exception is produced or function returns with false result.
     * One can specify usage of timeout exception by SetTmoutExcept() method (default - false).
     * Another source of exceptions - disconnecting of module ports. Will be
     * produced when port, used in operation (Send or Recv), disconnected.
     * Execution of module main loop can be suspended by Stop() function and resumed again
     * by Start() function. This can only happens in some of ModuleSync methods like Send, TakeBuffer and so on.
     * In case when module is destroyed, MainLoop will be leaved by stop exception Exception::IsStop().
     * User can catch this exception, but MUST throw it again that module can be safely deleted afterwards.
     * Typical MainLoop of the ModuleSync should look like this:
     *
     * ~~~~~~~~~~~~~~~{.c}
     * void MainLoop()
     * {
     *    while (ModuleWorking()) {
     *      // do something
     *    }
     * }
     * ~~~~~~~~~~~~~~~
     *
     * ModuleWorking() function tests if module should continue to work, and will execute commands,
     * submitted to module. To execute commands immediately, synchron modus must be
     * specified by SetSyncCommands(true) call.
     * In case when main loop contains no any methods like Recv or Send, it will
     * consume 100% of CPU resources, therefore one better should use
     * WaitEvent() call for waiting on any events which may happen in module.
     */


   class ModuleSync : public Module {
      private:
         /** Flag indicates if timeout exception should be generated. */
         bool           fTmoutExcept;

         /** Flag indicates if disconnect exception should be generated. */
         bool           fDisconnectExcept;

         /** Flag indicates if commands will be executed synchronously.
          * When true, all commands submitted to the module, executed inside ModuleWorking() method  */
         bool           fSyncCommands;

         /** List of commands, submitted to the module */
         CommandsQueue* fNewCommands;

         /** Current item, waiting for the event.
          * It should be input/output port or pool handle. */
         ModuleItem*   fWaitItem;

         /** Internal variable, used in wait event */
         uint16_t      fWaitId;

         /** Result of event waiting */
         bool          fWaitRes;

         /** Flag indicates if main loop is executing */
         bool          fInsideMainLoop;

         /** Internal - entrance function for main loop execution. */
         virtual void DoWorkerMainLoop();

         /** Internal - function executed after leaving main loop. */
         virtual void DoWorkerAfterMainLoop();

         /* Internal - central method for blocking functionality.
          * Block main loop and waits for dedicated event.
          * Keep processing of other events or commands.
          * Used in framework to rise exceptions when module should be stopped */
         bool WaitItemEvent(double& tmout,
                            ModuleItem* item = 0,
                            uint16_t *resevid = 0,
                            ModuleItem** resitem = 0);

         /** Internal - process commands which are submitted to sync queue */
         void AsyncProcessCommands();

         /** Internal - preview command before execution */
         virtual int PreviewCommand(Command cmd);

         /** Internal - central method of events processing */
         virtual void ProcessItemEvent(ModuleItem* item, uint16_t evid);

      protected:

         /** Call this method from main loop if one want suspend of module
           * execution until new Start of the module is called from outside */
         void StopUntilRestart();

         /** Internal DABC method. Called when module will be destroyed */
         virtual void ObjectCleanup();

         /** Normal constructor */
         ModuleSync(const std::string& name, Command cmd = 0);

         /** Destructor */
         virtual ~ModuleSync();

         /** Returns true when module in running state.
          * Normally should be used like:
          * ~~~~~~~~~~~~~~~{.c}
          * void UserModule::MainLoop()
          * {
          *    while (ModuleWorking()) {
          *       Buffer buf = Recv();
          *       Send(buf);
          *    }
          * }
          * ~~~~~~~~~~~~~~~
          */
         bool ModuleWorking(double timeout = 0.);

         /** Main execution function of SyncModule.
          * Here user code **MUST** be implemented. */
         virtual void MainLoop() = 0;

         /** Waits for any event */
         uint16_t WaitEvent(double timeout = -1);

         /** Waits for connection for specified port */
         bool WaitConnect(const std::string& name, double timeout = -1);

         /** Waits until input port has specified number of buffers in the queue */
         bool WaitInput(unsigned indx, unsigned minqueuesize = 1, double timeout = -1);

         /** Method return true if receiving from specified port can be done */
         bool CanRecv(unsigned indx = 0) const
            { return indx < fInputs.size() ? fInputs[indx]->CanRecv() : false; }

         /** Receive buffer from input port.
          * Blocks until buffer is arrived.
          * If timeout value specified, can return without buffer.  */
         Buffer Recv(unsigned indx = 0, double timeout = -1);

         /** Send buffer via specified output port */
         bool Send(unsigned indx, Buffer& buf, double timeout = -1);

         /** Send buffer via first output port */
         bool Send(Buffer& buf, double timeout = -1)
            { return Send(0, buf, timeout); }

         /** Receive buffer from any input port.
          * One also can obtain index of port */
         Buffer RecvFromAny(unsigned* port = 0, double timeout = -1);

         /** Take buffer from memory pool */
         Buffer TakeBuffer(unsigned pool = 0, double timeout = -1);

         /** Set if exception should be generated when any operation is timeout */
         void SetTmoutExcept(bool on = true) { fTmoutExcept = on; }

         /** Returns true if timeout exception generation is configured */
         bool IsTmoutExcept() const { return fTmoutExcept; }

         /** Set if disconnect exception should be generated by module */
         void SetDisconnectExcept(bool on = true) { fDisconnectExcept = on; }

         /** Returns true if disconnect exception should be generated by module */
         bool IsDisconnectExcept() const { return fDisconnectExcept; }

         /** Set if commands should be executed synchronous in main loop */
         void SetSyncCommands(bool on = true) { fSyncCommands = on; }

         /** Returns true if commands should be executed synchronous in main loop */
         bool IsSyncCommands() const { return fSyncCommands; }

      public:

         /** Returns class name */
         virtual const char* ClassName() const { return "ModuleSync"; }

   };
}

#endif
