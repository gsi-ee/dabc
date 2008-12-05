#ifndef DABC_ModuleSync
#define DABC_ModuleSync

#ifndef DABC_Module
#include "dabc/Module.h"
#endif

#ifndef DABC_Port
#include "dabc/Port.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

/** Subclass from Module class
  * Allows to use explicit main loop of the module
  * Provides blocking operation (Recv, Send, TakeBuffer, ...) with timeouts.
  * If timeout is happening, either exception is produced or function returnes with false results.
  * One can specify usage of timout exception by SetTmoutExcept() method (fefault - false).
  * Another source of exceptions - disconnecting of module ports. Will be
  * produced when port, used in operation (Send or Recv), disconnected.
  * Execution of module main loop can be suspended by Stop() function and resumed again
  * by Start() function. This can only happens in some of ModuleSync methods like Send, TakeBuffer and so on.
  * In case when module is destroyed, MainLoop will be leaved by StopException.
  * User can catch this excpetion, but MUST throw it again that module can be safely deleted afterwards.
  * Typicall MainLoop of the ModuleSync should look like this:
  *     void MainLoop()
  *     {
  *        while (TestWorking()) {
  *          // do something
  *        }
  *     }
  * TestWorking() function tests if module should continue to work, and will execute commands,
  * submitted to module. To execute commands immidiately, synchron modus must be
  * specified by SetSyncCommands(true) call.
  * In case when main loop containes no any methods like Recv or Send, it will
  * consume 100% of CPU resources, therefore one better should use
  * WaitEvent() call for waiting on any events wich may happen in module.
  */

namespace dabc {

   class ModuleSync : public Module {

      public:
         ModuleSync(const char* name, Command* cmd = 0);

         virtual ~ModuleSync();

         bool TestWorking(double timeout = 0.)
            throw (StopException, TimeoutException);

         virtual void MainLoop() = 0;

         uint16_t WaitEvent(double timeout = -1)
            throw (StopException, TimeoutException);
         bool WaitConnect(Port* port, double timeout = -1)
            throw (PortException, StopException, TimeoutException);
         Buffer* Recv(Port* port, double timeout = -1)
            throw (PortInputException, StopException, TimeoutException);
         Buffer* RecvFromAny(Port** port = 0, double timeout = -1)
            throw (PortInputException, StopException, TimeoutException);
         bool WaitInput(Port* port, unsigned minqueuesize = 1, double timeout = -1)
            throw (PortInputException, StopException, TimeoutException);
         Buffer* TakeBuffer(PoolHandle* pool, unsigned size, double timeout = -1)
            throw (StopException, TimeoutException);
         bool Send(Port* port, Buffer* buf, double timeout = -1)
            throw (PortOutputException, StopException, TimeoutException);
         bool Send(Port* port, BufferGuard& buf, double timeout = -1)
            throw (PortOutputException, StopException, TimeoutException);

         void SetTmoutExcept(bool on = true) { fTmoutExcept = on; }
         bool IsTmoutExcept() const { return fTmoutExcept; }

         void SetDisconnectExcept(bool on = true) { fDisconnectExcept = on; }
         bool IsDisconnectExcept() const { return fDisconnectExcept; }

         void SetSyncCommands(bool on = true) { fSyncCommands = on; }
         bool IsSyncCommands() const { return fSyncCommands; }

      protected:

         enum ELoopStatus { stInit, stRun, stSuspend };

         virtual void ProcessUserEvent(ModuleItem* item, uint16_t evid);

         /** Call this method from main loop if one want suspend of module
           * execution until new Start of the module is called from outside */
         void StopUntilRestart();

      private:

         virtual void DoProcessorMainLoop();
         virtual void DoProcessorAfterMainLoop();

         virtual int PreviewCommand(Command* cmd);

         bool WaitItemEvent(double& tmout, ModuleItem* item = 0, uint16_t *resevid = 0, ModuleItem** resitem = 0)
            throw (StopException, TimeoutException);

         void AsyncProcessCommands();

         bool fTmoutExcept;
         bool fDisconnectExcept;
         bool fSyncCommands;
         CommandsQueue fNewCommands;

         ModuleItem*  fWaitItem;
         uint16_t     fWaitId;
         bool         fWaitRes;

         ELoopStatus  fLoopStatus;

   };
}

#endif
