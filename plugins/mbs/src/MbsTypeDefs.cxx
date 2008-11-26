#include "MbsTypeDefs.h"

#include <string.h>
#include <byteswap.h>

const char* mbs::xmlFileName          = "FileName";
const char* mbs::xmlSizeLimit         = "SizeLimit";
const char* mbs::xmlFirstMultiple     = "FirstMultiple";
const char* mbs::xmlNumMultiple       = "NumMultiple";

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

uint32_t mbs::BufferHeader::BufferLength() const
{
   switch (iType) {
      // new buffer type
      case MBS_TYPE_100_1: return sizeof(BufferHeader) + iUsedWords * 2;

      // old buffer type
      case MBS_TYPE_10_1: return sizeof(BufferHeader) + iWords * 2;
   }

   return 0;

}

uint32_t mbs::BufferHeader::UsedBufferSize() const
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

void mbs::SwapData(void* data, unsigned bytessize)
{
   if (data==0) return;
   unsigned cnt = bytessize / 4;
   uint32_t* d = (uint32_t*) data;

   while (cnt-- != 0) {
      *d = bswap_32(*d);
      d++;
   }
}


enum EMbsServerKinds {
      NoServer = 0,
      TransportServer = 1,
      StreamServer = 2,
      OldTransportServer = 3,
      OldStreamServer = 4
   };

const char* mbs::ServerKindToStr(int kind)
{
   switch (kind) {
      case TransportServer: return "Transport";
      case StreamServer: return "Stream";
      case OldTransportServer: return "OldTransport";
      case OldStreamServer: return "OldStream";
   }
   return "NoServer";
}

int mbs::StrToServerKind(const char* str)
{
   if (str==0) return NoServer;
   if (strcmp(str, "Transport") == 0) return TransportServer;
   if (strcmp(str, "Stream") == 0) return StreamServer;
   if (strcmp(str, "OldTransport") == 0) return OldTransportServer;
   if (strcmp(str, "OldStream") == 0) return OldStreamServer;
   return NoServer;
}
