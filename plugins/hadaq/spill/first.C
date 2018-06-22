// this is example for

#include "hadaq/SpillProcessor.h"


void first()
{
   base::ProcMgr::instance()->SetRawAnalysis(true);
   // base::ProcMgr::instance()->SetTriggeredAnalysis(true);

   // all new instances get this value
   base::ProcMgr::instance()->SetHistFilling(4);

   // this limits used for liner calibrations when nothing else is available
   hadaq::TdcMessage::SetFineLimits(31, 491);

   // create processor
   hadaq::SpillProcessor* proc = new hadaq::SpillProcessor();

   // range which TDC
   proc->SetTdcRange(0x1000, 0x1100);
   
   // 1. level to detect spill on
   // 2. level to detect spill of
   // 3. number of bins in HLD_Hits histogram to analyze
   proc->SetSpillDetect(5000, 1000, 3);
   
   // maximal spill length in seconds
   proc->SetMaxSpillLength(10.);

   // create ROOT file store
   // base::ProcMgr::instance()->CreateStore("td.root");

   // 0 - disable store
   // base::ProcMgr::instance()->SetStoreKind(3);
}
