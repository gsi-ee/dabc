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


#include "dabc/Profiler.h"

#include "dabc/string.h"


void dabc::Profiler::MakeStatistic()
{

   clock_t now = GetClock();

   clock_t diff = fLast ? now - fLast : 0;

   if (diff) {
      for (auto &&entry : fEntries) {
         entry.fRatio = 1.*entry.fSum/diff;
         entry.fSum = 0;
      }
   }

   fLast = now;
}
