#ifndef BNET_Structures
#define BNET_Structures

#include "bnet/defines.h"

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

   /** Structure to collect information about event on the master node,
    * Used to produce schedule, use balancing, force data drop on the nodes */

   struct EventMasterRec {
      bnet::EventId   evid;      // event identifier
      int             tgtnode;   // node where event should be build
      double          firsttm;   // time when event first time detected

      EventMasterRec() { reset(); }

      ~EventMasterRec() { reset(); }

      void reset() { evid = 0; tgtnode = -1; firsttm = 0.; }

      private:

      // forbeed copy constructor - only objects in the queue should be used
      EventMasterRec(const EventMasterRec& src) {}
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

   /** Structure to keep schedule description for single turn
    * Should include start time, id and nodeid<->evid association */

   struct ScheduleTurnRec {
      uint64_t fTurn;
      double   starttime;
      std::vector<bnet::EventId>  fVector; // vector of events id - 1 for each node

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

   };

}


#endif
