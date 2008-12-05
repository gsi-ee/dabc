#ifndef DABC_ModuleAsync
#define DABC_ModuleAsync

#ifndef DABC_Module
#include "dabc/Module.h"
#endif

namespace dabc {

   class ModuleAsync : public Module {
      public:
         ModuleAsync(const char* name, Command* cmd = 0) : Module(name, cmd) { }

         virtual ~ModuleAsync();

         // here is a list of methods,
         // which can be reimplemented in user code

         // Either generic event processing function must be reimplemented
         virtual void ProcessUserEvent(ModuleItem* item, uint16_t evid);

         // Or one can redefine one or several following methods to
         // react on specific events only
         virtual void ProcessInputEvent(Port* port) {}
         virtual void ProcessOutputEvent(Port* port) {}
         virtual void ProcessPoolEvent(PoolHandle* pool);
         virtual void ProcessTimerEvent(Timer* timer) {}
         virtual void ProcessConnectEvent(Port* port) {}
         virtual void ProcessDisconnectEvent(Port* port) {}

   };

};

#endif
