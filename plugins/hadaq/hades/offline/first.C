#include "base/ProcMgr.h"
#include "hadaq/HldProcessor.h"
#include "hadaq/TdcProcessor.h"
#include "hadaq/TrbProcessor.h"

#include <fstream>
#include <map>
#include <stdio.h>

std::map<unsigned, int> fTdcModes;

void read_db(const char *fname = "file.db")
{
   std::ifstream f(fname);

   if (!f) {
      printf("Fail to read DB file %s\n", fname);
      exit(2);
   }

   bool found_separ = false;

   for (std::string line; std::getline(f, line); ) {
      if (!found_separ) {

         if (line.find("#lxhadebXX") == 0)
            found_separ = true;
         continue;
      }

      while ((line.length() > 0) && line[0] == ' ')
         line.erase(0, 1);

      if (line.find("# hub setup") == 0)
         break;

      if ((line.length() == 0) || (line[0] == '#'))
         continue;

      unsigned id = 0, v1 = 0, v2 = 0, v3 = 0, v4 = 0, v5 = 0;
      int i1 = 0, mode = 0;

      int res = sscanf(line.c_str(), "0x%x %d 0x%x 0x%x 0x%x 0x%x 0x%x %d", &id, &i1, &v1, &v2, &v3, &v4, &v5, &mode);

      if (res == 8) {

         if ((mode != 0) && (mode != 2)) {
            std::cout << line << std::endl;
            printf("0x%04x  %d\n", id, mode);
         }

         if (mode > 0)
            fTdcModes[id] = mode;
      }
   }

   if (!found_separ) {
      printf("Fail to find separator in DB file %s\n", fname);
      exit(2);
   }
}

float getTotRms(unsigned id)
{
   if ((id & 0xfff0) == 0x84c0) return 0.3;
   if (id == 0x86c1) return 0.4;
   if ((id & 0xfff0) == 0x86c0) return 0.3;
   if (id == 0x8b15) return 0.4;
   if ((id & 0xfff0) == 0x8b10) return 0.4;
   if ((id & 0xfff0) == 0x8b00) return 0.4;
   if ((id & 0xfff0) == 0x8d00) return 0.4;
   return 0.2; // default
}

void first()
{
   read_db();

   base::ProcMgr::instance()->SetRawAnalysis(true);
   // base::ProcMgr::instance()->SetTriggeredAnalysis(true);

   // all new instances get this value
   base::ProcMgr::instance()->SetHistFilling(1);

   // this limits used for liner calibrations when nothing else is available
   hadaq::TdcMessage::SetFineLimits(31, 480);

   // default channel numbers and edges mask
   // 1 - use only rising edge, falling edge is ignore
   // 2   - falling edge enabled and fully independent from rising edge
   // 3   - falling edge enabled and uses calibration from rising edge
   // 4   - falling edge enabled and common statistic is used for calibration
   hadaq::TrbProcessor::SetDefaults(33, 4);

   // [min..max] range for TDC ids
   hadaq::TrbProcessor::SetTDCRange(0x5000, 0x8000);

   // [min..max] range for HUB ids
   hadaq::TrbProcessor::SetHUBRange(0x8100, 0x81FF);

   // ignore any calibration messages in the file
   hadaq::TdcProcessor::SetIgnoreCalibrMsgs(true);

   // when first argument true - TRB/TDC will be created on-the-fly
   // second parameter is function name, called after elements are created
   hadaq::HldProcessor* hld = new hadaq::HldProcessor(true, "after_create");


   hld->SetCustomNumCh([](unsigned id) -> unsigned {
      if ((id & 0xfff0) == 0x8880) return 49;
      if ((id & 0xfff0) == 0x8890) return 49;
      if ((id & 0xff00) == 0x8a00) return 49;
      if ((id & 0xfff0) == 0x84c0) return 49;
      if ((id & 0xfff0) == 0x8b10) return 49;
      if ((id & 0xfff0) == 0x8b00) return 49;
      if ((id & 0xfff0) == 0x8d00) return 49;
      return 0; // use default
   });

   // first parameter if filename  prefix for calibration files
   //     and calibration mode (empty string - no file I/O)
   // second parameter is hits count for autocalibration
   //     0 - only load calibration
   //    -1 - accumulate data and store calibrations only at the end
   //    -77 - accumulate data and store linear calibrations only at the end
   //    >0 - automatic calibration after N hits in each active channel
   //         if value ends with 77 like 10077 linear calibration will be calculated
   //    >1000000000 - automatic calibration after N hits only once, 1e9 excluding
   // third parameter is trigger type mask used for calibration
   //   (1 << 0xD) - special 0XD trigger with internal pulser, used also for TOT calibration
   //    0x3FFF - all kinds of trigger types will be used for calibration (excluding 0xE and 0xF)
   //   0x80000000 in mask enables usage of temperature correction
   hld->ConfigureCalibration("local", 0, (1 << 0xD));

   // only accept trigger type 0x1 when storing file
   // new hadaq::HldFilter(0x1);

   // create ROOT file store
   // base::ProcMgr::instance()->CreateStore("td.root");

   // 0 - disable store
   // 1 - std::vector<hadaq::TdcMessageExt> - includes original TDC message
   // 2 - std::vector<hadaq::MessageFloat>  - compact form, without channel 0, stamp as float (relative to ch0)
   // 3 - std::vector<hadaq::MessageDouble> - compact form, with channel 0, absolute time stamp as double
   base::ProcMgr::instance()->SetStoreKind(0);


   // when configured as output in DABC, one specifies:
   // <OutputPort name="Output2" url="stream://file.root?maxsize=5000&kind=3"/>


}

// extern "C" required by DABC to find function from compiled code

extern "C" void after_create(hadaq::HldProcessor* hld)
{
   if (!hld) return;

   printf("Called after all sub-components are created numTDC %d\n", hld->NumberOfTDC());

   for (unsigned l = 0; l < hld->NumberOfTRB(); l++) {
      hadaq::TrbProcessor* trb = hld->GetTRB(l);
      if (!trb) continue;
      printf("Configure %s NumTDCs %u\n", trb->GetName(), trb->NumberOfTDC());
      trb->SetPrintErrors(10);

      int mode = fTdcModes.find(trb->GetID()) == fTdcModes.end() ? 0 : fTdcModes[trb->GetID()];

      if ((trb->NumberOfTDC() > 0) && (mode == 0))
         printf("!!!!!!!!!!!!!!! NO TDC are expected !!!!!!!!!!!!!!!!\n");

      for (unsigned k = 0; k < trb->NumberOfTDC(); k++) {
         hadaq::TdcProcessor* tdc = trb->GetTDCWithIndex(k);
         if (!tdc) continue;

         printf("   Configure %s calibr_mode %d \n", tdc->GetName(), mode);

         tdc->SetTotStatLimit(100);
         tdc->SetTotRMSLimit(getTotRms(trb->GetID()));

         if (mode % 10 == 1) {
            tdc->SetUseLinear();
            tdc->SetLinearNumPoints(2);
         }

         if ((mode >= 10) && (mode / 10 == 1))
            tdc->SetToTRange(20., 30., 60.); // special mode for DiRICH

         if ((tdc->GetID() >= 0x5000) && (tdc->GetID() < 0x5008)) {
            tdc->SetPairedChannels(true);
            tdc->SetToTRange(30., 20., 65.);
         }
      }
   }
}

