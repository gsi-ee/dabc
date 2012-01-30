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

#include "dabc/Timer.h"

#include "dabc/logging.h"

dabc::Timer::Timer(Reference parent, const char* name, double period, bool synchron) :
   ModuleItem(mitTimer, parent, name),
   fPeriod(period),
   fCounter(0),
   fActive(false),
   fSynhron(synchron),
   fInaccuracy(0.)
{
}

dabc::Timer::~Timer()
{
}

void dabc::Timer::SingleShoot(double delay)
{
   ActivateTimeout(delay);
}

void dabc::Timer::DoStart()
{
   fActive = true;
   
   if (fPeriod>0.)    
      ActivateTimeout(fPeriod);
}

void dabc::Timer::DoStop()
{
   fActive = false;
   
   ActivateTimeout(-1);
}

double dabc::Timer::ProcessTimeout(double last_diff)
{
   if (!fActive) return -1.;
    
   fCounter++; 
   
   ProduceUserEvent(evntTimeout);
   
   // if we have not synchron timer, just fire next event
   if (!fSynhron) return fPeriod;

   fInaccuracy += (last_diff - fPeriod);
   
   double res = fPeriod - fInaccuracy;
   if (res < 0) res = 0;

   //DOUT0(("Timer %s lastdif = %5.4f next %5.4f", GetName(), last_diff, res));

   return res;
}
