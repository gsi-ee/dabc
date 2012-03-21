#include "bnet/Structures.h"

bnet::EventPartRec* bnet::EventsPartsQueue::Find(const bnet::EventId& id)
{
   if (id.null() || Empty())
      return 0;

   for (unsigned n = 0; n < Size(); n++) {
      bnet::EventPartRec* rec = &Item(n);
      if (rec->evid == id)
         return rec;
   }

   return 0;
}

void bnet::EventsPartsQueue::FillEventsInfo(dabc::Pointer& ptr, bnet::EventId& last)
{
   if (Empty())
      return;

   for (unsigned n = 0; n < Size(); n++) {
      bnet::EventPartRec* rec = &Item(n);
      if (!last.null() && (rec->evid <= last))
         continue;

      uint64_t id = rec->evid;
      ptr.copyfrom_shift(&id, sizeof(id));

      last = rec->evid;
   }
}

void bnet::EventsPartsQueue::SkipEventParts(const bnet::EventId& id)
{
   if (id.null()) return;

   while (!Empty()) {
      if (Front().evid > id) break;
      PopOnly();
   }
}

// =========================================================================================

void bnet::EventsMasterQueue::SetStrictOrder(bool on)
{
   if (!Empty()) {
      EOUT(("Cannot change order when queue is not empty"));
   }

   fStrictOrder = on;
}

bnet::EventMasterRec* bnet::EventsMasterQueue::Find(const bnet::EventId& id)
{
   if (id.null() || Empty())
      return 0;

   if (IsStrictOrder()) {
      if  ((id < Front().evid) || (id > Back().evid)) return 0;
      return ItemPtr(id - Front().evid);
   }

   for (Iterator iter = begin(); iter != end(); iter++) {
      if (iter()->evid == id) return iter();
   }

   return 0;
}

bnet::EventMasterRec* bnet::EventsMasterQueue::AddEventInfo(int nodeid, const bnet::EventId& id, double curr_tm)
{
   if (id.null() || id.ctrl()) {
      EOUT(("Wrong event id"));
      return 0;
   }

   bnet::EventMasterRec* rec = Find(id);

   if (rec == 0) {
      bnet::EventId frontid = Empty() ? fLastInserted : Front().evid;

      if (!frontid.null() && (frontid > id)) {
         EOUT(("Very old event %u - why it here", (unsigned) id));
         return 0;
      }

      // TODO: should we preserve sequence of events ids, probably we need to add empty in between ???

      bnet::EventId lastid;

      if (!Empty() && IsStrictOrder()) {
         lastid = Back().evid;
         if (lastid > id) {
            EOUT(("Something wrong!!!"));
            return 0;
         }
         lastid++;
      } else {
         lastid = id;
      }

      // create
      while (lastid <= id) {
         if (Full()) {
            EOUT(("No place to add info for event %u", (unsigned) lastid));
            return 0;
         }
         rec = PushEmpty();

         rec->reset();
         rec->evid = lastid++;
         rec->firsttm = curr_tm;
      }

      fLastInserted = id;
   }

   if (rec->nodes[nodeid]) {
      EOUT(("Node already informed that event is exists"));
   }
   rec->nodes[nodeid] = true;

   return rec;
}

bool bnet::EventsMasterQueue::AddRawEventInfo(dabc::Pointer& rawdata, int nodeid, double curr_tm)
{
   uint64_t id(0);

   // FIXME: should be optimized!!!

   while (rawdata.fullsize() >= sizeof(uint64_t)) {
      rawdata.copyto_shift(&id, sizeof(uint64_t));

      // DOUT1(("ADD EVENT %u node %d queue size %u", (unsigned) id, nodeid, Size()));

      if (AddEventInfo(nodeid, id, curr_tm) == 0) return false;
   }

   return true;
}

int bnet::EventsMasterQueue::NumCompleteNonassignedEvents()
{
   // TODO: this could be kept in extra-counter

   int cnt(0);

   for (unsigned n = 0; n < Size(); n++)
      if (Item(n).complete() && (Item(n).tgtnode < 0))
         cnt++;

   return cnt;
}

int bnet::EventsMasterQueue::SkipEventsUntil(const bnet::EventId& id) {
   if (id.null())
      return 0;
   int cnt(0);

   while (!Empty()) {
      if (Front().evid > id)
         break;

      PopOnly();
      cnt++;
   }

   return cnt;
}

