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

#ifndef ROC_Board
#define ROC_Board

#include <stdint.h>

#include "nxyter/Data.h"


namespace roc {

   enum BoardRole { roleNone, roleObserver, roleMaster, roleDAQ };

   // this is value of subevent iProc in mbs event
   // For raw data rocid stored in iSubcrate, for merged data iSubcrate stores higher bits
   enum ERocMbsTypes {
      proc_RocEvent     = 1,   // complete event from one roc board (iSubcrate = rocid)
      proc_ErrEvent     = 2,   // one or several events with corrupted data inside (iSubcrate = rocid)
      proc_MergedEvent  = 3    // sorted and synchronised data from several rocs (iSubcrate = upper rocid bits)
   };

   extern const char* xmlNumRocs;
   extern const char* xmlRocPool;
   extern const char* xmlTransportWindow;
   extern const char* xmlBoardIP;

   extern const char* typeUdpDevice;

   enum ERocBufferTypes {
      rbt_RawRocData     = 234
   };

   class ReadoutModule;

   class Board {
      protected:
         BoardRole      fRole;
         int            fErrNo;
         int            fRocNumber;

         ReadoutModule *fReadout;

         Board();
         virtual ~Board();

         void SetReadout(ReadoutModule* m) { fReadout = m; }

         virtual bool initialise(BoardRole role) = 0;
         virtual void* getdeviceptr() = 0;

      public:

         BoardRole Role() const { return fRole; }
         int errno() const { return fErrNo; }
         int rocNumber() const { return fRocNumber; }

         virtual bool poke(uint32_t addr, uint32_t value, double tmout = 5.) = 0;
         virtual uint32_t peek(uint32_t addr, double tmout = 5.) = 0;

         bool startDaq();
         bool suspendDaq();
         bool stopDaq();

         bool getNextData(nxyter::Data& data, double tmout = 1.);

         static Board* Connect(const char* name, BoardRole role = roleObserver);
         static bool Close(Board* brd);
   };

}

#endif
