// this file used to compile stream initialization scripts on the fly

#include "base/ProcMgr.h"

#include "hadaq/HldProcessor.h"
#include "hadaq/TrbProcessor.h"
#include "hadaq/TdcProcessor.h"

#include <stdio.h>

#include "first.C"

#ifdef _SECOND_
#include "second.C"
#endif

extern "C" void stream_engine() {

   printf("Calling first.C\n");
   first();

#ifdef _SECOND_
   printf("Calling second.C\n");
   second();
#endif
}
