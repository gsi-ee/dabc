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

#ifndef DABC_string
#define DABC_string

#include <string>

namespace dabc {

   extern std::string format(const char *fmt, ...);
   extern void formats(std::string& sbuf, const char *fmt, ...);

   extern bool str_to_int(const char* val, int* res);
   extern bool str_to_uint(const char* val, unsigned* res);
   extern bool str_to_double(const char* val, double* res);
};

#define FORMAT(args) dabc::format args .c_str()

#endif
