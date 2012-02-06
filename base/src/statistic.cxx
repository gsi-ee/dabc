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

#include "dabc/statistic.h"

#include <math.h>
#include <string.h>

#include "dabc/logging.h"
#include "dabc/timing.h"

dabc::CpuStatistic::CpuStatistic(bool withmem) :
   fStatFp(0),
   fProcStatFp(0),
   fCPUs(),
   fVmSize(0),
   fVmPeak(0),
   fNumThreads(0)
{
   const char* cpu_stat_file = "/proc/stat";

   fStatFp = fopen (cpu_stat_file, "r");
   if (fStatFp==0)
     EOUT(("fopen of %s failed", cpu_stat_file));


   if (withmem) {
      pid_t id = getpid();

      char fname[100];
      sprintf(fname,"/proc/%d/status", id);

      fProcStatFp = fopen(fname,"r");
      if (fProcStatFp==0)
         EOUT(("fopen of %s failed", fname));
   }

   Measure();
}

dabc::CpuStatistic::~CpuStatistic()
{
   if (fStatFp!=0)
     if (fclose (fStatFp) != 0)
        EOUT(("fclose of stat file failed"));

   if (fProcStatFp!=0)
     if (fclose (fProcStatFp) != 0)
        EOUT(("fclose of proc stat file failed"));
}

bool dabc::CpuStatistic::Measure()
{
   if (fStatFp==0) return false;

   rewind(fStatFp);

   fflush(fStatFp);

   char buffer[1024];

   unsigned cnt = 0;

   while (fgets(buffer, sizeof(buffer), fStatFp)) {

      if (strncmp (buffer, "cpu", 3)!=0) break;

      const char* info = buffer;

      while ((*info!=' ') && (*info != 0)) info++;
      if (*info==0) break;

      unsigned long curr_user(0), curr_nice(0), curr_sys(0), curr_idle(0);

      sscanf(info, "%lu %lu %lu %lu", &curr_user, &curr_nice, &curr_sys, &curr_idle);

      //DOUT0(("Scan:%s", info));
      //if (cnt==0)
      //   DOUT0(("Res:%lu %lu %lu %lu", curr_user, curr_nice, curr_sys, curr_idle));

      curr_user += curr_nice;

      unsigned long user(0), sys(0), idle(0);

      if (cnt>=fCPUs.size()) {
         fCPUs.push_back(SingleCpu());
      } else {
         user = curr_user - fCPUs[cnt].last_user;
         sys = curr_sys - fCPUs[cnt].last_sys;
         idle = curr_idle - fCPUs[cnt].last_idle;
      }

      unsigned long total = user + sys + idle;
      if (total>0) {
         fCPUs[cnt].user_util = 1. * user / total;
         fCPUs[cnt].sys_util = 1. * sys / total;
         fCPUs[cnt].cpu_util = 1. * (user + sys) / total;
      } else {
         fCPUs[cnt].user_util = 0.;
         fCPUs[cnt].sys_util = 0.;
         fCPUs[cnt].cpu_util = 0.;
      }

      fCPUs[cnt].last_user = curr_user;
      fCPUs[cnt].last_sys = curr_sys;
      fCPUs[cnt].last_idle = curr_idle;

      cnt++;
   }

   if (fProcStatFp != 0) {
      rewind(fProcStatFp);
      fflush(fProcStatFp);

      while (fgets(buffer, sizeof(buffer), fProcStatFp)) {
         if (strncmp(buffer, "VmSize:", 7)==0)
            sscanf(buffer + 7, "%lu", &fVmSize);
         else
         if (strncmp(buffer, "VmPeak:", 7)==0)
            sscanf(buffer + 7, "%lu", &fVmPeak);
         else
         if (strncmp(buffer, "Threads:", 8)==0) {
           sscanf(buffer + 8, "%lu", &fNumThreads);
           break;
         }
      }
   }

   return true;
}

bool dabc::CpuStatistic::Reset()
{
   fCPUs.clear();
   return Measure();
}

// ____________________________________________________________________________



dabc::Ratemeter::Ratemeter()
{
   fMeasureInterval = 0;
   fMeasurePoints = 0;
   fPoints = 0;
   Reset();
}

dabc::Ratemeter::~Ratemeter()
{
   Reset();
}

void dabc::Ratemeter::Packet(int size, double now)
{
   if (now <= 0.) now = dabc::Now().AsDouble();

   if (firstoper<=0.) {
      firstoper = now;
      lastoper = 0.;
      numoper = 0;
      totalpacketsize = 0;
   } else
   if (now>=firstoper) {
      lastoper = now;
      numoper++;
      totalpacketsize+=size;
   } else
      return; // do not account packets comming before expected start of measurement

   if ((fPoints!=0) && (fMeasureInterval>0)) {
      long npnt = lrint((now - firstoper) / fMeasureInterval);
      if ((npnt>=0) && (npnt<fMeasurePoints))
         fPoints[npnt] += size / fMeasureInterval;
   }
}

void dabc::Ratemeter::Reset()
{
  firstoper = 0.;
  lastoper = 0.;
  numoper = 0;
  totalpacketsize = 0;

  if (fPoints!=0) {
     delete[] fPoints;
     fMeasureInterval = 0;
     fMeasurePoints = 0;
     fPoints = 0;
  }
}

