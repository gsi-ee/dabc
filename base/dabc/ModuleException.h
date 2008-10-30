#ifndef DABC_ModuleException
#define DABC_ModuleException

#ifndef DABC_Exception
#include "dabc/Exception.h"
#endif

namespace dabc {
   
   class Module;

   class ModuleException : public Exception {
      public: 
         ModuleException(Module* m, const char* info = "") throw ();
         Module* GetModule() const throw() { return fModule; }
      
      protected:
         Module* fModule;
   };
}


#endif
