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
#ifndef ABB_Device
#define ABB_Device

#include "pci/BoardDevice.h"
#include "mprace/DMAEngineWG.h"

namespace abb {

   class Device : public pci::BoardDevice {

      public:

         Device(Basic* parent, const char* name, bool enabledma, dabc::Command* cmd);
         virtual ~Device();

         virtual int ReadPCI(dabc::Buffer* buf);

         virtual bool ReadPCIStart(dabc::Buffer* buf);

         virtual int ReadPCIComplete(dabc::Buffer* buf);

         virtual const char* ClassName() const { return "abb::Device"; }

         unsigned int GetDeviceNumber() { return fDeviceNum; }

         void SetEventCount(uint64_t val){fEventCnt=val;}
         void SetID(uint64_t id){fUniqueId=id;}

      protected:
         virtual bool DoDeviceCleanup(bool full = false);

         bool WriteBnetHeader(dabc::Buffer* buf);

      private:

      /** number X of abb device (/dev/fpgaX) */
      unsigned int fDeviceNum;

      /** access the dma engine directly for wait */
      mprace::DMAEngineWG* fDMAengine;

      /** current dma read buffer*/
      mprace::DMABuffer* fReadDMAbuf;


      /** event counter for emulated event headers*/
      uint64_t                  fEventCnt;
      /** id number for emulated event headers*/
      uint64_t                  fUniqueId;

   };

}

#endif
