// this is example for

#include "hadaq/StartProcessor.h"


void first()
{
   base::ProcMgr::instance()->SetRawAnalysis(true);
   // base::ProcMgr::instance()->SetTriggeredAnalysis(true);

   // all new instances get this value
   base::ProcMgr::instance()->SetHistFilling(4);

   // this limits used for liner calibrations when nothing else is available
   hadaq::TdcMessage::SetFineLimits(31, 491);

   // create processor
   hadaq::StartProcessor* proc = new hadaq::StartProcessor();

   // subevent ID from start
   proc->SetStartId(0x8880);

   // range which TDC
   proc->SetTdcRange(0x5000, 0x5010);

}
