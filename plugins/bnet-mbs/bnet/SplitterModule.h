#ifndef DABC_SplitterModule
#define DABC_SplitterModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif


namespace dabc {

   class SplitterModule : public ModuleAsync {
   protected:


   public:
      SplitterModule(const char* name, Command cmd = 0);

      virtual void ProcessItemEvent(dabc::ModuleItem*, uint16_t);

   };


}



#endif
