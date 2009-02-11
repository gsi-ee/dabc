#include "dabc/CpuInfoModule.h"

dabc::CpuInfoModule::CpuInfoModule(const char* name, dabc::Command* cmd) :
   dabc::ModuleAsync(name, cmd),
   fStat(true)
{
   double period = GetCfgDouble("Period", 2., cmd);
   fKind = GetCfgInt("Kind", 1 + 2 + 8, cmd);

   CreateTimer("Timer", period);

   SetParDflts(1, false, false);

   if (fStat.NumCPUs() > 0)
      if (fKind & 1)
         CreateRateParameter("CPUutil", true, period, "", "", "%", 0., 100.);
      else
         CreateParDouble("CPUutil", 0.);

   if (fStat.NumCPUs() > 2)
     for (unsigned n=0; n < fStat.NumCPUs() - 1; n++)
        if (fKind & 2)
           CreateParDouble(FORMAT(("CPU%u", n)), 0.);
        else
        if (fKind & 4)
           CreateRateParameter(FORMAT(("CPU%u", n)), false, period, "", "", "%", 0., 100.);

   if (fKind & 8) {
      CreateParInt("VmSize", 0);
      CreateParInt("VmPeak", 0);
      CreateParInt("NumThreads", 0);
   }
}

void dabc::CpuInfoModule::ProcessTimerEvent(Timer* timer)
{
   if (!fStat.Measure()) {
      EOUT(("Cannot measure CPU statistic"));
      return;
   }

   if (fStat.NumCPUs()==0) return;

   SetParDouble("CPUutil", fStat.CPUutil(0)*100.);

   if ((fStat.NumCPUs() > 2) && (fKind & 6))
     for (unsigned n=0; n < fStat.NumCPUs() - 1; n++)
        SetParDouble(FORMAT(("CPU%u", n)), fStat.CPUutil(n+1));

   if (fKind & 8) {
      SetParInt("VmSize", fStat.GetVmSize());
      SetParInt("VmPeak", fStat.GetVmPeak());
      SetParInt("NumThreads", fStat.GetNumThreads());
   }
}
