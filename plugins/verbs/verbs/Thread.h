/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#ifndef VERBS_Thread
#define VERBS_Thread

#ifndef DABC_WorkingThread
#include "dabc/WorkingThread.h"
#endif

#include <map>
#include <stdint.h>

struct ibv_comp_channel;
struct ibv_wc;

#define VERBS_USING_PIPE

#define VERBS_THRD_CLASSNAME "VerbsThread"

namespace dabc {
   class Port;
}

namespace verbs {

   class Device;
   class MemoryPool;

   class Processor;
   class TimeoutProcessor;
   class ProtocolProcessor;
   class ConnectProcessor;

   class QueuePair;
   class ComplQueue;

   class Thread : public dabc::WorkingThread {

      typedef std::map<uint32_t, uint32_t>  ProcessorsMap;

      enum { LoopBackQueueSize = 10 };

      protected:
         enum EWaitStatus { wsWorking, wsWaiting, wsFired };

         Device                  *fDevice;
         struct ibv_comp_channel *fChannel;
      #ifdef VERBS_USING_PIPE
         int                      fPipe[2];
      #else
         QueuePair               *fLoopBackQP;
         MemoryPool              *fLoopBackPool;
         int                      fLoopBackCnt;
         TimeoutProcessor        *fTimeout; // use to produce timeouts
      #endif
         ComplQueue              *fMainCQ;
         long                     fFireCounter;
         EWaitStatus              fWaitStatus;
         ProcessorsMap            fMap; // for 1000 elements one need about 50 nanosec to get value from it
         int                      fWCSize; // size of array
         struct ibv_wc*           fWCs; // list of event completion
         ConnectProcessor        *fConnect;  // connection initiation
         long                     fFastModus; // makes polling over MainCQ before starting wait - defines pooling number
      public:
         // list of all events for all kind of socket processors

         Thread(Device* dev, dabc::Basic* parent, const char* name = "verbsthrd");
         virtual ~Thread();

         void CloseThread();

         struct ibv_comp_channel* Channel() const { return fChannel; }

         ComplQueue* MakeCQ();

         Device* GetDevice() const { return fDevice; }

         bool DoServer(dabc::Command* cmd, dabc::Port* port, const char* portname);
         bool DoClient(dabc::Command* cmd, dabc::Port* port, const char* portname);
         void FillServerId(std::string& id);

         ConnectProcessor* Connect() const { return fConnect; }

         ProtocolProcessor* FindProtocol(const char* connid);

         bool IsFastModus() const { return fFastModus > 0; }

         virtual int ExecuteCommand(dabc::Command* cmd);

         virtual const char* ClassName() const { return VERBS_THRD_CLASSNAME; }

         virtual bool CompatibleClass(const char* clname) const;

         static const char* StatusStr(int code);

      protected:

         bool StartConnectProcessor();

         virtual dabc::EventId WaitEvent(double tmout);

         virtual void _Fire(dabc::EventId arg, int nq);

         virtual void ProcessorNumberChanged();
   };
}

#endif
