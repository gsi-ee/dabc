#ifndef DABC_string
#define DABC_string

#include <string>

namespace dabc {

   extern std::string format(const char *fmt, ...);
   extern void formats(std::string& sbuf, const char *fmt, ...);
};

#define FORMAT(args) dabc::format args .c_str()

#endif
