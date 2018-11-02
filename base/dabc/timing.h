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

#ifndef DABC_timing
#define DABC_timing

#include <stdint.h>

#ifndef DABC_string
#include "dabc/string.h"
#endif

namespace dabc {

   class Profiler;

   /** \brief Class for acquiring and holding timestamps.
    *
    * \ingroup dabc_all_classes
    *
    * Time measurement is done in seconds relative to program start.
    * In normal case constant tsc counter is used - by program start during 0.1 seconds value of
    * tsc counter compared with normal monolitic clock. If deviation is less than 0.01%, tsc counter
    * will be used for the measurement, otherwise normal standard getclock.
    * Major difference is performance. TSC counter can be read within 20 nanosec while getclock call
    * takes about 300 ns. Before usage of TSC it is checked that constant_tsc is supported by the
    * CPU looking in /proc/cpuinfo file.
    */

   struct TimeStamp {

      friend class Profiler;

      protected:
         double fValue;  ///< time since start of the application in seconds

         static bool gFast;  ///< indicates if fast or slow method is used for time measurement

         typedef long long int slowclock_t;

         static slowclock_t gSlowClockZero;

         static slowclock_t GetSlowClock();

         #if defined (__x86_64__) || defined(__i386__)
         typedef unsigned long long fastclock_t;
         static inline fastclock_t GetFastClock()
         {
            unsigned low, high;
            asm volatile ("rdtsc" : "=a" (low), "=d" (high));
            return (fastclock_t(high) << 32) | low;
         }
         #elif defined(__PPC__) || defined(__PPC64__)
         typedef unsigned long fastclock_t;
         static inline fastclock_t GetFastClock()
         {
            fastclock_t ret;
            asm volatile ("mftb %0" : "=r" (ret) : );
            return ret;
         }
         #elif defined(__ia64__)
         typedef unsigned long fastclock_t;
         static inline fastclock_t GetFastClock()
         {
            fastclock_t ret;
            asm volatile ("mov %0=ar.itc" : "=r" (ret));
            return ret;
         }
         #else
         typedef slowclock_t fastclock_t;
         static inline fastclock_t GetFastClock() { return GetSlowClock(); }
         #endif

         static fastclock_t gFastClockZero;
         static double gFastClockMult;

         static double CalculateFastClockMult();

         static bool CheckLinuxTSC();

         TimeStamp(double v) : fValue(v) {}

         /** \brief Method returns TimeStamp instance with current time, measured by
          * 'slow' getclock() function.*/
         static inline TimeStamp Slow() { return TimeStamp((GetSlowClock() - gSlowClockZero) * 1e-9); }

         /** \brief Method returns TimeStamp instance with current time, measure by
          * fast TSC clock. One should not use this method directly while TSC clock may not be correctly working */
         static inline TimeStamp Fast() { return TimeStamp((GetFastClock() - gFastClockZero) * gFastClockMult); }

      public:

         TimeStamp() : fValue(0)  {}

         TimeStamp(const TimeStamp& src) : fValue(src.fValue) {}

         TimeStamp& operator=(const TimeStamp& src) { fValue = src.fValue; return *this; }

         TimeStamp& operator+=(double _add) { fValue+=_add; return *this; }

         TimeStamp& operator-=(double _sub) { fValue-=_sub; return *this; }

         TimeStamp operator+(double _add) const { return TimeStamp(fValue+_add); }

         TimeStamp operator-(double _sub) const { return TimeStamp(fValue-_sub); }

         double operator()() const { return fValue; }

         /** \brief Return time stamp in form of double (in seconds) */
         double AsDouble() const { return fValue; }

         double operator-(const TimeStamp& src) const { return fValue - src.fValue; }

         bool operator<(const TimeStamp& src) const { return fValue < src.fValue; }

         bool operator>(const TimeStamp& src) const { return fValue > src.fValue; }

         bool operator==(const TimeStamp& src) const { return fValue == src.fValue; }

         /** \brief Returns true if time stamp is not initialized or its value less than 0 */
         bool null() const { return fValue <= 0.; }

         /** \brief Set time stamp value to null */
         void Reset() { fValue = 0.; }

         /** \brief Method to acquire current time stamp */
         inline void GetNow() { fValue = gFast ? (GetFastClock() - gFastClockZero) * gFastClockMult : (GetSlowClock() - gSlowClockZero) * 1e-9; }

