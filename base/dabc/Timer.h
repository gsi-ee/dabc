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

#ifndef DABC_Timer
#define DABC_Timer

#ifndef DABC_ModuleItem
#include "dabc/ModuleItem.h"
#endif

namespace dabc {
    
   /** Provides timer event to the module
     * Main aim of this class is to generate periodical or single "shoot" events,
     * which can be used in module for different purposes.
     * If period parameter is positive, timer will produces periodical events.
     * Timer can work in synchrone or asynchrone mode. In first case it will try to produce
     * as many events per second as it should - 1/Period. In case if some events
     * postponed, next events comes faster.  
     * In asynchron mode period only specifies minimum distance between two events,
     * therefore number of events per second can be less than 1s/Period.
     * One can also makes single shoots of the timer for activate timer event 
     * only once. If delay==0, event will be activated as soon as possible, 
     * delay>0 - after specified interval (in seconds). When delay<0, no shoot 
     * will be done and previous shoot will be canceled. If SingleShoot() was called
     * several times before event is produced, only last call has an effect.
     */

   class Timer : public ModuleItem {

      friend class Module;

      protected:
         double      fPeriod;   // period of timer events 
         long        fCounter;  // number of generated events
         bool        fActive;   // is timer active
         bool        fSynhron; // indicate if timer tries to keep number of events per second
         double      fInaccuracy; // accumulated inaccuracy of timer in synchron mode
      public:
         Timer(Reference parent, const char* name, double timeout = -1., bool synchron = false);
         virtual ~Timer();
         
         double GetPeriod() const { return fPeriod; }
         void SetPeriod(double v) { fPeriod = v; }
         
         bool IsSynchron() const { return fSynhron; }
         void SetSynchron(bool on = true) { fSynhron = on; }
         
         long GetCounter() const { return fCounter; }

         void SingleShoot(double delay = 0.);
      
      protected:  

         virtual void DoStart();
         virtual void DoStop();
         
         virtual double ProcessTimeout(double last_diff);
   };
   
}

#endif
