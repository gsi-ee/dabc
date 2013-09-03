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


bool dabc::DateTime::GetNow()
{
   timespec tm;
   clock_gettime(CLOCK_REALTIME, &tm);

   tv_sec = tm.tv_sec;
   tv_nsec = tm.tv_nsec;


   return true;
}

bool dabc::DateTime::AsString(char* sbuf, int len, int ndecimal) const
{
   if (null()) return false;

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

   strftime(sbuf, len, "%Y-%m-%d %H:%M:%S", &res);

   if (frac>0) {
      int rlen = strlen(sbuf);
      if (len - rlen > ndecimal + 1) sprintf(sbuf+rlen, ".%0*d", ndecimal, frac);
   }

   return true;
}

double dabc::DateTime::AsDouble() const
{
   return tv_sec + tv_nsec*1e-9;
}


bool dabc::DateTime::AsJSString(char* sbuf, int len) const
{
   snprintf(sbuf, len,"%5.3f", AsDouble());
   return true;
}

