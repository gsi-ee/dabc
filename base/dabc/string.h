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

#ifndef DABC_string
#define DABC_string

#include <string>

namespace dabc {

   extern std::string format(const char *fmt, ...);
   extern void formats(std::string& sbuf, const char *fmt, ...);

   /** Convert size to string of form like 4.2 GB or 3.7 MB
    *\param prec defines precision after decimal point,
    *\param select forces to use special units 0 - default, 1 - B, 2 - KB, 3 - MB, 4 - GB */
   extern std::string size_to_str(unsigned long sz, int prec=1, int select = 0);

   extern bool str_to_int(const char* val, int* res);
   extern bool str_to_uint(const char* val, unsigned* res);
   extern bool str_to_double(const char* val, double* res);
};

#endif
