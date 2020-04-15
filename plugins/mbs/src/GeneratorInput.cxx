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

#include "mbs/GeneratorInput.h"

#include <cstdlib>
#include <cmath>

#include "dabc/Application.h"
#include "dabc/Manager.h"
#include "mbs/Iterator.h"


double Gauss_Rnd(double mean, double sigma)
{

   double x, y, z;

//   srand(10);

   z = 1.* rand() / RAND_MAX;
   y = 1.* rand() / RAND_MAX;

   x = z * 6.28318530717958623;
   return mean + sigma*sin(x)*sqrt(-2*log(y));
}


mbs::GeneratorInput::GeneratorInput(const dabc::Url& url) :
   dabc::DataInput(),
   fEventCount(0),
   fNumSubevents(2),
   fFirstProcId(0),
   fSubeventSize(32),
   fIsGo4RandomFormat(true),
   fFullId(0),
   fTotalSize(0),
   fTotalSizeLimit(0),
   fGenerTimeout(0.)
{
   fEventCount = url.GetOptionInt("first", 0);

   fNumSubevents = url.GetOptionInt("numsub", 2);
   fFirstProcId = url.GetOptionInt("procid", 0);
   fSubeventSize = url.GetOptionInt("size", 32);
   fIsGo4RandomFormat = url.GetOptionBool("go4", true);
   fFullId = url.GetOptionInt("fullid", 0);
   fTotalSizeLimit = url.GetOptionInt("total", 0);
   fGenerTimeout = url.GetOptionInt("tmout", 0)*0.001;
   fTimeoutSwitch = false;
}

bool mbs::GeneratorInput::Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   return dabc::DataInput::Read_Init(wrk,cmd);
}

unsigned mbs::GeneratorInput::Read_Size()
{
   if (fGenerTimeout > 0.) {
      fTimeoutSwitch = !fTimeoutSwitch;
      if (fTimeoutSwitch) return dabc::di_RepeatTimeOut;
   }

   if ((fTotalSizeLimit>0) && (fTotalSize / 1024. / 1024. > fTotalSizeLimit)) return dabc::di_EndOfStream;

   return dabc::di_DfltBufSize;
}


unsigned mbs::GeneratorInput::Read_Complete(dabc::Buffer& buf)
{

   mbs::WriteIterator iter(buf);

   while (iter.NewEvent(fEventCount)) {

      bool eventdone = true;

      for (unsigned subcnt = 0; subcnt < fNumSubevents; subcnt++)
         if (iter.NewSubevent(fSubeventSize, fFirstProcId + subcnt + 1, fFirstProcId + subcnt)) {

            if ((fFullId!=0) && (fNumSubevents==1))
               iter.subevnt()->fFullId = fFullId;

            unsigned subsz = fSubeventSize;

            uint32_t* value = (uint32_t*) iter.rawdata();

            if (fIsGo4RandomFormat) {
               unsigned numval = fSubeventSize / sizeof(uint32_t);
               for (unsigned nval=0;nval<numval;nval++)
                  *value++ = (uint32_t) Gauss_Rnd(nval*100 + 2000, 500./(nval+1));

               subsz = numval * sizeof(uint32_t);
            } else {
               if (subsz>0) *value++ = fEventCount;
               if (subsz>4) *value++ = fFirstProcId + subcnt;
            }

            iter.FinishSubEvent(subsz);
         } else {
            eventdone = false;
            break;
         }

      if (!eventdone) break;

      if (!iter.FinishEvent()) break;

      fEventCount++;
   }

   // When close iterator - take back buffer with correctly modified length field
   buf = iter.Close();

   fTotalSize += buf.GetTotalSize();

   return dabc::di_Ok;
}
