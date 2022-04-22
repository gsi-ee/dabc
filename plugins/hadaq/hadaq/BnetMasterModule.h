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

#ifndef HADAQ_BnetMasterModule
#define HADAQ_BnetMasterModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#include <vector>
#include <string>


namespace hadaq {

   /** \brief Master monitor for BNet components
    *
    * Provides statistic for clients
    */

   class BnetMasterModule : public dabc::ModuleAsync {
      protected:

         bool          fControl{false};  ///< when true, master actively controls BNET nodes and switches to new RUNs
         unsigned      fMaxRunSize{0}; ///< maximal run size in MB
         dabc::Command fCurrentFileCmd; ///< currently running cmd to switch files
         int           fCmdCnt{0};     ///< just counter to avoid mismatch
         int           fCmdReplies{0}; ///< number of replies for current command
         double        fCmdQuality{0};  ///< current command quality, used when creating calibration
         unsigned      fCalibrRunId{0};  ///< last calibration runid
         long unsigned fCalibrTm{0};     ///< last calibr time in seconds since 1970

         dabc::Command fCurrentRefreshCmd; ///< currently running cmd to refresh nodes qualities
         int           fRefreshCnt{0};   ///< currently running refresh command
         int           fRefreshReplies{0}; ///< number of replies for current command

         int           fCtrlId{0};     ///< counter for control requests
         dabc::TimeStamp fCtrlTm;   ///< time when last control count was send
         dabc::TimeStamp fNewRunTm;   ///< time when last control count was send
         int           fCtrlCnt{0};    ///< how many control replies are awaited
         bool          fCtrlError{false};  ///< if there are error during current communication loop
         int           fCtrlErrorCnt{0}; ///< number of consequent control errors
         double        fCtrlStateQuality{0};  ///< <0.3 error, <0.7 warning, more is ok
         std::string   fCtrlStateName; ///< current name
         int           fCtrlInpNodesCnt{0}; ///< count of recognized input nodes
         int           fCtrlInpNodesExpect{0}; ///< count of expected input nodes
         int           fCtrlBldNodesCnt{0}; ///< count of recognized builder nodes
         int           fCtrlBldNodesExpect{0}; ///< count of expected builder nodes
         int           fCtrlSzLimit{0}; ///< 0 - do nothing, 1 - start checking (after start run), 2 - exced
         double        fCtrlData{0};    ///< accumulated data rate
         double        fCtrlEvents{0};   ///< accumulated events rate
         double        fCtrlLost{0};     ///< accumulated lost rate
         uint64_t      fCurrentLost{0};   ///< current value
         uint64_t      fCurrentEvents{0}; ///< current value
         uint64_t      fCurrentData{0}; ///< current value
         uint64_t      fTotalLost{0};    ///< last value
         uint64_t      fTotalEvents{0};  ///< last value
         uint64_t      fTotalData{0};  ///< last value
         dabc::TimeStamp fLastRateTm;   ///< last time ratemeter was updated
         unsigned      fCtrlRunId{0};    ///< received run id from builders
         std::string   fCtrlRunPrefix; ///< received run prefix from builders
         std::vector<std::string> fLastBuilders; ///< last list of builder nodes
         int           fSameBuildersCnt{0}; ///< how many time same number of inputs was detected
         dabc::Command fInitRunCmd;   ///< command used to start run at very beginning, uses delay technique

         bool ReplyCommand(dabc::Command cmd) override;

         void AddItem(std::vector<std::string> &items, std::vector<std::string> &nodes, const std::string &item, const std::string &node);

         void PreserveLastCalibr(bool do_write = false, double quality = 1., unsigned runid = 0, bool set_time = false);

      public:

         BnetMasterModule(const std::string &name, dabc::Command cmd = nullptr);

         void BeforeModuleStart() override;

         void ProcessTimerEvent(unsigned timer) override;

         int ExecuteCommand(dabc::Command cmd) override;

   };
}


#endif
