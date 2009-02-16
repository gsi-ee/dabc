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
#ifndef DABC_LocalTransport
#define DABC_LocalTransport

#ifndef DABC_Basic
#include "dabc/Basic.h"
#endif

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_Device
#include "dabc/Device.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

namespace dabc {
   
   class LocalDevice;
   
   class LocalTransport : public Transport {
      friend class LocalDevice;
      protected: 
         LocalTransport            *fOther;
         BuffersQueue               fQueue;
         Mutex                     *fMutex; 
         bool                       fMutexOwner;
         bool                       fMemCopy;
         
         virtual void PortChanged();
         
         virtual bool _IsReadyForCleanup();
         
      public:  
         LocalTransport(LocalDevice* dev, Port* port, Mutex* mutex, bool owner, bool memcopy);
         virtual ~LocalTransport();

         virtual bool ProvidesInput() { return (fOther!=0) && (fQueue.Capacity()>0); }
         virtual bool ProvidesOutput() { return fOther ? fOther->fQueue.Capacity()>0 : false; }
         
         virtual bool Recv(Buffer* &);
         virtual unsigned RecvQueueSize() const;
         virtual Buffer* RecvBuffer(unsigned indx) const;
         virtual bool Send(Buffer*);
         virtual unsigned SendQueueSize();
         virtual unsigned MaxSendSegments() { return 9999; }
   };
   
   // _______________________________________________________
   
   class NullTransport : public Transport {
      public:
         NullTransport(LocalDevice* dev);
         
         virtual bool ProvidesInput() { return false; }
         virtual bool ProvidesOutput() { return true; }

         virtual bool Recv(Buffer* &buf) { buf = 0; return false; }
         virtual unsigned  RecvQueueSize() const { return 0; }
         virtual Buffer* RecvBuffer(unsigned) const { return 0; }
         virtual bool Send(Buffer*);
         virtual unsigned SendQueueSize() { return 0; }
         virtual unsigned MaxSendSegments() { return 9999; }
   };
   
   // _______________________________________________________________

   
   class LocalDevice : public Device {
      public:
         LocalDevice(Basic* parent, const char* name);

         bool ConnectPorts(Port* port1, Port* port2, CommandClientBase* cli = 0);
         
         bool MakeNullTransport(Port* port);
         
         virtual int CreateTransport(Command* cmd, dabc::Port* port);
         
         virtual int ExecuteCommand(Command* cmd);
         
         virtual const char* ClassName() const { return "LocalDevice"; }
   };

}

#endif
