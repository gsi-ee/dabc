#include "base/ProcMgr.h"
#include "hadaq/HldProcessor.h"
#include "hadaq/TrbProcessor.h"
#include "hadaq/TdcProcessor.h"

#include "TSystem.h"


// Compare calibration files from different paths

void compare(const char *prefix = "local")
{
   // important - number 600 is number of bins in calibration
   hadaq::TdcProcessor::SetDefaults(600, 200, 1);

   std::string dir2name = "22063/";

   auto mgr = new base::ProcMgr;

   auto hld = new hadaq::HldProcessor();
   auto trb31 = new hadaq::TrbProcessor(0x8000, hld);

   auto trb32 = new hadaq::TrbProcessor(0x8100, hld);

   auto dir = gSystem->OpenDirectory(".");

   int cnt = 0;

   while (auto entry = gSystem->GetDirEntry(dir)) {
      std::string s = entry;
      if ((s.length() < 8) || (s.find(".cal") != s.length() - 4))
         continue;
      if (prefix && *prefix && s.find(prefix) != 0)
         continue;

      auto tdcid = std::stoi(s.substr(s.length() - 8, 4).c_str(), nullptr, 16);

      std::string s2 = dir2name + s;

      printf("Files: %s %s tdcid: 0x%x\n", s.c_str(), s2.c_str(), (unsigned) tdcid);

      auto tdc1 = hadaq::TdcProcessor::CreateFromCalibr(trb31, s);

      auto tdc2 = hadaq::TdcProcessor::CreateFromCalibr(trb32, s2);

      if (!tdc1 || !tdc2) {
         fprintf(stderr, "Cannot read calibration for TDC %x\n", tdcid);
         continue;
      }
      if (tdc1->GetID() != tdc2->GetID()) {
         fprintf(stderr, "IDs mismatch %u %u\n", tdc1->GetID(), tdc2->GetID());
         continue;
      }

      if (tdc1->NumChannels() != tdc2->NumChannels()) {
         fprintf(stderr, "%s channels mismatch %u %u\n", tdc1->GetName(), tdc1->NumChannels(), tdc2->NumChannels());
         continue;
      }

      for (unsigned n = 1; n < tdc1->NumChannels(); ++n) {
         double tot1 = tdc1->GetChannelTotShift(n);
         double tot2 = tdc2->GetChannelTotShift(n);
         if (std::abs(tot1 - tot2) > 0.1)
            fprintf(stderr, "%s channel %2u ToT mismatch %6.3f %6.3f diff %7.3f\n", tdc1->GetName(), n, tot1, tot2, std::abs(tot1 - tot2));
      }

      if (cnt > 5) break;
   }

   printf("Totally processed are %d files\n", cnt);

   gSystem->FreeDirectory(dir);
}
