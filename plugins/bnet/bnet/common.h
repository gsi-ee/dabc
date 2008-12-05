#ifndef BNET_common
#define BNET_common

#include <stdint.h>

#include <vector>

#include "dabc/string.h"

namespace bnet {

   extern const char* xmlClusterClass; // preferred net device for building network

   extern const char* xmlIsRunning;
   extern const char* xmlNetDevice;
   extern const char* xmlWithController;
   extern const char* xmlNumEventsCombine;

   extern const char* xmlIsGenerator;
   extern const char* xmlIsSender;
   extern const char* xmlIsReceiever;
   extern const char* xmlIsFilter;
   extern const char* xmlCombinerModus;
   extern const char* xmlNumReadouts;
   extern const char* xmlStoragePar;

   extern const char* ReadoutPoolName;
   extern const char* TransportPoolName;
   extern const char* EventPoolName;
   extern const char* ControlPoolName;

   extern const char* SenderThreadName;
   extern const char* ReceiverThreadName;
   extern const char* ControlThreadName;

   extern const char* xmlReadoutBuffer;
   extern const char* xmlReadoutPoolSize;
   extern const char* xmlTransportBuffer;
   extern const char* xmlTransportPoolSize;
   extern const char* xmlEventBuffer;
   extern const char* xmlEventPoolSize;
   extern const char* xmlCtrlBuffer;
   extern const char* xmlCtrlPoolSize;

   extern const char* parStatus;
   extern const char* parRecvStatus;
   extern const char* parSendStatus;
   extern const char* parSendMask;
   extern const char* parRecvMask;


   // list of configuration parameters in worker application
   extern const char* CfgNodeId;
   extern const char* CfgNumNodes;
   extern const char* CfgController;
   extern const char* CfgSendMask;
   extern const char* CfgRecvMask;
   extern const char* CfgClusterMgr;
   extern const char* CfgNetDevice;
   extern const char* CfgConnected;
   extern const char* CfgEventsCombine;
   extern const char* CfgReadoutPool;




   enum EBnetBufferTypes {
      mbt_EvInfo    = 201, // events information
      mbt_EvAssign  = 202  // events assignment to builders
   };

   #define ReadoutQueueSize      4

   #define SenderInQueueSize     4
   #define SenderQueueSize       5

   #define ReceiverQueueSize     8
   #define ReceiverOutQueueSize  4

   #define BuilderInpQueueSize   4
   #define BuilderOutQueueSize   4

   #define FilterInpQueueSize    4
   #define FilterOutQueueSize    4

   #define CtrlInpQueueSize      4
   #define CtrlOutQueueSize      4

   #define BnetUseAcknowledge    false

   typedef uint64_t EventId;

#pragma pack(1)

   typedef struct SubEventNetHeader {
      uint64_t evid;
      uint64_t srcnode;
      uint64_t tgtnode;
      uint64_t pktid;
   };

   typedef struct EventInfoRec {
      EventId  evid;
      int64_t  evsize;
   };

   typedef struct EventAssignRec {
      EventId  evid;
      uint64_t tgtnode;
   };


#pragma pack(0)

   class NodesVector {
      protected:
         std::vector<int> fUsedNodes;
         int fNumNodes;

      public:
         NodesVector() : fUsedNodes(), fNumNodes(0) {}
         NodesVector(std::string mask, int numnodes = 0) :
            fUsedNodes(),
            fNumNodes(0) {
               Reset(mask, numnodes);
            }

         virtual ~NodesVector() {}

         int NumNodes() const { return fNumNodes; }
         unsigned size() const { return fUsedNodes.size(); }
         int operator[](unsigned node) { return fUsedNodes[node]; }

         void Reset(std::string mask, int numnodes = 0);

         bool HasNode(int node) const;

   };

}

#endif
