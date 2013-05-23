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

#ifndef DABC_statistic
#define DABC_statistic

#include <stdio.h>

#include <stdint.h>

#include <vector>

namespace dabc {

   /** \brief Helper class to get CPU statistic
    *
    * \ingroup dabc_all_classes
    *
    */

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

         static long GetProcVirtMem();
   };

   // _____________________________________________________________________________

   /** \brief Helper class to calculate data rate.
    *
    * \ingroup dabc_all_classes
    *
    * Should not be mixed up with \ref dabc::Parameter class,
    * which could measure data rate as well
    */

   class Ratemeter {
     public:
        Ratemeter();
        virtual ~Ratemeter();

        void DoMeasure(double interval_sec, long npoints, double firsttm = 0.);

        void Packet(int size, double tm = 0.);
        void Reset();

        double GetRate();
        double GetTotalTime();
        int AverPacketSize();

        int GetNumOper() const { return numoper; }

        void SaveInFile(const char* fname);

        static void SaveRatesInFile(const char* fname, Ratemeter** rates, int nrates, bool withsum = false);

     protected:
        double firstoper, lastoper;
        int64_t numoper, totalpacketsize;

        double fMeasureInterval; // interval between two points
        long fMeasurePoints;
        double* fPoints;
   };

   // ___________________________________________________________________________________

   /** \brief Helper class to calculate average value.
    *
    * \ingroup dabc_all_classes
    *
    * Should not be mixed up with \ref dabc::Parameter class,
    * which could measure average value as well
    */

   class Average {
      public:
         Average();
         virtual ~Average();
         void AllocateHist(int nbins, double xmin, double xmax);
         void ShowHist();
         void Reset();
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
         long* hist; // histogram, same as in root hist[0], undeflow, hist[n+1] - overflow
         int nhist;
         double hist_min;
         double hist_max;


   };

}


#endif
