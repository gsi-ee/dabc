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

#include "dabc/timing.h"

#include <time.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

#include "dabc/logging.h"

bool dabc::TimeStamp::gFast = false;

dabc::TimeStamp::slowclock_t dabc::TimeStamp::gSlowClockZero = dabc::TimeStamp::GetSlowClock();

dabc::TimeStamp::fastclock_t dabc::TimeStamp::gFastClockZero = dabc::TimeStamp::GetFastClock();

double dabc::TimeStamp::gFastClockMult = CalculateFastClockMult(); // coefficient for slow timing

dabc::TimeStamp::slowclock_t dabc::TimeStamp::GetSlowClock()
{
   timespec tm;
   clock_gettime(CLOCK_MONOTONIC, &tm);
   return tm.tv_sec*1000000000LL + tm.tv_nsec;
}


double dabc::TimeStamp::CalculateFastClockMult()
{
   // if we cannot define that TSC is available - not use it
   if (!CheckLinuxTSC()) {
      gFast = false;
      return 1e-9;
   }

   slowclock_t slow1, slow2;
   fastclock_t fast1, fast2;

   double sum1(0.), sum2(0.), coef(0.);

   const int num = 10;

   double values[num];

   for (int n=0;n<num;n++) {

      slow1 = GetSlowClock();
      fast1 = GetFastClock();

      // we will use 10 millisecond sleep to increase precision
      dabc::Sleep(0.01);

      slow2 = GetSlowClock();
      fast2 = GetFastClock();

      if (fast1 == fast2) {
         gFast = false;
         return 1e-9;
      }

      coef = (slow2-slow1) * 1e-9 / (fast2 - fast1);

      values[n] = coef;

//      printf("Measurement %d  koef = %8.2f %lld %llu\n", n, coef>0 ? 1/coef : -1., slow2-slow1, fast2-fast1);

      sum1 += coef;
   }

   double aver = num/sum1;

   for (int n=0;n<num;n++) {
      sum2 += (1/values[n] - aver)*(1/values[n] - aver);
   }

   double deviat = sqrt(sum2/num);

//   printf("Frequency = %8.2f Hz Deviat = %8.2f (rel:%8.7f) hz\n", aver, deviat, deviat/aver);

   if (deviat/aver>0.0001) {
//      printf("Cannot use TSC for time measurement\n");
      gFast = false;
   } else {
      gFast = true;
   }

   return 1./aver;
}


bool dabc::TimeStamp::CheckLinuxTSC()
{
   FILE* f = fopen("/proc/cpuinfo","r");
   if (!f) return false;

   bool can_use_tsc = false;
   bool isgsiamd = false;

   char buf[1024];

   while(fgets(buf, sizeof(buf), f)) {
      if (strstr(buf,"AMD Opteron(tm) Processor 248")!=0) {
         // this is known type of AMD Opteron, we can use it in time measurement
         isgsiamd = true;
         break;
      }

      if (strstr(buf,"constant_tsc")!=0) {
         can_use_tsc = true;
         break;
      }
   }
   fclose(f);

   return can_use_tsc || isgsiamd;
}

void dabc::Sleep(double tm)
{
   if (tm<=0) return;

   struct timespec t;
   t.tv_sec = lrint(tm); /* Whole seconds */
   if (t.tv_sec > tm) t.tv_sec--;
   t.tv_nsec = lrint((tm - t.tv_sec)*1e9); /* nanoseconds */

   if (t.tv_sec==0)
      usleep(t.tv_nsec / 1000);
   else
      nanosleep (&t, 0);
}

// ====================================================


dabc::DateTime& dabc::DateTime::GetNow()
{
   timespec tm;
   clock_gettime(CLOCK_REALTIME, &tm);

   tv_sec = tm.tv_sec;
   tv_nsec = tm.tv_nsec;

   return *this;
}

uint64_t dabc::DateTime::AsJSDate() const
{
   return ((uint64_t) tv_sec) * 1000 + tv_nsec / 1000000;
}


std::string dabc::DateTime::AsString(int ndecimal, bool localtime) const
{
   if (null()) return std::string();

   time_t src = tv_sec;
   int frac = 0;

   if ((ndecimal>0) && (tv_nsec>0)) {
      if (ndecimal>9) ndecimal = 9;
      frac = tv_nsec;
      int maxfrac = 1000000000;
      int dec_cnt = ndecimal;
      while (dec_cnt++<9) {
         // this is rounding rule - if last digit 5 or more, we must increment by 1
         frac = frac / 10 + ((frac % 10 >= 5) ? 1 : 0);
         maxfrac = maxfrac / 10;
         // DOUT0("NSEC = %u FRAC = %d MAXFRAC = %d", tv_nsec, frac, maxfrac);
      }
      if (frac >= maxfrac) { src++; frac = 0; }
   }

   struct tm res;

   if (localtime)
      localtime_r(&src, &res);
   else
      gmtime_r(&src, &res);

   char sbuf[50];

   strftime(sbuf, sizeof(sbuf), "%Y-%m-%d %H:%M:%S", &res);

   if (frac>0) {
      int rlen = strlen(sbuf);
      if ((int)sizeof(sbuf) - rlen > ndecimal + 1) sprintf(sbuf+rlen, ".%0*d", ndecimal, frac);
   }

   return std::string(sbuf);
}

