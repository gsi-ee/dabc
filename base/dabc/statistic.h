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
#ifndef DABC_statistic
#define DABC_statistic

#include <stdio.h>

#include <stdint.h>

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#include <vector>

namespace dabc {

   class CpuStatistic {
      protected:

         struct SingleCpu {
            unsigned long last_user, last_sys, last_idle;
            double user_util, sys_util, cpu_util;
         };

         FILE *fStatFp;
         FILE *fProcStatFp;
         std::vector<SingleCpu> fCPUs;

         long unsigned  fVmSize;
         long unsigned  fVmPeak;
         long unsigned  fNumThreads;

      public:
         CpuStatistic(bool withmem = false);
         virtual ~CpuStatistic();

         bool Measure();
         bool Reset();

         unsigned NumCPUs() const { return fCPUs.size(); }

         double CPUutil(unsigned n = 0) const { return fCPUs[n].cpu_util; }
         double UserUtil(unsigned n = 0) const { return fCPUs[n].user_util; }
         double SysUtil(unsigned n = 0) const { return fCPUs[n].sys_util; }

         long unsigned GetVmSize() const { return fVmSize; }
         long unsigned GetVmPeak() const { return fVmPeak; }
         long unsigned GetNumThreads() const { return fNumThreads; }

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
