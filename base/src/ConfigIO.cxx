#include "dabc/ConfigIO.h"

#include "dabc/string.h"

#include <fnmatch.h>


bool dabc::ConfigIO::CreateIntAttr(const char* name, int value)
{
   return CreateAttr(name, FORMAT(("%d", value)));
}

bool dabc::ConfigIO::CheckAttr(const char* name, const char* value)
{
   const char* res = GetAttrValue(name);

   if (IsExact()) return res == 0 ? false : strcmp(res, value) == 0;

   if (res==0) return true;

   if (strcmp(res, value) == 0) return true;

   return fnmatch(res, value, FNM_NOESCAPE)==0;
}

bool dabc::ConfigIO::CheckIntAttr(const char* name, int value)
{
   return CheckAttr(name, FORMAT(("%d", value)));
}
