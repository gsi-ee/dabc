#ifndef DABC_Device
#define DABC_Device

#ifndef DABC_Folder
#include "dabc/Folder.h"
#endif

#ifndef DABC_WorkingProcessor
#include "dabc/WorkingProcessor.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

namespace dabc {
    
   class Port;
   class Transport;
   class WorkingThread;
   
   class Device : public Folder,
                  public WorkingProcessor {
      friend class Transport;                       
                             
      protected:  
         Device(Basic* parent, const char* name);
         
         void TransportCreated(Transport *tr);
         void TransportDestroyed(Transport *tr);
         
         virtual bool DoDeviceCleanup(bool full = false);

         void InitiateCleanup();
         
         Mutex* DeviceMutex() { return &fDeviceMutex; }
         
         Mutex           fDeviceMutex;
         PointersVector  fTransports; // transports, created by the device (owner)
     
      public:
         virtual ~Device();
         
         void CleanupDevice(bool force = false);
         
         virtual int ExecuteCommand(dabc::Command* cmd);
         
         virtual CommandReceiver* GetCmdReceiver() { return this; }
         
         virtual int CreateTransport(Command* cmd, Port* port) { return cmd_false; }

         virtual bool SubmitRemoteCommand(const char* servid, const char* channelid, Command* cmd) { return false; }
         
         static Command* MakeRemoteCommand(Command* cmd, const char* serverid, const char* channel = 0);
         
         virtual const char* ClassName() const { return "Device"; }
   };
};

#endif
