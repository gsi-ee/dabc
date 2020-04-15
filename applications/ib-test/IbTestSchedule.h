#ifndef IbTestSchedule_H
#define IbTestSchedule_H

#include <cstdint>

#include <string>
#include <vector>
#include <map>
#include <list>

#include "IbTestDefines.h"

/** Record represents route between two nodes in the cluster
 * Route can be:
 * null(not known),
 * local (node connected with itself),
 * one hope (connection between two nodes on the same switch)
 * three hopes (connection via three switches)
 * */

struct IbTestRoute {
   uint32_t route;

   enum { NullRoute = 0x77000000, LocalRoute = 0xff000000 };

   IbTestRoute() : route(NullRoute) {}

   IbTestRoute(const IbTestRoute& src) : route(src.route) {}

   IbTestRoute& operator=(const IbTestRoute& src) { route = src.route; return *this; }

   inline void SetRoute(uint32_t r) { route = r; }

   void SetNull() { SetRoute(NullRoute); }
   bool IsNull() const { return route == NullRoute; }

   void SetLocal() { SetRoute(LocalRoute); }
   bool IsLocal() const { return route == LocalRoute; }

   void Set1Hop(int switchid) { route = switchid & 0xff; }

   void Set3Hop(int sw1id, int sw2id, int sw3id)
   {
      route = (sw1id & 0xff) | (sw2id & 0xff) << 8 | (sw3id & 0xff) << 16;
   }

   int GetNumHops() const
   {
      if (IsNull()) return -1;
      if (IsLocal()) return 0;
      return (route & 0xffffff) <= 0xff ? 1 : 3;
   }

   int GetHop1() const { return route & 0xff; }
   int GetHop2() const { return (route >> 8) & 0xff; }
   int GetHop3() const { return (route >> 16) & 0xff; }


   int GetPath1() const { return GetHop1() | (GetHop2() << 8); }
   int GetPath2() const { return GetHop2() | (GetHop3() << 8); }

   bool operator==(const IbTestRoute& r) const { return route == r.route; }

   bool operator!=(const IbTestRoute& r) const { return route != r.route; }

   bool operator<(const IbTestRoute& r) const { return route < r.route; }

};


class IbTestClusterRouting {
   protected:
      enum { MaxNumNodes = 1024, MaxNumLids = 16 };

      int fNumNodes;

      int fNumSpines;

      int fNumLids; // indicate maximum measured lid numbers for single node, can be not a power of two

      IbTestRoute** fMatrix[MaxNumNodes];

      typedef std::map<std::string, int> NamesMap;

      NamesMap fSwitchNames;

      struct NodeRec {
         int id;
         int lid;
      };

      typedef std::map<std::string, NodeRec> NodesMap;

      NodesMap fNodes;

      void Reset();

      int FindNode(const std::string &name);
      /* Adds node of name and lid to the list of nodes.
       * If node with such name exists, liddiff will return difference
       * between base lid and specified lid parameter */
      int AddNode(const std::string &name, int lid = 0, int* liddiff = 0);
      int AddSwitch(const std::string &name);

      void ExcludeNode(int nodeid);

      /** Returns switch id, -1 if name not found */
      int FindSwitch(const std::string &name);


   public:
      IbTestClusterRouting();
      virtual ~IbTestClusterRouting();

      void AddCSCSwitches(int nspines, int nleafs);

      bool ReadFile(std::string filename);

      void GenerateFullTopology(int switchsize = 36, bool manylids = false);

      int NumNodes() const { return fNumNodes; }
      int NumLids() const { return fNumLids; }

      int NumSwitches() const { return fSwitchNames.size(); }
      int NumSpines() { return fNumSpines; }
      int NumLeafs() { return NumSwitches() - NumSpines(); }

      const char* NodeName(int nodeid) const;
      const char* SwitchName(int id) const;

      /** Add routes which were optimized during scanning */
      void ReinjectOptimizedRoutes();

      /** Method exclude all nodes where unknown routes are exists */
      void RemoveUndefinedRoutes();

      void PrintSpineStatistic();

      /** Match nodes on both routing tables - list of nodes should be similar at the end */
      bool MatchNodes(IbTestClusterRouting& other);

      /** Print difference between two routing tables */
      void PrintDifferecne(IbTestClusterRouting& other);

      /** Method used to find four nodes which are using same route for transport twice */
      void FindSameRouteTwice(bool bothlanes = true, const IbTestRoute& route0 = IbTestRoute(), bool rnd = false);

      /** Return route between node1 and node2.
       * Additional argument lid specifies shift in node2 lid. */
      IbTestRoute GetRoute(int node1, int node2, int lid = 0) const;

      /** Return switch to which node connected, -1 if failed */
      int GetNodeSwitch(int node);

      std::string GetRouteAsString(int node1, int node2, int lid = 0) const;

      void FillRandomIds(IbTestIntColumn& column) const;

      void FillBadIdsFor2Switches(IbTestIntColumn& column) const;

      /** Fill ids of nodes with name like node*, all these nodes can be used for interactive jobs */
      void FillInteractiveNodes(IbTestIntColumn& column) const;

      /** Select nodes id according arguments list
       * For a moment it is switch names */
      bool SelectNodes(const std::string &all_args, IbTestIntColumn& ids);

      /** Method tries to build optimal routes to balance load over spine switches */
      void BuildOptimalRoutes();

      /** Check the idea that route only depends from target lid.
       * This can save a lot of time during cluster exploration. */
      bool CheckTargetRoutes();

      /** Check how many LIDs are required to address all spines */
      int CheckUsefulLIDs();

