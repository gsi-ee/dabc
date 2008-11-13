#include "dabc/ConfigIO.h"

#include "dabc/string.h"

bool dabc::ConfigIO::CreateIntAttr(const char* name, int value)
{
   return CreateAttr(name, FORMAT(("%d", value)));
}