// ===============================================================================

void bnet::EventBundlesQueue::SkipEventBundels(const bnet::EventId& id)
{
   while (!Empty()) {
      if (Front().evid > id)
         break;
      PopOnly();
   }
}

// ==============================================================================


void bnet::ScheduleTurnRec::FillEmptyTurn()
{
   // clear all events assignment, controller should get its packets anyway
   for (unsigned n = 0; n < fVector.size(); n++) fVector[n].SetNull();
}

// ==============================================================================

void bnet::ScheduleTurnsQueue::FillTurnsInfo(dabc::Pointer& ptr, uint64_t& lastid)
{
   // here one should fill information for each turn which includes starttime, event association

   // TODO: performance - each turn is same for all nodes, therefore one could
   // transform it into binary once and than reuse already such binary form many times

   if (Empty()) return;

   while (lastid < Front().fTurn) {
      EOUT(("Missing turn id %u ????", (unsigned) lastid ));
      lastid++;
   }

   for (Iterator iter = begin(); iter != end(); iter++) {
      if (iter()->fTurn <= lastid) continue;

      // if buffer cannot contain next turn, stop filling
      if (iter()->rawsize() > ptr.fullsize()) break;

      lastid = iter()->fTurn;

//      DOUT1(("Add info for turn %u", (unsigned) lastid));

      ptr.copyfrom_shift(&lastid, sizeof(uint64_t));

      ptr.copyfrom_shift(&(iter()->starttime), sizeof(double));

      for (unsigned n = 0; n < iter()->fVector.size(); n++) {
         uint64_t id = iter()->fVector[n];
         ptr.copyfrom_shift(&id, sizeof(uint64_t));
      }
   }
}

void bnet::ScheduleTurnsQueue::PrintAll()
{
   unsigned cnt(0);
   for (Iterator iter = begin(); iter != end(); iter++) {
      DOUT1(("Turn %u starttime %7.6f same %s", (unsigned) iter()->fTurn, iter()->starttime, DBOOL(ItemPtr(cnt++) == iter())));
   }

//   for (unsigned n=0;n<Size();n++)
//      DOUT1(("Turn %u starttime %7.6f", (unsigned) ItemPtr(n)->fTurn, ItemPtr(n)->starttime));
}

bool bnet::ScheduleTurnsQueue::AddRawTurns(dabc::Pointer& ptr)
{
   while (!Full() && (ptr.fullsize() > 0)) {
      uint64_t newid(0);

      ptr.copyto(&newid, sizeof(uint64_t));

      // TODO: one should process such error situation
      if (!Empty() && (Back().fTurn + 1 != newid)) {
         EOUT(("Turn ID sequence is broken last %u new %u - should we repair it!!!", (unsigned) Back().fTurn + 1, (unsigned) newid));
      }

      ScheduleTurnRec* rec = PushEmpty();

      if (ptr.fullsize() < rec->rawsize()) {
         EOUT(("Buffer size is smaller than required to read complete turn record"));
      }

      ptr.shift(sizeof(uint64_t));

      rec->fTurn = newid;

      ptr.copyto(&rec->starttime, sizeof(double));
      ptr.shift(sizeof(double));

      for (unsigned n = 0; n < rec->fVector.size(); n++) {
         uint64_t evid;
         ptr.copyto(&evid, sizeof(uint64_t));
         ptr.shift(sizeof(uint64_t));
         rec->fVector[n] = evid;
      }
   }

   if (ptr.fullsize() > 0) {
      EOUT(("Not able to insert new data - queue is full"));
      return false;
   }

   return true;
}

bnet::ScheduleTurnRec* bnet::ScheduleTurnsQueue::AddNewTurn(uint64_t id, double tm, bool empty)
{
   if (Full()) return 0;

   bnet::ScheduleTurnRec* rec = PushEmpty();

   rec->fTurn = id;
   rec->starttime = tm;

   if (empty) rec->FillEmptyTurn();

   return rec;
}


void bnet::ScheduleTurnsQueue::RemoveTurnsUntil(uint64_t id)
{
   while (!Empty()) {
      if (Front().fTurn > id)
         break;
      PopOnly();
   }
}