double dabc::Ratemeter::GetRate()
{
   // calculate rate as MB/s

   double totaltm = GetTotalTime();
   if ((numoper<2) || (totaltm<=0.)) return 0.;
   return totalpacketsize / totaltm * 1e-6;
}

double dabc::Ratemeter::GetTotalTime()
{
   return lastoper - firstoper;
}

int dabc::Ratemeter::AverPacketSize()
{
   if (numoper<=0) return 0;
   return totalpacketsize/numoper;
}

void dabc::Ratemeter::DoMeasure(double interval_sec, long npoints, double firsttm)
{
   Reset();
   fMeasureInterval = interval_sec;
   fMeasurePoints = npoints;
   fPoints = new double[fMeasurePoints];
   for (int n=0;n<fMeasurePoints;n++)
      fPoints[n] = 0.;

   firstoper = firsttm;
}

void dabc::Ratemeter::SaveRatesInFile(const char* fname, Ratemeter** rates, int nrates, bool withsum)
{
   if (nrates<=0) return;

   int npoints = 0;
   double interval = 0;
   for (int nr=0;nr<nrates;nr++)
      if (rates[nr]!=0) {
         npoints = rates[nr]->fMeasurePoints;
         interval = rates[nr]->fMeasureInterval;
      }

   if ((npoints<=0) || (interval<=0)) return;

   FILE* f = fopen(fname,"w");

   for (int n=0;n<npoints;n++) {
      fprintf(f, "%7.1f", n*interval*1e3); // time in millisec
      double sum = 0.;
      for (int nr=0;nr<nrates;nr++) {
         if (rates[nr]==0) continue;
         fprintf(f,"\t%5.1f", rates[nr]->fPoints[n]);
         sum+=rates[nr]->fPoints[n];
      }
      if (withsum) fprintf(f,"\t%5.1f\n", sum);
              else fprintf(f,"\n");
   }

   fclose(f);
}

void dabc::Ratemeter::SaveInFile(const char* fname)
{
   Ratemeter* rate = this;
   SaveRatesInFile(fname, &rate, 1, false);
}

// ************************************************

dabc::Average::Average()
{
   hist = 0;
   nhist = 0;
   hist_min = 0.;
   hist_max = 1.;
   Reset();
}


dabc::Average::~Average()
{
   AllocateHist(0, 0., 1.);
}

void dabc::Average::AllocateHist(int nbins, double xmin, double xmax)
{
   if (hist) { delete[] hist; hist = 0; nhist = 0; }

   if ((nbins>0) && (xmax>xmin)) {
      hist = new long[nbins+2];
      nhist = nbins;
      hist_min = xmin;
      hist_max = xmax;
      for (int n=0;n<nhist+2;n++) hist[n] = 0;
   }
}


void dabc::Average::Reset()
{
   num = 0;
   sum1=0.;
   sum2=0.;
   min=0.;
   max=0.;
   if (hist && nhist)
      for (int n=0;n<nhist+2;n++)
         hist[n] = 0;
}


void dabc::Average::Fill(double zn)
{
   if (num==0) {
      min = zn;
      max = zn;
   } else {
      if (zn>max) max = zn; else
      if (zn<min) min = zn;
   }

   num++;
   sum1 += zn;
   sum2+= zn*zn;

   if (hist && nhist) {
      if (zn<hist_min) hist[0]++; else
      if (zn>=hist_max) hist[nhist+1]++; else {
         // +0.5 to set integer value in the middle of interval
         long bin = lrint((zn - hist_min) / (hist_max-hist_min) * nhist  + 0.5);
         if ((bin>0) && (bin<=nhist)) hist[bin]++;
                                else EOUT(("Bin error bin = %ld nhist = %d", bin, nhist));
      }
   }
}

double dabc::Average::Dev() const
{
   if (num<=1) return 0.;
   return sqrt(sum2/num - Mean()*Mean());
}

void dabc::Average::Show(const char* name, bool showextr)
{
   if (showextr)
      DOUT1(("%s = %f +- %f (min = %f, max = %f)",name,Mean(), Dev(), min, max));
   else
      DOUT1(("%s = %f +- %f",name,Mean(), Dev()));
}

void dabc::Average::ShowHist()
{
   if ((hist==0) || (nhist==0)) return;

   long sum0(0);
   for (int n=0;n<nhist+2;n++) sum0+=hist[n];
   if (sum0<=0) sum0=1;

   DOUT1(("Below %5.2f cnt = %3ld %5.1f", hist_min, hist[0], 100.*hist[0]/sum0));
   long sum1 = hist[0];
   for (int n=1;n<=nhist;n++) {
      sum1+=hist[n];
      DOUT1(("Bin%02d x:%5.2f = %3ld %5.1f", n, (n - 0.5) / nhist * (hist_max-hist_min) + hist_min, hist[n], 100.*sum1/sum0));
   }
   DOUT1(("Over %5.2f cnt = %3ld", hist_max, hist[nhist+1]));
}


// __________________________________________________________________

long dabc::GetProcVirtMem()
{
    DOUT1(("Something %d", 5));

    pid_t id = getpid();

    char fname[100];
    sprintf(fname,"/proc/%d/status", id);

    FILE* f = fopen(fname,"r");
    if (f==0) return 0;
    char buf[256];

    long sz = 0;

    while(fgets(buf, sizeof(buf), f)) {
       int rc = sscanf(buf, "VmSize: %ld", &sz);
       if (rc == 1) break;
    }
   fclose(f);

   return sz;
}
