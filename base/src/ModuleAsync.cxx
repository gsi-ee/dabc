#include "dabc/ModuleAsync.h"

#include "dabc/logging.h"
#include "dabc/ModuleItem.h"
#include "dabc/PoolHandle.h"
#include "dabc/Port.h"

dabc::ModuleAsync::~ModuleAsync()
{
   // call stop before ModuleAsync specific data will be destroyed
   Stop();
}


void dabc::ModuleAsync::ProcessUserEvent(ModuleItem* item, uint16_t evid)
{
    switch (evid) {
       case evntInput:
          ProcessInputEvent((Port*) item);
          break;
       case evntOutput:
          ProcessOutputEvent((Port*) item);
          break;
       case evntPool:
          ProcessPoolEvent((PoolHandle*) item);
          break;
       case evntTimeout:
          ProcessTimerEvent((Timer*)item);
          break;
       case evntPortConnect:
          ProcessConnectEvent((Port*) item);
          break;
       case evntPortDisconnect:
          ProcessDisconnectEvent((Port*) item);
          break;
    }
}

void dabc::ModuleAsync::ProcessPoolEvent(PoolHandle* pool)
{
   // this is default implementation

   Buffer* buf = pool->TakeRequestedBuffer();
   dabc::Buffer::Release(buf);
}

void dabc::ModuleAsync::OnThreadAssigned()
{
   dabc::Module::OnThreadAssigned();

   // produce initial output events,
   // that user can fill output queue via processing of output event
   for (unsigned n=0;n<NumOutputs();n++) {
      Port* port = Output(n);
      unsigned len = port->OutputQueueCapacity() - port->OutputPending();
      while (len--)
         GetUserEvent(port, evntOutput);
   }
}

