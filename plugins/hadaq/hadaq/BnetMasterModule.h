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

         bool          fControl;  ///< when true, master actively controls BNET nodes and switches to new RUNs
         unsigned      fMaxRunSize; ///< maximal run size in MB
         dabc::Command fCurrentFileCmd; ///< currently running cmd to switch files
         int           fCmdCnt;     ///< just counter to avoid mismatch
         int           fCmdReplies; ///< number of replies for current command
         int           fCtrlId;     ///< counter for control requests
         dabc::TimeStamp fCtrlTm;   ///< time when last control count was send
         int           fCtrlCnt;    ///< how many control replies are awaited
         double        fCtrlStateQuality;  ///< <0.3 error, <0.7 warning, more is ok
         std::string   fCtrlStateName; ///< current name
         int           fCtrlInpNodesCnt; ///< count of recognized input nodes
         int           fCtrlInpNodesExpect; ///< count of expected input nodes
         int           fCtrlBldNodesCnt; ///< count of recognized builder nodes
         int           fCtrlBldNodesExpect; ///< count of expected builder nodes
         bool          fCtrlSzLimit; ///< when true, size limit was exceed
         double        fCtrlData;    ///< accumulated data rate
         double        fCtrlEvents;   ///< accumulated events rate
         double        fCtrlLost;     ///< accumulated lost rate
         unsigned      fCtrlRunId;    ///< received run id from builders
         std::string   fCtrlRunPrefix; ///< received run prefix from builders
         std::vector<std::string> fLastBuilders; ///< last list of builder nodes
         int           fSameBuildersCnt; ///< how many time same number of inputs was detected
         dabc::Command fInitRunCmd;   ///< command used to start run at very beginning, uses delay technique

         virtual bool ReplyCommand(dabc::Command cmd);

         void AddItem(std::vector<std::string> &items, std::vector<std::string> &nodes, const std::string &item, const std::string &node);

         void PreserveLastCalibr(bool do_write = false, double quality = 1.);

      public:

         BnetMasterModule(const std::string &name, dabc::Command cmd = nullptr);

         virtual void BeforeModuleStart();

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);

   };
}


#endif
