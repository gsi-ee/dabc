/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "dabc/timing.h"

// #include <sys/timex.h>
// #include <sys/time.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#include "dabc/logging.h"

dabc::TimeSource dabc::gStamping(true);

const dabc::TimeStamp_t dabc::NullTimeStamp = 0.;


double Linux_get_cpu_mhz()
{
   FILE* f;
   char buf[1024];
   float mhz = -1;

   f = fopen("/proc/cpuinfo","r");
   if (!f) return -1;

   bool ismyamd = false;
   bool isconst_tsc = false;

   double max_ghz = -1;

   while(fgets(buf, sizeof(buf), f)) {
      float m;
      if (strstr(buf,"AMD Opteron(tm) Processor 248")!=0) {
         ismyamd = true;
         continue;
      }

      if (strstr(buf,"model name")!=0) {
         const char* pos = strstr(buf,"@");
         if (pos==0) continue;
         if (sscanf(pos+1, "%fGHz", &m) != 1) continue;
         if (max_ghz<0) max_ghz = m;
         if (fabs(max_ghz-m)>0.1) { max_ghz = -1; break; }
      }

      if (strstr(buf,"constant_tsc")!=0) {
         isconst_tsc = true;
         continue;
      }
      if (sscanf(buf, "cpu MHz : %f", &m) == 1) {
         if (mhz < 0.) mhz = m;
         if (fabs(mhz-m)>10.) {
//          EOUT(("Conflicting CPU frequency values detected: %f != %f, use slow timing", mhz, m));
            mhz = -1;
            break;
         }
      }
   }
   fclose(f);

   if (isconst_tsc && (max_ghz>0.)) return max_ghz*1000.;

//   DOUT0(("Find MHZ = %5.3f isamd = %s const_tsc = %s", mhz, DBOOL(ismyamd), DBOOL(isconst_tsc)));

   return ismyamd ? mhz : -1;
}

dabc::cycles_t dabc::GetSlowClock()
{
//   ntptimeval val;
//   ntp_gettime(&val);
//   return val.time.tv_sec * 1000000UL + val.time.tv_usec;

   timespec tm;
   clock_gettime(CLOCK_MONOTONIC, &tm);
   return tm.tv_sec*1000000000LL + tm.tv_nsec;
}

#ifdef DABC_SLOWTIMING

dabc::cycles_t dabc::GetFastClock()
{
   return dabc::GetSlowClock(); 
}

#endif   

// __________________________________________________


bool dabc::TimeSource::gForceSlowTiming = false;

dabc::TimeSource::TimeSource(bool usefast) 
{
#ifdef DABC_SLOWTIMING
   fUseFast = false;
#else
   fUseFast = gForceSlowTiming ? false : usefast;
#endif

   fTimeScale = 0.001;
   
   if (fUseFast) {
      double cpu_mhz = Linux_get_cpu_mhz();
      if (cpu_mhz>0) fTimeScale = 1./cpu_mhz; else {
         DOUT5(("Use slow timing"));
         fUseFast = false;
      }
   }

   fClockShift = fUseFast ? GetFastClock() : GetSlowClock();
   fTimeShift = 0.; 
}

dabc::TimeSource::TimeSource(const TimeSource &src) :
   fUseFast(src.fUseFast),
   fClockShift(src.fClockShift),
   fTimeScale(src.fTimeScale),
   fTimeShift(src.fTimeShift)
{
}
         
dabc::TimeSource& dabc::TimeSource::operator=(const TimeSource& rhs)
{
   if (this!=&rhs) {
      fUseFast = rhs.fUseFast;
      fClockShift = rhs.fClockShift;
      fTimeScale = rhs.fTimeScale;
      fTimeShift = rhs.fTimeShift;
   } 
   return *this; 
}

void dabc::TimeSource::AdjustTimeStamp(TimeStamp_t stamp)
{
   fTimeShift += (stamp-GetTimeStamp()); 
}

void dabc::TimeSource::AdjustCalibr(double dshift, double dscale)
{
   fTimeShift += dshift;
   
   if (dscale!=1.) {
      fTimeShift += (GetTimeStamp()-fTimeShift)*(1. - dscale);
      fTimeScale *= dscale;
   }
}

void dabc::MicroSleep(int microsec)
{
   if (microsec<1) microsec = 1;

   if (microsec>=1000000) {
       struct timespec t;
       t.tv_sec = microsec / 1000000; /* Whole seconds */
       t.tv_nsec = (microsec % 1000000) * 1000;
       nanosleep (&t, 0);
   } else
      usleep(microsec);
}

void dabc::LongSleep(int sec)
{
   MicroSleep(sec*1000000);
}

void dabc::ShowLongSleep(const char* title, int sec)
{
   fprintf(stdout, "%s    ", title);
   while (sec-->0) {
      fprintf(stdout, "\b\b\b%3d", sec);
      fflush(stdout);
      LongSleep(1);
   }
   fprintf(stdout, "\n");
}
