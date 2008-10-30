#include "dabc/ModuleAsync.h"

#include "dabc/logging.h"
#include "dabc/ModuleItem.h"
#include "dabc/PoolHandle.h"

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