double dabc::DateTime::AsDouble() const
{
   return tv_sec + tv_nsec*1e-9;
}


std::string dabc::DateTime::AsJSString(int ndecimal) const
{
   if (null()) return std::string();
   char sbuf[50];

   time_t src = tv_sec;
   int frac = 0;

   if ((ndecimal>0) && (tv_nsec>0)) {
      if (ndecimal>9) ndecimal = 9;
      frac = tv_nsec;
      int maxfrac = 1000000000;
      int dec_cnt = ndecimal;
      while (dec_cnt++<9) {
         // this is rounding rule - if last digit 5 or more, we must increment by 1
         frac = frac / 10 + ((frac % 10 >= 5) ? 1 : 0);
         maxfrac = maxfrac / 10;
         // DOUT0("NSEC = %u FRAC = %d MAXFRAC = %d", tv_nsec, frac, maxfrac);
      }
      if (frac >= maxfrac) { src++; frac = 0; }
   }

   struct tm res;

   gmtime_r(&src, &res);

   strftime(sbuf, sizeof(sbuf), "%Y-%m-%dT%H:%M:%S", &res);

   if (frac>0) {
      int rlen = strlen(sbuf);
      if ((int)sizeof(sbuf) - rlen > ndecimal + 1) sprintf(sbuf+rlen, ".%0*d", ndecimal, frac);
   }

   int rlen = strlen(sbuf);
   if (rlen >= (int)sizeof(sbuf)-1) return std::string();

   sbuf[rlen] = 'Z';
   sbuf[rlen+1] = 0;

   return std::string(sbuf);
}


std::string dabc::DateTime::OnlyDateAsString(const char *separ) const
{
   if (null()) return std::string();
   time_t src = tv_sec;
   struct tm res;
   gmtime_r(&src, &res);
   if (!separ) separ = "-";
   std::string fmt = std::string("%Y") + separ + "%m" + separ + "%d";
   char sbuf[200];
   strftime(sbuf, sizeof(sbuf), fmt.c_str(), &res);
   return std::string(sbuf);
}

std::string dabc::DateTime::OnlyTimeAsString(const char *separ) const
{
   if (null()) return std::string();
   time_t src = tv_sec;
   struct tm res;
   gmtime_r(&src, &res);
   if (!separ) separ = ":";
   std::string fmt = std::string("%H") + separ + "%M" + separ + "%S";
   char sbuf[200];
   strftime(sbuf, sizeof(sbuf), fmt.c_str(), &res);
   return std::string(sbuf);
}

bool dabc::DateTime::SetOnlyDate(const char* sbuf)
{
   if ((sbuf==0) || (strlen(sbuf)!=10)) return false;

   unsigned year(0), month(0), day(0);
   if (sscanf(sbuf,"%4u-%02u-%02u", &year, &month, &day)!=3) return false;
   if ((year<1970) || (year>2100) || (month>12) || (month==0) || (day>31) || (day==0)) return false;

   struct tm res;
   time_t src = tv_sec;
   gmtime_r(&src, &res);
   res.tm_year = year - 1900;
   res.tm_mon = month - 1;
   res.tm_mday = day;
   res.tm_isdst = 0;
   tv_sec = timegm(&res); // SHOULD we implement timegm ourself??
   return true;
}

bool dabc::DateTime::SetOnlyTime(const char* sbuf)
{
   if ((sbuf==0) || (strlen(sbuf)!=8)) return false;

   unsigned hour(0), min(0), sec(0);
   if (sscanf(sbuf,"%02u:%02u:%02u", &hour, &min, &sec)!=3) return false;
   if ((hour>23) || (min>59) || (sec>59)) return false;

   struct tm res;
   if (null()) {
     memset(&res, 0, sizeof(res));
   } else {
      time_t src = tv_sec;
      gmtime_r(&src, &res);
   }

   res.tm_hour = hour;
   res.tm_min = min;
   res.tm_sec = sec;

   tv_sec = timegm(&res); // SHOULD we implement timegm ourself??
   return true;
}

double dabc::DateTime::DistanceTo(const DateTime& pnt) const
{
   if (null()) return pnt.AsDouble();
   if (pnt.null()) return -1. * AsDouble();

   // to exclude precision lost, calculate sec and nanosec separately

   double res = (pnt.tv_sec >= tv_sec) ? 1.*(pnt.tv_sec - tv_sec) : -1. * (tv_sec - pnt.tv_sec);

   if (pnt.tv_nsec >= tv_nsec) res += 1e-9* (pnt.tv_nsec - tv_nsec);
                          else res -= 1e-9 * (tv_nsec - pnt.tv_nsec);

   return res;
}


