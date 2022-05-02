#include "hadaq/HadaqTypeDefs.h"

static int cnt = 0;

extern "C" bool filter_func(hadaq::RawEvent *evnt)
{
   (void) evnt;

   if (cnt++ < 10) printf("Did filtering\n");

   return true;
}
