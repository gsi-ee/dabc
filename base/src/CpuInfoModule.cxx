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

#include "dabc/CpuInfoModule.h"

dabc::CpuInfoModule::CpuInfoModule(const std::string& name, dabc::Command cmd, int kind) :
   dabc::ModuleAsync(name, cmd),
   fStat(true),
   fKind(kind)
{
   double period = Cfg("Period",cmd).AsDouble(2);
   if (fKind<0) fKind = Cfg("Kind",cmd).AsInt(1 + 2);

   CreateTimer("Timer", period);

   if (fStat.NumCPUs() > 0) {
      CreatePar("CPUutil").SetLimits(0, 100.).SetUnits("%");
      if (fKind & 1)
         Par("CPUutil").SetAverage(true, period);
   }
   if (fStat.NumCPUs() > 2) {
     for (unsigned n=0; n < fStat.NumCPUs() - 1; n++) {
        Parameter par = CreatePar(dabc::format("CPU%u", n)).SetLimits(0, 100.).SetUnits("%");
        if (fKind & 4) par.SetAverage(true, period);
     }
   }

   CreatePar("VmSize").SetLimits(0, 16000.).SetUnits("KB");
   Par("VmSize").SetAverage(true, period);

//      CreatePar("VmPeak").SetInt(0);
//      CreatePar("NumThreads").SetInt(0);
}

void dabc::CpuInfoModule::ProcessTimerEvent(unsigned timer)
{
   if (!fStat.Measure()) {
      EOUT("Cannot measure CPU statistic");
      return;
   }

   if (fStat.NumCPUs()==0) return;

   Par("CPUutil").SetDouble(fStat.CPUutil(0)*100.);

   if ((fStat.NumCPUs() > 2) && (fKind & 6))
     for (unsigned n=0; n < fStat.NumCPUs() - 1; n++)
        Par(dabc::format("CPU%u", n)).SetDouble(fStat.CPUutil(n+1)*100.);

   Par("VmSize").SetDouble(fStat.GetVmSize());
//   Par("VmPeak").SetInt(fStat.GetVmPeak());
//   Par("NumThreads").SetInt(fStat.GetNumThreads());
}
