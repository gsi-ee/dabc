#ifndef DABC_Configuration
#define DABC_Configuration

#ifndef DABC_ConfigBase
#include "dabc/ConfigBase.h"
#endif

namespace dabc {

   class Configuration : public ConfigBase {
      protected:
         XMLNodePointer_t  fSelected; // selected context node

         bool XDAQ_LoadLibs();
         bool XDAQ_ReadPars();
      public:
         Configuration(const char* fname);
         ~Configuration();

         bool SelectContext(unsigned cfgid, unsigned nodeid, unsigned numnodes, const char* logfile = 0);

         bool LoadLibs();

         bool ReadPars();
   };


}

#endif
