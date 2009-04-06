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

#ifndef ROC_Device
#define ROC_Device

#ifndef DABC_Device
#include "dabc/Device.h"
#endif

#ifndef ROC_Board
#include "roc/Board.h"
#endif

namespace roc {

   extern const char* xmlNumRocs;
   extern const char* xmlRocPool;
   extern const char* xmlTransportWindow;
   extern const char* xmlBoardIP;


   extern const char* typeUdpDevice;

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

   class Device : public dabc::Device {
      friend class roc::Board;

      protected:
         int         fErrNo;
         uint32_t    fRocNumber;

         virtual bool initialise(BoardRole role) { return false; }

      public:

         Device(dabc::Basic* parent, const char* name);
         virtual ~Device();

         virtual const char* ClassName() const { return "roc::Device"; }

         virtual int ExecuteCommand(dabc::Command* cmd);

         virtual int CreateTransport(dabc::Command* cmd, dabc::Port* port);

         uint32_t rocNumber() const { return fRocNumber; }

         virtual int errno() const { return fErrNo; }
         virtual bool poke(uint32_t addr, uint32_t value, double tmout = 5.);
         virtual uint32_t peek(uint32_t addr, double tmout = 5.);
   };

}

#endif
