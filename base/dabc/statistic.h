#ifndef DABC_statistic
#define DABC_statistic

#include <stdio.h>

#include <stdint.h>

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

namespace dabc {

   class CpuStatistic {
      public:
         CpuStatistic();
         virtual ~CpuStatistic();

         bool Reset();
         bool Measure();

         double CPUutil();

       protected:
         bool Get(unsigned long &system, unsigned long &user, unsigned long &idle);

         unsigned long system1, system2;
         unsigned long user1, user2;
         unsigned long idle1, idle2;

         FILE *fStatFp;
   };

   class Ratemeter {
     public:
        Ratemeter();
        virtual ~Ratemeter();

        void DoMeasure(double interval_sec, long npoints);

        void Packet(int size);
        void Reset();

        double GetRate();
        double GetTotalTime();
        int AverPacketSize();

        int GetNumOper() const { return numoper; }

        void SaveInFile(const char* fname);

        static void SaveRatesInFile(const char* fname, Ratemeter** rates, int nrates, bool withsum = false);

     protected:
        TimeStamp_t firstoper, lastoper;
        int64_t numoper, totalpacketsize;

        double fMeasureInterval; // interval between two points
        long fMeasurePoints;
        double* fPoints;
   };

   class Average {
      public:
         Average() { Reset(); }
         virtual ~Average() {}
         void Reset() { num = 0; sum1=0.; sum2=0.; min=0.; max=0.; }
         void Fill(double zn);
         long Number() const { return num; }
         double Mean() const { return num>0 ? sum1/num : 0.; }
         double Max() const { return max; }
         double Min() const { return min; }
         double Dev() const;
         void Show(const char* name, bool showextr = false);
      protected:
         long num;
         double sum1;
         double sum2;
         double min, max;
   };

   long GetProcVirtMem();


}


#endif
