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
#ifndef ROC_ReadoutApplication
#define ROC_ReadoutApplication

#include "dabc/Application.h"
#include "dabc/Basic.h"
#include "dabc/threads.h"
#include "dabc/MemoryPool.h"
#include "dabc/Manager.h"
#include "dabc/Module.h"

#include "roc/Factory.h"
#include "roc/Device.h"

namespace roc {

   extern const char* xmlDoCalibr;
   extern const char* xmlRocIp;
   extern const char* xmlRawFile;
   extern const char* xmlCalibrFile;
   extern const char* xmlReadoutAppClass;

   class ReadoutApplication : public dabc::Application {
      protected:
         std::string  fDevName;
         roc::Device* GetBoardDevice(int indx = 0);
      public:
         ReadoutApplication();

         virtual bool IsModulesRunning();

         bool DoTaking() const { return GetParInt(xmlDoCalibr, 0) < 2; }
         bool DoCalibr() const { return GetParInt(xmlDoCalibr, 0) > 0; }

         /** Number of ROCs connected to ReadoutModule*/
         int   NumRocs() const { return GetParInt(roc::xmlNumRocs, 1); }

         /** IP address for ROC of index*/
         std::string RocIp(int index = 0) const;

         /** id number of Mbs server (stream, transport)*/
         int DataServerKind() const;

         std::string OutputFileName() const { return GetParStr(xmlRawFile, ""); }
         std::string CalibrFileName() const { return GetParStr(xmlCalibrFile, ""); }

         virtual bool CreateAppModules();

         virtual int SMCommandTimeout() const { return 20; }


      protected:

        /** Send configuration to connected roc*/
        bool ConfigureRoc(int index=0);
   };
}

#endif
