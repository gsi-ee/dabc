#include "dabc/statistic.h"

#include <math.h>

#include "dabc/logging.h"

dabc::CpuStatistic::CpuStatistic()
{
   const char* cpu_stat_file = "/proc/stat";

   fStatFp = fopen (cpu_stat_file, "r");
   if (fStatFp==0)
     EOUT(("fopen of %s failed", cpu_stat_file));

   system1 = 0; user1 = 0; idle1 = 0;
   system2 = 0; user2 = 0; idle2 = 0;

   Reset();
}

dabc::CpuStatistic::~CpuStatistic()
{
   if (fStatFp!=0)
     if (fclose (fStatFp) != 0)
        EOUT(("fclose of stat file failed"));
}

bool dabc::CpuStatistic::Get(unsigned long &system, unsigned long &user, unsigned long &idle)
{
   system = 0; user = 0; idle = 0;

   if (fStatFp==0) return false;

   const char* cpu_str = "cpu";
   const int stat_buffer_size = 1024;
   const char* delimiter = " ";
   static char buffer[stat_buffer_size];

   if (fflush (fStatFp)) {
      EOUT(("fflush of cpu stat file failed"));
      return false;
   }

   for (;;) {
      if ( NULL == fgets (buffer, stat_buffer_size, fStatFp)) {
         EOUT(("%s not found", cpu_str));
         return false;
      }

      if (!strncmp (buffer, cpu_str, strlen (cpu_str))) break;
   }

   (void) strtok (buffer, delimiter);
   user   = strtoul (strtok (NULL, delimiter), NULL, 0);
   user  += strtoul (strtok (NULL, delimiter), NULL, 0);
   system = strtoul (strtok (NULL, delimiter), NULL, 0);
   idle   = strtoul (strtok (NULL, delimiter), NULL, 0);
   rewind(fStatFp);
   return true;
}

bool dabc::CpuStatistic::Reset()
{
   return Get(system1, user1, idle1);
}

bool dabc::CpuStatistic::Measure()
{
   return Get(system2, user2, idle2);
}

double dabc::CpuStatistic::CPUutil()
{
   unsigned long system = system2 - system1;
   unsigned long user = user2 - user1;
   unsigned long idle = idle2 - idle1;

   unsigned long total = system + user + idle;

   double util = 0.;

   if (total!=0)
      util = 1.0 - (1.*idle / total );

//    util = util*2.; // this is only for double processors

    if (util>1.) util = 1.;

    return util;
}


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
