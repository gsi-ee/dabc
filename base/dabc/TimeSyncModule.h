#ifndef DABC_TimeSyncModule
#define DABC_TimeSyncModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_logging
#include "dabc/logging.h"
#endif

#ifndef DABC_statistic
#include "dabc/statistic.h"
#endif


#ifndef DABC_threads
#include "dabc/threads.h"
#endif


namespace dabc {

   class CommandDoTimeSync : public Command {
      public:
         CommandDoTimeSync(bool thrdclock, int niter, bool doshift = false, bool doscale = false) :
            Command("DoTimeSync")
            {
               SetBool("ThrdClock", thrdclock);
               SetInt("NumIter", niter);
               SetBool("DoShift", doshift);
               SetBool("DoScale", doscale);
            }
   };

   class TimeSource;

   class TimeSyncModule : public ModuleAsync {
      protected:
         bool        fMasterConn;
         int         fNumSlaves;
         double*     fSyncTimes;  // time when last tyme sync was done, need for scale calculations
         std::string      fPoolName;

         TimeSource* fTimeSource;

         Command* fCurrCmd;
         int      fSlaveCnt;
         int      fPktCnt;
         Average  fLoopTime;
         Average  fMasterToSlave;
         Average  fSlaveToMaster;
         double   fSetShift;
         double   fSetScale;
         bool     fSkipSlavePacket;

         int      fMasterSendCnt;
         int      fMasterRecvCnt;

         Mutex    fMutex;

         PoolHandle* fPool;

         void GenerateNextEvent(Buffer* buf);
         bool FillMasterPacket(Buffer* buf);
         bool FillSlavePacket(Buffer* buf);

         virtual void OnThreadAssigned();

         virtual int ExecuteCommand(Command* cmd);

         virtual void ProcessInputEvent(Port* port);

      public:
         TimeSyncModule(const char* name, Command* cmd);
         virtual ~TimeSyncModule();
   };
}

#endif
