#ifndef DABC_string
#define DABC_string

#include <string>

namespace dabc {

   typedef std::string String;

/*    
   class String {
      protected:
         class stl_string;
         
         stl_string* fStr;
         
      public:  
         String();
         String(const char* v);
         String(const String& v);
         virtual ~String();
         
         const char* c_str() const;
         void append(const char* s);
         unsigned length() const;
         int compare(const char* s) const;
         void assign(const char* s);
         void assign(const char* s, int len);
         
         String& operator=(const char* s);
         String& operator=(String& s);
   };
*/

   extern dabc::String format(const char *fmt, ...);
   extern void formats(dabc::String& sbuf, const char *fmt, ...);
};

#define FORMAT(args) dabc::format args .c_str()

#endif
