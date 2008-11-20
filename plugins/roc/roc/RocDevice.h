#ifndef ROC_ROCDEVICE_H
#define ROC_ROCDEVICE_H

//#include <queue>

#include "dabc/Basic.h"
#include "dabc/Device.h"
#include "dabc/Buffer.h"

#include "SysCoreControl.h"

namespace dabc{
   class MemoryPool;
   class Command;
};

namespace roc {

   extern const char* xmlNumRocs;
   extern const char* xmlRocPool;
   extern const char* xmlTransportWindow;
   extern const char* xmlBoardIP;

   enum ERocBufferTypes {
      rbt_RawRocData     = 234
   };

   // this is value of subevent iProc in mbs event
   // For raw data rocid stored in iSubcrate, for merged data iSubcrate stores higher bits
   enum ERocMbsTypes {
      proc_RocEvent     = 1,   // complete event from one roc board (iSubcrate = rocid)
      proc_ErrEvent     = 2,   // one or several events with corrupted data inside (iSubcrate = rocid)
      proc_MergedEvent  = 3    // sorted and synchronised data from several rocs (iSubcrate = upper rocid bits)
   };

   class RocDevice : public dabc::Device,
                     protected SysCoreControl {
      public:

         RocDevice(dabc::Basic* parent, const char* name);
         virtual ~RocDevice();

         virtual const char* ClassName() const { return "RocDevice"; }

         int AddBoard(const char* address, unsigned ctlPort);

         SysCoreBoard* GetBoard(unsigned id);

         virtual int ExecuteCommand(dabc::Command* cmd);

         virtual int CreateTransport(dabc::Command* cmd, dabc::Port* port);

      protected:

         virtual bool DoDeviceCleanup(bool full = false);

         virtual void DataCallBack(SysCoreBoard* brd);
   };

}

#endif //ROCDEVICE_H
