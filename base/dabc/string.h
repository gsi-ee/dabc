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

   extern std::string format(const char *fmt, ...)
   #if defined(__GNUC__) && !defined(__CINT__)
    __attribute__((format(printf, 1, 2)))
   #endif
   ;
   extern void formats(std::string& sbuf, const char *fmt, ...)
   #if defined(__GNUC__) && !defined(__CINT__)
     __attribute__((format(printf, 2, 3)))
   #endif
   ;

   /** \brief Convert size to string of form like 4.2 GB or 3.7 MB
    *
    * \param[in] sz      size value to convert
    * \param[in] prec    precision after decimal point,
    * \param[in] select  forces to use special units 0 - default, 1 - B, 2 - KB, 3 - MB, 4 - GB
    * \returns           string with converted value */
   extern std::string size_to_str(unsigned long sz, int prec = 1, int select = 0);

   /** \brief Convert number to string of form like 4.2G or 3.7M
    *
    * \param[in] num     number to convert
    * \param[in] prec    precision after decimal point,
    * \param[in] select  forces to use special units 0 - default, 1 - normal, 2 - K, 3 - M, 4 - G
    * \returns           string with converted value */
   extern std::string number_to_str(unsigned long num, int prec = 1, int select = 0);

   /** \brief Convert string to integer value
    * \param[in]  val   source string
    * \param[out] res   pointer on result value
    * \returns true if succeed */
   extern bool str_to_int(const char *val, int* res);

   /** \brief Convert string to long integer value
    * \param[in]  val   source string
    * \param[out] res   pointer on result value
    * \returns true if succeed */
   extern bool str_to_lint(const char *val, long* res);

   /** \brief Convert string to long long integer value
    * \param[in]  val   source string
    * \param[out] res   pointer on result value
    * \returns true if succeed */
   extern bool str_to_llint(const char *val, long long* res);

   /** \brief Convert string to unsigned integer value
    * One could use hexadecimal (in form 0xabc100)  or decimal format
    * \param[in]  val   source string
    * \param[out] res   pointer on result value
    *\returns true if succeed */
   extern bool str_to_uint(const char *val, unsigned* res);

   /** \brief Convert string to long unsigned integer value
    * One could use hexadecimal (in form 0xabc100)  or decimal format
    * \param[in]  val   source string
    * \param[out] res   pointer on result value
    *\returns true if succeed */
   extern bool str_to_luint(const char *val, long unsigned* res);

   /** \brief Convert string to long unsigned integer value
    * One could use hexadecimal (in form 0xabc100)  or decimal format
    * \param[in]  val   source string
    * \param[out] res   pointer on result value
    *\returns true if succeed */
   extern bool str_to_lluint(const char *val, long long unsigned* res);

   /** \brief Convert string to double value
    * \param[in]  val   source string
    * \param[out] res   pointer on result value
    *\returns true if succeed */
   extern bool str_to_double(const char *val, double* res);

   /** \brief Convert string to bool value
    * \param[in]  val   source string
    * \param[out] res   pointer on result value
    *\returns true if succeed */
   extern bool str_to_bool(const char *val, bool* res);

   /** \brief Replace all matches in the string
    * \param[in] str     initial string
    * \param[in] match   find content
    * \param[in] replace replace content
    *\returns result string */
   extern std::string replace_all(const std::string &str, const std::string &match, const std::string &replace);


   class NumericLocale {
   protected:
      std::string fPrev;
   public:
      NumericLocale();
      ~NumericLocale();
   };

};

#endif
