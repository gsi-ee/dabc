#include "LmdTypeDefs.h"

unsigned mbs::EventHeader::NumSubevents() const
{
   unsigned cnt = 0;
   SubeventHeader* sub = 0;
   while ((sub = NextSubEvent(sub)) != 0) cnt++;
   return cnt;
}             

void mbs::BufferHeader::Init(bool newformat)
{
   iWords = 0; 
   iType = newformat ? MBS_TYPE_100_1 : MBS_TYPE_10_1;
   iBufferId = 0;
   iNumEvents = 0;
   iTemp = 0;
   iSeconds = 0;
   iNanosec = 0;
   iEndian = 0; 
   iLast = 0;
   iUsedWords = 0;
   iFree3 = 0;
   SetEndian();
}

uint32_t mbs::BufferHeader::UsedBufferSize()
{
   switch (iType) {
      // new buffer type
      case MBS_TYPE_100_1: return iUsedWords * 2;
         
      // old buffer type 
      case MBS_TYPE_10_1: return i_used * 2;
         
      default: break;
//         EOUT(("Uncknown buffer type %d-%d", i_type, i_subtype));
   }
   
   return 0; 

}

void mbs::BufferHeader::SetUsedBufferSize(uint32_t len)
{
   switch (iType) {
      // new buffer type
      case MBS_TYPE_100_1:
         iUsedWords = len / 2;
         if (iWords==0) SetFullSize(len + sizeof(BufferHeader));
         
         break;
         
      // old buffer type 
      case MBS_TYPE_10_1: 
         i_used = len / 2;
         if (iWords==0) SetFullSize(len + sizeof(BufferHeader));
         break;
         
      default:   
//         EOUT(("Uncknown buffer type %d-%d", i_type, i_subtype));
        break;
   }
}
