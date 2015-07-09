
void first()
{

   base::ProcMgr::instance()->SetRawAnalysis(true);

   // this limits used for liner calibrations when nothing else is available
   hadaq::TdcMessage::SetFineLimits(31, 421);

   // default channel numbers and edges mask
   hadaq::TrbProcessor::SetDefaults(33, 0x1);

   hadaq::HldProcessor* hld = new hadaq::HldProcessor();

   // About time calibration - there are two possibilities
   // 1) automatic calibration after N hits in every enabled channel.
   //     Just use SetAutoCalibrations method for this
   // 2) generate calibration on base of provided data and than use it later statically for analysis
   //     Than one makes special run with SetWriteCalibrations() enabled.
   //     Later one reuse such calibrations enabling only LoadCalibrations() call

   hadaq::TrbProcessor* trb3_1 = new hadaq::TrbProcessor(0x0001, hld);
   trb3_1->SetHistFilling(4);
   trb3_1->SetCrossProcess(false);
   trb3_1->CreateTDC(0xC000, 0xC001, 0xC002, 0xC003);
   // enable automatic calibration, specify required number of hits in each channel
   trb3_1->SetAutoCalibrations(50000);
   // calculate and write static calibration at the end of the run
   //trb3_1->SetWriteCalibrations("run1");
   // load static calibration at the beginning of the run
   //trb3_1->LoadCalibrations("run1");

   hadaq::TrbProcessor* trb3_2 = new hadaq::TrbProcessor(0x0002, hld);
   trb3_2->SetHistFilling(4);
   trb3_2->SetCrossProcess(true);
   trb3_2->CreateTDC(0xC010, 0xC011, 0xC012, 0xC013);
   trb3_2->SetAutoCalibrations(50000);
   //trb3_2->SetWriteCalibrations("run1");
   //trb3_2->LoadCalibrations("run1");

   // this is array with available TDCs ids
   int tdcmap[8] = { 0xC000, 0xC001, 0xC002, 0xC003, 0xC010, 0xC011, 0xC012, 0xC013 };

   // TDC subevent header id

   for (int cnt=0;cnt<8;cnt++) {

      hadaq::TdcProcessor* tdc = hld->FindTDC(tdcmap[cnt]);
      if (tdc==0) continue;

      // specify reference channel
      tdc->SetRefChannel(3, 1, 0xffff, 2000,  -10., 10., true);
      tdc->SetRefChannel(4, 2, 0xffff, 2000,  -10., 10., true);

      if (cnt > 0) {
         // specify reference channel from other TDC
         tdc->SetRefChannel(0, 0, 0xC000, 2000,  -10., 10., true);
         tdc->SetRefChannel(5, 5, 0xC000, 2000,  -10., 10., true);
         tdc->SetRefChannel(7, 7, 0xC000, 2000,  -10., 10., true);
      }

      // for old FPGA code one should have epoch for each hit, no longer necessary
      // tdc->SetEveryEpoch(true);

      // When enabled, time of last hit will be used for reference calculations
      // tdc->SetUseLastHit(true);

   }

}



