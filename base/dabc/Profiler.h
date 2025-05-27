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

#ifndef DABC_defines
#include "dabc/defines.h"
#endif

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#include <vector>

#ifdef DABC_PROFILER

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

      std::string Format();

   };

   class ProfilerGuard {
      Profiler &fProfiler;
      unsigned fCnt{0};
      Profiler::clock_t fLast{0};

   public:
      ProfilerGuard(Profiler &prof, const char *name = nullptr, unsigned lvl = 0) : fProfiler(prof), fCnt(lvl)
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

      void Next(const char *name = nullptr, unsigned lvl = 0)
      {
         if (!fProfiler.fActive || (fCnt >= fProfiler.fEntries.size()))
            return;

         auto now = fProfiler.GetClock();

         fProfiler.fEntries[fCnt].fSum += (now - fLast);

         fLast = now;

         if (lvl) fCnt = lvl; else fCnt++;

         if (name && (fCnt < fProfiler.fEntries.size()) && fProfiler.fEntries[fCnt].fName.empty())
            fProfiler.fEntries[fCnt].fName = name;
      }

   };

} // namespace dabc

#else

// dummy profiler implementation
namespace dabc {

   class ProfilerGuard;

   class Profiler {

   public:

      Profiler() { }

      void Reserve(unsigned = 10)
      {
      }

      void SetActive(bool = true) {  }

      void MakeStatistic() {}

      std::string Format() { return ""; }

   };


   class ProfilerGuard {

   public:
      ProfilerGuard(Profiler &, const char * = nullptr, unsigned = 0)
      {
      }

      void Next(const char * = nullptr, unsigned = 0)
      {
      }

   };

} // namespace dabc


#endif

#endif
