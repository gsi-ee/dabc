#include "dabc/statistic.h"

#include <math.h>

#include "dabc/logging.h"

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

void dabc::Ratemeter::Packet(int size)
{
   TimeStamp_t now = TimeStamp();
   if (IsNullTime(firstoper)) {
      lastoper = NullTimeStamp;
      firstoper = now;
      numoper = 0;
      totalpacketsize = 0;
   } else {
      lastoper = now;
      numoper++;
      totalpacketsize+=size;
   }

   if (fPoints!=0) {
      double npnt = TimeDistance(firstoper, now) / fMeasureInterval;
      if ((npnt>=0) && (npnt<fMeasurePoints))
         fPoints[long(npnt)]+=size / fMeasureInterval;
   }
}

void dabc::Ratemeter::Reset()
{
  firstoper = NullTimeStamp;
  lastoper = NullTimeStamp;
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
   return totalpacketsize / totaltm / 1024. / 1024.;
}

double dabc::Ratemeter::GetTotalTime()
{
   return TimeDistance(firstoper, lastoper);
}

int dabc::Ratemeter::AverPacketSize()
{
   if (numoper<=0) return 0;
   return totalpacketsize/numoper;
}

void dabc::Ratemeter::DoMeasure(double interval_sec, long npoints)
{
   Reset();
   fMeasureInterval = interval_sec;
   fMeasurePoints = npoints;
   fPoints = new double[fMeasurePoints];
   for (int n=0;n<fMeasurePoints;n++)
      fPoints[n] = 0.;
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
