/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "dabc/string.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

void dabc::formats(std::string& sbuf, const char *fmt, ...)
{
   if (!fmt || (strlen(fmt)==0)) return;

   va_list args;
   int length(256), result(0);
   char *buffer(0);
   while (1) {
      if (buffer) delete [] buffer;
      buffer = new char [length];

      va_start(args, fmt);
      result = vsnprintf(buffer, length, fmt, args);
      va_end(args);

      if (result<0) length *= 2; else                 // this is error handling in glibc 2.0
      if (result>=length) length = result + 1; else   // this is error handling in glibc 2.1
      break;
   }
   sbuf.assign(buffer);
   delete [] buffer;
}

std::string dabc::format(const char *fmt, ...)
{
   if (!fmt || (strlen(fmt)==0)) return std::string("");

   va_list args;
   int length(256), result(0);
   char *buffer(0);
   while (1) {
      if (buffer) delete [] buffer;
      buffer = new char [length];

      va_start(args, fmt);
      result = vsnprintf(buffer, length, fmt, args);
      va_end(args);

      if (result<0) length *= 2; else                 // this is error handling in glibc 2.0
      if (result>=length) length = result + 1; else   // this is error handling in glibc 2.1
      break;
   }

   std::string s(buffer);
   delete [] buffer;
   va_end(args);
   return s;
}
