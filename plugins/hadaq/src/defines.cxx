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

#include "hadaq/defines.h"

#include <stdio.h>
#include <sys/time.h>
#include <time.h>


void hadaq::RawEvent::Dump()
{
   printf("*** Event #0x%06x fullid=0x%04x runid=0x%08x size %u *** \n",
         (unsigned) GetSeqNr(), (unsigned) GetId(), (unsigned) GetRunNr(), (unsigned) GetSize());
}

void hadaq::RawEvent::InitHeader(uint32_t id)
{
   tuDecoding = EvtDecoding_64bitAligned;
   SetId(id);
   SetSize(sizeof(hadaq::RawEvent));
   // timestamp at creation of structure:
   time_t tempo = time(NULL);
   struct tm* gmTime = gmtime(&tempo);
   uint32_t date = 0, clock = 0;
   date |= gmTime->tm_year << 16;
   date |= gmTime->tm_mon << 8;
   date |= gmTime->tm_mday;
   SetDate(date);
   clock |= gmTime->tm_hour << 16;
   clock |= gmTime->tm_min << 8;
   clock |= gmTime->tm_sec;
   SetTime(clock);
}

uint32_t hadaq::RawEvent::CreateRunId()
{
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return tv.tv_sec - hadaq::HADAQ_TIMEOFFSET;
}


std::string hadaq::RawEvent::FormatFilename (uint32_t runid, uint16_t ebid)
{
   // same
   char buf[128];
   time_t iocTime = runid + HADAQ_TIMEOFFSET;
   struct tm tm_res;
   size_t off = strftime(buf, 128, "%y%j%H%M%S", localtime_r(&iocTime, &tm_res));
   if(ebid!=0) snprintf(buf+off, 128-off, "%02d", ebid);
   return std::string(buf);
}



int hadaq::RawEvent::CoreAffinity(pid_t pid)
{
  // stolen from stats.c of old eventbuilders JAM
   unsigned long new_mask = 2;
   unsigned int len = sizeof(new_mask);
   cpu_set_t cur_mask;
   CPU_ZERO(&cur_mask);
   if(pid)
   {
      // this one for whole process:
      sched_getaffinity(pid, len, &cur_mask);
   }
   else
   {
      // try if we can find out current thread affinity:
      pthread_t thread=pthread_self();
      int r=pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cur_mask);
      if(r)
      {
         printf("Error %d in pthread_getaffinity_np for thread 0x%x!\n",r, (unsigned) thread);
      }
   }


   int i;
   for (i = 0; i < 24; i++) {
      int cpu;
      cpu = CPU_ISSET(i, &cur_mask);
      if (cpu > 0)
         break;
   }
   return i;
}








hadaq::RawSubevent* hadaq::RawEvent::NextSubevent(RawSubevent* prev)
{
   if (prev == 0) {

      if (GetSize() - sizeof(hadaq::RawEvent) < sizeof(hadaq::RawSubevent)) return 0;

      return (hadaq::RawSubevent*) ((char*) this + sizeof(hadaq::RawEvent));
   }

   char* next = (char*) prev + prev->GetPaddedSize();

   if (next >= (char*) this + GetSize()) return 0;

   return (hadaq::RawSubevent*) next;
}

// ===========================================================

void hadaq::RawSubevent::PrintRawData(unsigned ix, unsigned len, unsigned prefix)
{
   unsigned sz = ((GetSize() - sizeof(RawSubevent)) / Alignment());

   if ((ix>=sz) || (len==0)) return;
   if (ix + len > sz) len = sz - ix;

   unsigned wlen = 2;
   if (len>99) wlen = 3; else
   if (len>999) wlen = 4;

   for (unsigned cnt=0;cnt<len;cnt++,ix++)
      printf("%*s[%*u] %08x%s", (cnt%8 ? 2 : 2+prefix), "", wlen, ix, (unsigned) Data(ix), (cnt % 8 == 7 ? "\n" : ""));

   if (len % 8 != 0) printf("\n");
}


void hadaq::RawSubevent::Dump(bool print_raw_data)
{
   printf("   *** Subevent size %3u decoding 0x%06x id 0x%04x trig 0x%08x %s align %u *** \n",
             (unsigned) GetSize(),
             (unsigned) GetDecoding(),
             (unsigned) GetId(),
             (unsigned) GetTrigNr(),
             IsSwapped() ? "swapped" : "not swapped",
             (unsigned) Alignment());

   if (print_raw_data) PrintRawData();
}
