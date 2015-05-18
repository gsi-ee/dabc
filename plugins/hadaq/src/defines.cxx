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
   char sbuf[50];
   if (GetSize()!=GetPaddedSize())
      snprintf(sbuf,sizeof(sbuf), "%u+%u", (unsigned) GetSize(), (unsigned) (GetPaddedSize() - GetSize()));
   else
      snprintf(sbuf,sizeof(sbuf),"%u", (unsigned) GetSize());

   printf("*** Event #0x%06x fullid=0x%04x runid=0x%08x size %s *** \n",
         (unsigned) GetSeqNr(), (unsigned) GetId(), (unsigned) GetRunNr(), sbuf);
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
   char sbuf[50];
   if (GetSize()!=GetPaddedSize())
      snprintf(sbuf,sizeof(sbuf), "%4u+%u", (unsigned) GetSize(), (unsigned) (GetPaddedSize() - GetSize()));
   else
      snprintf(sbuf,sizeof(sbuf),"%6u", (unsigned) GetSize());
   printf("   *** Subevent size %s decoding 0x%06x id 0x%04x trig 0x%08x %s align %u *** \n",
             sbuf,
             (unsigned) GetDecoding(),
             (unsigned) GetId(),
             (unsigned) GetTrigNr(),
             IsSwapped() ? "swapped" : "not swapped",
             (unsigned) Alignment());

   if (print_raw_data) PrintRawData();
}
