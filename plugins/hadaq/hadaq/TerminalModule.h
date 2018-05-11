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

#ifndef HADAQ_TerminalModule
#define HADAQ_TerminalModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#include <vector>

namespace hadaq {

   /** \brief Terminal for HADAQ event builder
    *
    * Provides text display of different counters.
    */

   class TerminalModule : public dabc::ModuleAsync {
      protected:

         struct CalibrRect {
            long unsigned         lastrecv;
            bool                  send_request;
            unsigned              trb;
            std::vector<uint64_t> tdcs;
            int                   progress;
            std::string           state;
            CalibrRect() : lastrecv(0), send_request(false), trb(0), tdcs(), progress(0), state() {}
         };

         uint64_t        fTotalBuildEvents;
         uint64_t        fTotalRecvBytes;
         uint64_t        fTotalDiscEvents;
         uint64_t        fTotalDroppedData;

         bool            fDoClear;      //!< clear terminal when start module
         bool            fDoShow;       //!< perform output
         dabc::TimeStamp fLastTm;
         std::vector<CalibrRect> fCalibr;
         int             fServPort;
         dabc::Command   fLastServCmd;    //!< last received data from file transport
         bool            fServReqRunning; //!<  is file request running
         int             fFilePort;
         dabc::Command   fLastFileCmd;    //!< last received data from file transport
         bool            fFileReqRunning; //!< is file request running
         int             fRingSize;       //! number of last IDs shown

         std::string rate_to_str(double r);

         virtual bool ReplyCommand(dabc::Command cmd);

      public:

         TerminalModule(const std::string &name, dabc::Command cmd = nullptr);

         virtual void BeforeModuleStart();

         virtual void ProcessTimerEvent(unsigned timer);
   };
}


#endif