      /** Save names of selected nodes in the file */
      bool SaveNodesList(const std::string &fname, const IbTestIntColumn& ids);

      /** Load nodes names from the file */
      bool LoadNodesList(const std::string &fname, IbTestIntColumn& ids);

};

struct IbTestScheduleItem {
   int  node;     // node id of the receiver
   int  lid;      // lid numbner

   IbTestScheduleItem()  { Reset(); }

   void Reset() { node = -1; lid = 0; }

   inline IbTestScheduleItem& operator=(const IbTestScheduleItem& src)
   {
      node = src.node;
      lid = src.lid;
      return *this;
   }

   bool Empty() const { return node<0; }

   void Set(int _node, int _lid = 0) { node = _node; lid = _lid; }
};

/** Structure used for coding of move operation inside schedule.
 * Used to rollback several moving done in the schedule */
struct IbTestScheduleMove {
   int nsend;
   int slot1;
   int slot2;
   int lid;    // stored lid number

   IbTestScheduleMove() : nsend(-1), slot1(0), slot2(0), lid(-1) {}

   IbTestScheduleMove(int _nsend, int _slot1, int _slot2, int _lid = -1) : nsend(_nsend), slot1(_slot1), slot2(_slot2), lid(_lid) {}

   IbTestScheduleMove(const IbTestScheduleMove& src) : nsend(src.nsend), slot1(src.slot1), slot2(src.slot2), lid(src.lid) {}

   IbTestScheduleMove& operator=(const IbTestScheduleMove& src) { nsend = src.nsend; slot1 = src.slot1; slot2 = src.slot2; lid = src.lid; return *this; }

   bool Empty() const { return nsend<0 || (slot1==slot2); }
};

typedef std::list<IbTestScheduleMove> IbTestScheduleMoveList;


class IbTestSchedule {
   protected:
      int   fNumSenders;
      int   fMaxNumSlots;
      int   fNumSlots;

      IbTestScheduleItem**  fSchedule;
      double*              fTimeSlots;
      IbTestScheduleItem    fDummy;

   public:
      IbTestSchedule();

      IbTestSchedule(int MaxNumSlots, int NumSenders);

      virtual ~IbTestSchedule();

      void Allocate(int MaxNumSlots, int NumSenders);

      int  numSlots() const { return fNumSlots; }
      void SetNumSlots(int n) { fNumSlots = n<fMaxNumSlots ? n : fMaxNumSlots; }

      int  numSenders() const { return fNumSenders; }
      int  maxNumSlots() const { return fMaxNumSlots; }

      IbTestScheduleItem* getScheduleSlot(int nslot);
      double timeSlot(int nslot);
      void SetTimeSlot(int nslot, double tm);
      double endTime() { return timeSlot(numSlots()); }
      void SetEndTime(double tm) { SetTimeSlot(numSlots(), tm); }

      /** return maximum number of lid, used in the schedule */
      int numLids();

      /** Fills schedule time with regular step */
      void FillRegularTime(double schedule_step);

      inline IbTestScheduleItem& Item(int nslot, int nsender)
      {
         return nslot<numSlots() && nsender<numSenders() ? fSchedule[nslot][nsender] : fDummy;
      }

      inline bool IsEmptyItem(int nslot, int nsender)
      {
         return Item(nslot,nsender).Empty();
      }

      /** Fill receive schedule, originally schedule was designed for senders */
      void FillReceiveSchedule(IbTestSchedule& recv);

      /** Return true if specified node has at least one operation specified */
      bool IsNodeActive(int node);

      /** Remove any operations which are concerns inactive node */
      void ExcludeInactiveNode(int node);

      /** Clear all times and all receivers */
      void Clear();

      bool ShiftToNextOperation(int node, double& basetm, int& nslot);

      double totalTime();

      double calcOccupation();

      void Print(IbTestIntMatrix* matr);

      double calcBandwidth(IbTestIntMatrix* matr);

      void FillRoundRoubin(IbTestIntColumn* ids = 0, double schstep = 1.);

      bool BuildOptimized(IbTestClusterRouting& routing, IbTestIntColumn* ids = 0, bool show = false);

      bool BuildRegularSchedule(IbTestClusterRouting& routing, IbTestIntColumn* ids = 0, bool show = false);

      double CheckConjunction(IbTestClusterRouting& routing, bool show = false);

      void PrintSlotWithRouting(const IbTestClusterRouting& routing, int nslot);

      void PrintWithRouting(const IbTestClusterRouting& routing, int nlast = 0);

      bool CanMoveItemTo(const IbTestClusterRouting& routing, int nsender, int nslot1, int nslot2, int& newlid);

      void MoveSender(IbTestScheduleMoveList& lst, int nsender, int nslot0, int nslot1, int lid = -1);

      bool TryMoveSender(const IbTestClusterRouting& routing, IbTestScheduleMoveList& global_lst, int nsender, int nslot0, int nslot1, int routekind, int nslot2 = -1, bool show = false);

      bool TryToCompress(IbTestClusterRouting& routing, bool show = false, double runtime = 50.);

      bool RollBack(IbTestScheduleMoveList& lst);

      /** \brief Check that schedule has all necessary transfers */
      bool ProveSchedule(IbTestIntColumn* ids = 0);

      /** \brief Select schedule for only specified ids, recode ids according to their sequence number */
      bool RecodeIds(const IbTestIntColumn& ids, IbTestSchedule& new_sch);

      /** \brief Copies schedule content to new object */
      bool CopyTo(IbTestSchedule& new_sch);

      bool SaveToFile(const std::string &fname);

      bool ReadFromFile(const std::string &fname);
};



#endif
