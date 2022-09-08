// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "dabc/string.h"

#include <cstring>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <clocale>

#include "dabc/ConfigBase.h"

void dabc::formats(std::string& sbuf, const char *fmt, ...)
{
   if (!fmt || (strlen(fmt) == 0)) return;

   va_list args;
   int length = 256, result = 0;
   char *buffer = nullptr;
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
   if (!fmt || (strlen(fmt) == 0)) return std::string("");

   va_list args;
   int length = 256, result = 0;
   char *buffer = nullptr;
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

std::string dabc::size_to_str(unsigned long sz, int prec, int select)
{
   if (select == 0) {
      unsigned long mult = prec<=0 ? 10 : 1;
      if (sz < 0x1000LU) select = 1; else
      if (sz < 0x100000LU*mult) select = 2; else
      if (sz < 0x40000000LU*mult) select =3; else select = 4;
   }

   switch (select) {
      case 1:
         return dabc::format("%lu B", sz);
      case 2:
         if (prec<=0)
            return dabc::format("%lu KB", sz/1024);
         else
            return dabc::format("%*.*f KB", prec+2, prec, sz/1024.);
      case 3:
         if (prec<=0)
            return dabc::format("%lu MB", sz/0x100000LU);
         else
            return dabc::format("%*.*f MB", prec+2, prec, sz/1024./1024.);
      case 4:
         if (prec<=0)
            return dabc::format("%lu GB", sz/0x40000000LU);
         else
            return dabc::format("%*.*f GB", prec+2, prec, sz/1024./1024./1024.);
   }

   return dabc::format("%lu B", sz);
}

std::string dabc::number_to_str(unsigned long num, int prec, int select)
{
   if (select == 0) {
      unsigned long mult = prec<=0 ? 10 : 1;
      if (num < 10000LU) select = 1; else
      if (num < 1000000LU*mult) select = 2; else
      if (num < 1000000000LU*mult) select = 3; else select = 4;
   }

   switch (select) {
      case 1:
         return dabc::format("%lu", num);
      case 2:
         if (prec<=0)
            return dabc::format("%luk", num/1000);
         else
            return dabc::format("%*.*fk", prec+2, prec, num/1e3);
      case 3:
         if (prec<=0)
            return dabc::format("%lum", num/1000000LU);
         else
            return dabc::format("%*.*fm", prec+2, prec, num/1e6);
      case 4:
         if (prec<=0)
            return dabc::format("%lug", num/1000000000LU);
         else
            return dabc::format("%*.*fg", prec+2, prec, num/1e9);
   }

   return dabc::format("%lu", num);

}



bool dabc::str_to_int(const char* val, int* res)
{
   if (!val || !res) return false;

   while (*val==' ') val++;
   if (*val == 0) return false;
   if ((strlen(val)>2) && (val[0]=='0') && ((val[1]=='x') || (val[1]=='X'))) {
      unsigned ures = 0;
      if (sscanf(val+2, "%x", &ures) == 1) {
         *res = ures;
         return true;
      }
      return false;
   }

   if (strpbrk(val,".,eE")) return false; // avoid any float value

   return sscanf(val, "%d", res) == 1;
}

bool dabc::str_to_lint(const char* val, long* res)
{
   if (!val || !res) return false;

   while (*val==' ') val++;
   if (*val == 0) return false;

   if ((strlen(val)>2) && (val[0]=='0') && ((val[1]=='x') || (val[1]=='X'))) {
      long unsigned ures = 0;
      if (sscanf(val+2, "%lx", &ures) == 1) {
         *res = ures;
         return true;
      }
      return false;
   }

   if (strpbrk(val,".,eE")) return false; // avoid any float value

   return sscanf(val, "%ld", res) == 1;
}


bool dabc::str_to_uint(const char* val, unsigned* res)
{
   if (!val || !res) return false;

   while (*val==' ') val++;
   if (*val == 0) return false;

   if ((strlen(val)>2) && (val[0]=='0') && ((val[1]=='x') || (val[1]=='X'))) {
      return sscanf(val+2, "%x", res) == 1;
   }

   if (strpbrk(val,".,eE")) return false; // avoid any float value

   return sscanf(val, "%u", res) == 1;
}

bool dabc::str_to_luint(const char* val, long unsigned* res)
{
   if (!val || !res) return false;

   while (*val==' ') val++;
   if (*val == 0) return false;

   if ((strlen(val)>2) && (val[0]=='0') && ((val[1]=='x') || (val[1]=='X'))) {
      return sscanf(val+2, "%lx", res) == 1;
   }

   if (strpbrk(val,".,eE")) return false; // avoid any float value

   return sscanf(val, "%lu", res) == 1;
}

bool dabc::str_to_double(const char* val, double* res)
{
   if (!val || !res) return false;

   if (sscanf(val, "%lf", res) == 1) return true;

   unsigned ures = 0;

   if (str_to_uint(val, &ures)) { *res = ures; return true; }

   return false;
}

bool dabc::str_to_bool(const char* val, bool* res)
{
   if (!val || !res) return false;
   if (strcmp(val, xmlTrueValue) == 0) { *res = true; return true; }
   if (strcmp(val, xmlFalseValue) == 0) { *res = false; return true; }
   return false;
}

std::string dabc::replace_all(const std::string &str, const std::string &match, const std::string &replace)
{
   if (match.empty()) return str;

   std::string res = str;
   size_t last = 0, pos = 0;

   while ((pos = res.find(match, last)) != std::string::npos) {
      res.erase(pos, match.length());
      res.insert(pos, replace);
      last = pos + replace.length();
   }

   return res;
}

dabc::NumericLocale::NumericLocale() : fPrev()
{
   char *loc = setlocale(LC_NUMERIC, nullptr);
   if (loc && (strcmp(loc, "C") != 0)) {
      fPrev = loc;
      setlocale(LC_NUMERIC, "C");
   }
}

dabc::NumericLocale::~NumericLocale()
{
   if (!fPrev.empty())
      setlocale(LC_NUMERIC, fPrev.c_str());
}
