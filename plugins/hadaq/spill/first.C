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
   proc->SetTdcRange(0xc000, 0xc100);

   // create ROOT file store
   // base::ProcMgr::instance()->CreateStore("td.root");

   // 0 - disable store
   // base::ProcMgr::instance()->SetStoreKind(3);
}
