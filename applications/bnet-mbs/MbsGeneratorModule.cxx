#include "MbsGeneratorModule.h"

#include "dabc/logging.h"
#include "dabc/Buffer.h"
#include "mbs/MbsTypeDefs.h"

void bnet::MbsGeneratorModule::GeneratePacket(dabc::Buffer* buf)
{
   int evid = fEventCnt; 
    
   mbs::GenerateMbsPacket(buf, UniqueId(), evid, 480, 4);
   
   fEventCnt = evid;
}
