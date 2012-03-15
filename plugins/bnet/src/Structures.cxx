#include "bnet/Structures.h"


bnet::EventPartRec* bnet::EventsPartsQueue::Find(const bnet::EventId& id)
{
   if (id.null() || Empty()) return 0;

   for (unsigned n=0;n<Size();n++) {
      bnet::EventPartRec* rec = &Item(n);
      if (rec->evid == id) return rec;
   }

   return 0;
}


unsigned bnet::EventsPartsQueue::FillEventsInfo(dabc::Pointer& ptr, bnet::EventId& last)
{
   if (Empty()) return 0;

   unsigned len = 0;

   for (unsigned n=0;n<Size();n++) {
      bnet::EventPartRec* rec = &Item(n);
      if (!last.null() && (rec->evid <= last)) continue;

      uint64_t id = rec->evid;
      ptr.copyfrom(&id, sizeof(id));
      ptr.shift(sizeof(id));
      len += sizeof(id);

      last = rec->evid;
   }

   return len;
}
