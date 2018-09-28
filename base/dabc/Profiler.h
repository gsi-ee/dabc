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

#ifndef DABC_Profiler
#define DABC_Profiler

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#include <string>
#include <vector>

namespace dabc {

   class ProfilerGuard;

   class Profiler {

      friend class ProfilerGuard;

      typedef unsigned long long clock_t;

      bool fActive{true};

      clock_t GetClock() { return TimeStamp::GetFastClock(); }

      clock_t fLast{0};

      struct Entry {
         clock_t fSum{0};           // sum of used time
         double fRatio{0.};          // rel time for this slot
         std::string fName;
      };

      std::vector<Entry>  fEntries;

   public:

      Profiler() { Reserve(10); }

      void Reserve(unsigned num = 10)
      {
         while (fEntries.size() < num)
            fEntries.emplace_back();
      }

      void SetActive(bool on = true) { fActive = on; }

      void MakeStatistic();

   };

   class ProfilerGuard {
      Profiler &fProfiler;
      unsigned fCnt;
      Profiler::clock_t fLast;

   public:
      ProfilerGuard(Profiler &prof, unsigned lvl = 0, const char *name = nullptr) : fProfiler(prof), fCnt(lvl)
      {
         if (!fProfiler.fActive || (fCnt >= fProfiler.fEntries.size()))
            return;

         fLast = fProfiler.GetClock();

         if (name && fProfiler.fEntries[fCnt].fName.empty())
            fProfiler.fEntries[fCnt].fName = name;
      }

      ~ProfilerGuard()
      {
         Next();
      }

      void Next(const char *name = nullptr)
      {
         if (!fProfiler.fActive || (fCnt >= fProfiler.fEntries.size()))
            return;

         auto now = fProfiler.GetClock();

         auto &entry = fProfiler.fEntries[fCnt];
         entry.fSum += (now - fLast);
         if (name && entry.fName.empty())
            entry.fName = name;
         fLast = now;

         fCnt++;
      }

   };

} // namespace dabc

#endif
