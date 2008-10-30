#include "dabc/string.h"

#include <stdarg.h>
/*

#include <string>

class dabc::String::stl_string : public std::string {};

dabc::String::String() : 
   fStr(0)
{
   fStr = new stl_string;
}

dabc::String::String(const char* v) :
   fStr(0)
{
   fStr = new stl_string;
   fStr->assign(v);
}

dabc::String::String(const String& v) :
   fStr(0)
{
   fStr = new stl_string(*v.fStr);
}

dabc::String::~String()
{
   delete fStr; fStr = 0;
}
         
const char* dabc::String::c_str() const
{
   return fStr->c_str();
}

void dabc::String::append(const char* s)
{
   fStr->append(s);
}

unsigned dabc::String::length() const
{
   return fStr->length();
}

int dabc::String::compare(const char* s) const
{
   return fStr->compare(s);
}

void dabc::String::assign(const char* s)
{
   fStr->assign(s);
}

void dabc::String::assign(const char* s, int len)
{
   fStr->assign(s, len);
}
 
dabc::String& dabc::String::operator=(const char* s)
{
   fStr->assign(s);
   return *this;
}

dabc::String& dabc::String::operator=(String& s)
{
   fStr->assign(*s.fStr);
   return *this;
}
    
// ______________________________________________________________ 

*/

void dabc::formats(dabc::String& sbuf, const char *fmt, ...)
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

dabc::String dabc::format(const char *fmt, ...) 
{ 
   if (!fmt || (strlen(fmt)==0)) return dabc::String("");
    
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
   dabc::String s(buffer); 
   delete [] buffer; 
   va_end(args); 
   return s; 
} 