         /** \brief Method to acquire current time stamp plus shift in seconds */
         inline void GetNow(double shift) { GetNow(); fValue += shift;  }

         /** Method return time in second, spent from the time kept in TimeStamp instance
          * If time was not set before, returns 0 */
         inline double SpentTillNow() const
         {
            return null() ? 0 : Now().AsDouble() - AsDouble();
         }

         /** Method return time in second, spent from the time kept in TimeStamp instance
          * If specified, automatically set to the current time
          * If time was not set before, returns 0 */
         inline double SpentTillNow(bool set_to_now)
         {
            double res = 0.;
            TimeStamp now = Now();
            if (!null()) res = now.AsDouble() - AsDouble();
            if (set_to_now) *this = now;
            return res;
         }

         /** Method returns true if specified time interval expired
          * relative to time, kept in TimeStamp instance */
         inline bool Expired(double interval = 0.) const { return Now().AsDouble() > AsDouble() + interval; }

         /** \brief Method returns true if specified time interval expired
          * relative to time, kept in TimeStamp instance.
          *
          * \param[in] curr      current time
          * \param[in] interval  time which should expire
          * \returns             true if specified time interval had expired  */
         inline bool Expired(const TimeStamp& curr, double interval) const { return curr.AsDouble() > AsDouble() + interval; }

         /** \brief Method returns TimeStamp instance with current time stamp value, measured
          * either by fast TSC (if it is detected and working correctly), otherwise slow getclock() method
          * will be used */
         static inline TimeStamp Now() { return gFast ? Fast() : Slow(); }

         static void SetUseSlow() { gFast = false; }
   };

   // ==========================================================================

   /** \brief Class for holding GMT time with precision of nanoseconds
    *
    * \ingroup dabc_all_classes
    *
    * For time measurement function like gettimeofday is used.
    */

   struct DateTime {
      protected:
         unsigned tv_sec;   // GMT time in seconds, since 1.1.1970
         unsigned tv_nsec;  // fractional part of time in nanoseconds
      public:
         DateTime() : tv_sec(0), tv_nsec(0) {}

         DateTime(uint64_t jsdate) : tv_sec(0), tv_nsec(0) { SetJSDate(jsdate); }

         DateTime(const DateTime& src) : tv_sec(src.tv_sec), tv_nsec(src.tv_nsec) {}

         DateTime& operator=(const DateTime& src)
         {
            tv_sec = src.tv_sec;
            tv_nsec = src.tv_nsec;
            return *this;
         }

         double operator-(const DateTime& src) const { return src.DistanceTo(*this); }

         bool null() const { return (tv_sec==0) && (tv_nsec==0); }

         DateTime& GetNow();

         /** \brief Return date and time in seconds since 1.1.1970 */
         double AsDouble() const;

         /** \brief Return date and time in JS format - number of millisecond  since 1.1.1970 */
         uint64_t AsJSDate() const;

         /** \brief Set value in form of JS date - milliseconds since 1.1.1970 */
         void SetJSDate(uint64_t jsdate)
         {
            tv_sec = jsdate / 1000;
            tv_nsec = ((jsdate % 1000)*1000000);
         }

         /** \brief convert string into human-readable format, cannot be interpret directly in JavaScript */
         std::string AsString(int ndecimal = 0) const;

         /** \brief convert string into sec.frac format, can be interpret directly in JavaScript
          * ISO 8601 standard is used and produces string like  '2013-09-16T12:42:30.884Z'
          * Time in GMT time zone */
         std::string AsJSString(int ndecimal = 3) const;

         /** \brief Fills only date as string */
         std::string OnlyDateAsString(const char *separ = nullptr) const;

         /** \brief Fills only time as string */
         std::string OnlyTimeAsString(const char *separ = nullptr) const;

         /** \brief Set only date part from the string */
         bool SetOnlyDate(const char* sbuf);

         /** \brief Set only time part of DateTime */
         bool SetOnlyTime(const char* sbuf);

         /** \brief Return distance in seconds to provided date */
         double DistanceTo(const DateTime& src) const;
   };

   inline TimeStamp Now() { return TimeStamp::Now(); }

   void Sleep(double tm);
}

#endif

