#ifndef BNET_Structures
#define BNET_Structures

#include "bnet/defines.h"

#include "dabc/Queue.h"

#include "dabc/Pointer.h"

#include <vector>

namespace bnet {

   /** Structure to store all relevant information for event buffer
    * Preserved in the queue until event data is not transported and transport is confirmed */

   enum EventPartState { eps_Init, eps_Scheduled, eps_Ready };

   struct EventPartRec {
       EventPartState  state;     // actual state of the data
       bnet::EventId   evid;      // eventid
       dabc::Buffer    buf;       // buffer with data
       double          acq_tm;    // time when event was insertred, will be used to remove it

       EventPartRec() : buf()  { reset(); }

       EventPartRec(const EventPartRec& src) : state(src.state), evid(src.evid), buf(src.buf), acq_tm(src.acq_tm)  {}

       ~EventPartRec() { reset(); }

       void reset() {  state = eps_Init; evid = 0; buf.Release(); acq_tm = 0.; }
   };


   class EventsPartsQueue : public dabc::RecordsQueue<EventPartRec, false> {
      typedef dabc::RecordsQueue<EventPartRec, false> parent_class;

      public:

      EventsPartsQueue() : parent_class() {}

      EventPartRec* Find(const bnet::EventId& id);

      void FillEventsInfo(dabc::Pointer& ptr, bnet::EventId& last);

      /** Remove all events with id less than specified */
      void SkipEventParts(const bnet::EventId& id);
   };

   /** Structure to collect information about event on the master node,
    * Used to produce schedule, use balancing, force data drop on the nodes */

   struct EventMasterRec {
      bnet::EventId   evid;      // event identifier
      int             tgtnode;   // node where event should be build
      double          firsttm;   // time when event first time detected
      // TODO: replace by bit mask, will be required in many other places
      std::vector<bool> nodes;   // info if node has such event ot not

      EventMasterRec() { reset(); }

      ~EventMasterRec() { reset(); }

      void reset()
      {
         evid = 0; tgtnode = -1; firsttm = 0.;
         for (unsigned n=0;n<nodes.size();n++) nodes[n] = false;
      }

      void allocate(unsigned numnodes)
      {
         nodes.resize(numnodes, false);
         reset();
      }

      /** Return true if parts on all nodes are detected */
      // TODO: this could be optimized
      bool complete() const
      {
         // node 0 is excluded while it is master controller
         for (unsigned n=1;n<nodes.size();n++)
            if (!nodes[n]) return false;
         return true;
      }

      private:

      // forbeed copy constructor - only objects in the queue should be used
      EventMasterRec(const EventMasterRec& src) {}
   };

   class EventsMasterQueue : public dabc::RecordsQueue<EventMasterRec, false> {
      typedef dabc::RecordsQueue<EventMasterRec, false> parent_class;

      bool fStrictOrder; // all events ids are in strict sequence: N, N+1, N+2 and so on

      bnet::EventId   fLastInserted; // last id which was processed

      public:

      EventsMasterQueue() : parent_class(), fStrictOrder(true), fLastInserted() {}

      bool IsStrictOrder() const { return fStrictOrder; }
      void SetStrictOrder(bool on);

      EventMasterRec* Find(const bnet::EventId& id);

      EventMasterRec* AddEventInfo(int nodeid, const bnet::EventId& id, double curr_tm = 0);

      bool AddRawEventInfo(dabc::Pointer& rawdata, int nodeid, double curr_tm = 0.);

      /** Return number of complete events, which are not yet assigned */
      int NumCompleteNonassignedEvents();

      /** Remove all events from the queue until specified id, returns number of skipped events */
      int SkipEventsUntil(const bnet::EventId& id);
   };

   /** Structure to collect all parts of complete event
    * Preserved in the queue until all data are ready */

   struct EventBundleRec {
      bnet::EventId   evid;      // event identifier
      unsigned        numbuf;    // number of buffers
      dabc::Buffer   *buf;       // array of buffers
      double          acq_tm;    // time when bundle was produced, will be used to remove it

      EventBundleRec() : evid(0), numbuf(0), buf(0), acq_tm(0.) {}

      ~EventBundleRec() { allocate(0); }

      void allocate(unsigned _numbuf)
      {
         if (_numbuf == numbuf) return;

         reset();

         if (buf!=0) {
            delete [] buf;
            buf = 0;
         }

         numbuf = 0;

         if (_numbuf > 0) {
            numbuf = _numbuf;
            buf = new dabc::Buffer[numbuf];
         }
      }

      bool ready()
      {
         for (unsigned n=0;n<numbuf;n++)
            if (buf[n].null()) return false;
         return true;
      }

      void reset()
      {
         evid = 0;
         acq_tm = 0;
         for (unsigned n=0;n<numbuf;n++) buf[n].Release();
      }

      private:

      EventBundleRec(const EventBundleRec& src) :
         evid(src.evid), acq_tm(src.acq_tm)
      {
         allocate(src.numbuf);
         for (unsigned n=0;n<src.numbuf;n++)
            buf[n] = src.buf[n];
      }

   };

   class EventBundlesQueue : public dabc::RecordsQueue<EventBundleRec, false> {
      typedef dabc::RecordsQueue<EventBundleRec, false> parent_class;

      public:

      EventBundlesQueue() : parent_class() {}

      void SkipEventBundels(const bnet::EventId& id);

   };

   /** Structure to keep schedule description for single turn
    * Should include start time, id and nodeid<->evid association */

   struct ScheduleTurnRec {

      private:

      // forbid copy constructor - all items should be created in queue
      ScheduleTurnRec(const ScheduleTurnRec& src) {}

      public:

      uint64_t fTurn;
      double   starttime;
      std::vector<bnet::EventId>  fVector; // vector of events id - one for each node

      ScheduleTurnRec() { reset(); }

      inline uint64_t id() const { return fTurn; }

      void reset()
      {
         fTurn = 0;
         starttime = 0;
      }

      void allocate(unsigned num)
      {
         fVector.resize(num, 0);
      }

      // how many bytes required for raw representation
      unsigned rawsize() const
         { return sizeof(uint64_t) + sizeof(double) + fVector.size() * sizeof(uint64_t); }

      void FillEmptyTurn();
   };

   class ScheduleTurnsQueue : public dabc::RecordsQueue<ScheduleTurnRec, false> {
      typedef dabc::RecordsQueue<ScheduleTurnRec, false> parent_class;

      public:

      ScheduleTurnsQueue() : parent_class() {}

      void FillTurnsInfo(dabc::Pointer& ptr, uint64_t& lastid);

      bool AddRawTurns(dabc::Pointer& ptr);

      ScheduleTurnRec* AddNewTurn(uint64_t id, double tm, bool empty = false);

      void RemoveTurnsUntil(uint64_t id);

      void PrintAll();
   };

   /** Structure contains current information about each node */
   struct MasterNodeInfo {

      uint64_t last_send_turnid; // id of the last sended turn

      bnet::EventId last_build_id; // id of last build event
      double last_build_tm;        // time when last event was build

      MasterNodeInfo() :
         last_send_turnid(0)
      {
      }

   };

   class MasterNodeInfoVector : public std::vector<MasterNodeInfo> {
      public:

      MasterNodeInfoVector() : std::vector<MasterNodeInfo>() {}
   };

}


#endif
