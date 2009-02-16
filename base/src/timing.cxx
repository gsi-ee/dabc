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

#include <sys/timex.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "dabc/logging.h"

double gLinuxCPUMhz = 1.0;

dabc::TimeSource dabc::gStamping(true);

const dabc::TimeStamp_t dabc::NullTimeStamp = 0.;


double Linux_get_cpu_mhz()
{
   FILE* f;
   char buf[256];
   double mhz = 0.0;

   f = fopen("/proc/cpuinfo","r");
   if (!f)
      return 0.0;
   while(fgets(buf, sizeof(buf), f)) {
      double m;
      int rc;
      rc = sscanf(buf, "cpu MHz : %lf", &m);
      if (rc != 1) {   // PPC has a different format
         rc = sscanf(buf, "clock : %lf", &m);
         if (rc != 1)
            continue;
      }
      if (mhz == 0.0) {
         mhz = m;
         continue;
      }
      if (mhz != m) {
          EOUT(("Conflicting CPU frequency values detected: %lf != %lf\n", mhz, m));
          return 1000.0;
      }
   }
   fclose(f);
   return mhz;
}

dabc::cycles_t dabc::GetSlowClock()
{
   ntptimeval val;
   ntp_gettime(&val);

   return val.time.tv_sec * 1000000UL + val.time.tv_usec;
}

#ifdef DABC_SLOWTIMING

dabc::cycles_t dabc::GetFastClock()
{
   return dabc::GetSlowClock(); 
}

#endif   

namespace dabc {

   double GetClockConvertion(bool isfast)
   {
      if (!isfast) return 1.;
      
      if (gLinuxCPUMhz<10) 
         gLinuxCPUMhz = Linux_get_cpu_mhz();
         
      if (gLinuxCPUMhz<10) {
         EOUT(("Cannot define CPU clock, use default 2200")); 
         gLinuxCPUMhz = 2200.;   
      }
      
      return 1./gLinuxCPUMhz;
   }
}

// __________________________________________________


bool dabc::TimeSource::gForceSlowTiming = false;

dabc::TimeSource::TimeSource(bool usefast) 
{
#ifdef DABC_SLOWTIMING
   fUseFast = false;
#else
   fUseFast = gForceSlowTiming ? false : usefast;
#endif
   
   static cycles_t firstslowclock = GetSlowClock();
   static cycles_t firstfastclock = GetFastClock();

   fClockShift = fUseFast ? firstfastclock : firstslowclock; 
   fTimeScale = GetClockConvertion(fUseFast);
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
