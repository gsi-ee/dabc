#include "dabc/string.h"

#include <stdarg.h>

void dabc::formats(std::string& sbuf, const char *fmt, ...)
{
   if (!fmt || (strlen(fmt)==0)) return;

   va_list args;
   va_start(args, fmt);

   int   result = -1, length = 256;
   char *buffer = 0;
   while (result<0) {
      if (buffer) delete [] buffer;
      buffer = new char [length + 1];
      memset(buffer, 0, length + 1);
      result = vsnprintf(buffer, length, fmt, args);
      length *= 2;
   }
   sbuf.assign(buffer);
   delete [] buffer;
   va_end(args);
}

std::string dabc::format(const char *fmt, ...)
{
   if (!fmt || (strlen(fmt)==0)) return std::string("");

   va_list args;
   va_start(args, fmt);

   int   result = -1, length = 256;
   char *buffer = 0;
   while (result<0) {
      if (buffer) delete [] buffer;
      buffer = new char [length + 1];
      memset(buffer, 0, length + 1);
      result = vsnprintf(buffer, length, fmt, args);
      length *= 2;
   }
   std::string s(buffer);
   delete [] buffer;
   va_end(args);
   return s;
}
