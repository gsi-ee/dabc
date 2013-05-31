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

   /** \brief Convert size to string of form like 4.2 GB or 3.7 MB
    *
    * \param[in] sz      size value to convert
    * \param[in] prec    precision after decimal point,
    * \param[in] select  forces to use special units 0 - default, 1 - B, 2 - KB, 3 - MB, 4 - GB
    * \returns           string with converted value */
   extern std::string size_to_str(unsigned long sz, int prec=1, int select = 0);

   /** \brief Convert string to integer value
    * \param[in]  val   source string
    * \param[out] res   pointer on result value
    * \returns true if succeed */
   extern bool str_to_int(const char* val, int* res);

   /** \brief Convert string to unsigned integer value
    * One could use hexadecimal (in form 0xabc100)  or decimal format
    * \param[in]  val   source string
    * \param[out] res   pointer on result value
    *\returns true if succeed */
   extern bool str_to_uint(const char* val, unsigned* res);

   /** \brief Convert string to long unsigned integer value
    * One could use hexadecimal (in form 0xabc100)  or decimal format
    * \param[in]  val   source string
    * \param[out] res   pointer on result value
    *\returns true if succeed */
   extern bool str_to_luint(const char* val, long unsigned* res);

   /** \brief Convert string to double value
    * \param[in]  val   source string
    * \param[out] res   pointer on result value
    *\returns true if succeed */
   extern bool str_to_double(const char* val, double* res);
};

#endif
