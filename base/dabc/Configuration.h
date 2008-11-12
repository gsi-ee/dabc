#ifndef DABC_Configuration
#define DABC_Configuration

#ifndef DABC_ConfigBase
#include "dabc/ConfigBase.h"
#endif

namespace dabc {

   class Configuration : public ConfigBase {
      public:
         Configuration(const char* fname);
         ~Configuration();

         bool LoadLibs(unsigned id) { return true; }

         bool ReadPars(unsigned id) { return true; }
   };


}

#endif
