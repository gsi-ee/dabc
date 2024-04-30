// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#ifndef DOGMA_TerminalModule
#define DOGMA_TerminalModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#include <vector>

namespace dogma {

   /** \brief Terminal for DOGMA event builder
    *
    * Provides text display of different counters.
    */

   class TerminalModule : public dabc::ModuleAsync {
      protected:

         struct CalibrRect {
            long unsigned         lastrecv{0};
            bool                  send_request{false};
            unsigned              trb{0};
            std::vector<uint64_t> tdcs;
            int                   progress{0};
            std::string           state;
         };

         uint64_t        fTotalBuildEvents{0};
         uint64_t        fTotalRecvBytes{0};
         uint64_t        fTotalDiscEvents{0};
         uint64_t        fTotalDroppedData{0};

         bool            fDoClear{false};       ///< clear terminal when start module
         bool            fDoShow{false};        ///< perform output
         dabc::TimeStamp fLastTm;
         std::vector<CalibrRect> fCalibr;
         int             fServPort{0};
         dabc::Command   fLastServCmd;    ///< last received data from file transport
         bool            fServReqRunning{false}; ///<  is file request running
         int             fFilePort{0};
         dabc::Command   fLastFileCmd;    ///< last received data from file transport
         bool            fFileReqRunning{false}; ///< is file request running
         int             fRingSize{0};       ///< number of last IDs shown

         std::string     fModuleName;    ///< name of hadaq combiner module

         std::string rate_to_str(double r);

         bool ReplyCommand(dabc::Command cmd) override;

      public:

         TerminalModule(const std::string &name, dabc::Command cmd = nullptr);

         void BeforeModuleStart() override;

         void ProcessTimerEvent(unsigned timer) override;
   };
}


#endif
