#include "dabc/ModuleException.h"

dabc::ModuleException::ModuleException(Module* m, const char* info) throw () : 
   dabc::Exception(info),
   fModule(m)
{
}
