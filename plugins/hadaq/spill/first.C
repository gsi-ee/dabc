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
   proc->SetSpillDetect(0, 2, 3);
   
   // maximal spill length in seconds
   proc->SetMaxSpillLength(10.);

  // set channels lookup table
  // 1. channel id
  // 2. strip id X:0..15 Y:100..115
/*
   proc->SetChannelsLookup(0,  1 );
   proc->SetChannelsLookup(1,  3 );
   proc->SetChannelsLookup(2,  5 );
   proc->SetChannelsLookup(3,  7 );
   proc->SetChannelsLookup(4,  9 );
   proc->SetChannelsLookup(5,  11);
   proc->SetChannelsLookup(6,  13);
   proc->SetChannelsLookup(7,  15);
   proc->SetChannelsLookup(8,  17);
   proc->SetChannelsLookup(9,  19);
   proc->SetChannelsLookup(10, 21);
   proc->SetChannelsLookup(11, 23);
   proc->SetChannelsLookup(12, 25);
   proc->SetChannelsLookup(13, 27);
   proc->SetChannelsLookup(14, 29);
   proc->SetChannelsLookup(15, 31);
*/
                                  
   proc->SetChannelsLookup1(1 ,1  );
   proc->SetChannelsLookup1(3 ,2  );
   proc->SetChannelsLookup1(5 ,3  );
   proc->SetChannelsLookup1(7 ,4  );
   proc->SetChannelsLookup1(9 ,5  );
   proc->SetChannelsLookup1(11,6  );
   proc->SetChannelsLookup1(13,7  );
   proc->SetChannelsLookup1(15,8  );
   proc->SetChannelsLookup1(17,9  );
   proc->SetChannelsLookup1(19,10 );
   proc->SetChannelsLookup1(21,11 );
   proc->SetChannelsLookup1(23,12 );
   proc->SetChannelsLookup1(25,13 );
   proc->SetChannelsLookup1(27,14 );
   proc->SetChannelsLookup1(29,15 );
   proc->SetChannelsLookup1(31,16 );
                             
   proc->SetChannelsLookup2(1 ,101   );
   proc->SetChannelsLookup2(3 ,102   );
   proc->SetChannelsLookup2(5 ,103   );
   proc->SetChannelsLookup2(7 ,104   );
   proc->SetChannelsLookup2(9 ,105   );
   proc->SetChannelsLookup2(11,106   );
   proc->SetChannelsLookup2(13,107   );
   proc->SetChannelsLookup2(15,108   );
   proc->SetChannelsLookup2(17,109   );
   proc->SetChannelsLookup2(19,110  );
   proc->SetChannelsLookup2(21,111 );
   proc->SetChannelsLookup2(23,112 );
   proc->SetChannelsLookup2(25,113 );
   proc->SetChannelsLookup2(27,114 );
   proc->SetChannelsLookup2(29,115 );
   proc->SetChannelsLookup2(31,116 );

   proc->SetChannelsLookup3(1 ,201   );
   proc->SetChannelsLookup3(3 ,202   );
   proc->SetChannelsLookup3(5 ,203   );
   proc->SetChannelsLookup3(7 ,204   );
   proc->SetChannelsLookup3(9 ,205   );
   proc->SetChannelsLookup3(11,206   );
   proc->SetChannelsLookup3(13,207   );
   proc->SetChannelsLookup3(15,208   );
   proc->SetChannelsLookup3(17,209   );
   proc->SetChannelsLookup3(19,210  );
   proc->SetChannelsLookup3(21,211 );
   proc->SetChannelsLookup3(23,212 );
   proc->SetChannelsLookup3(25,213 );
   proc->SetChannelsLookup3(27,214 );
   proc->SetChannelsLookup3(29,215 );
   proc->SetChannelsLookup3(31,216 );


                            
   // create ROOT file store
   // base::ProcMgr::instance()->CreateStore("td.root");

   // 0 - disable store
   // base::ProcMgr::instance()->SetStoreKind(3);
}
