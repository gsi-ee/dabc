#ifndef ROC_ReadoutApplication
#define ROC_ReadoutApplication

#include "dabc/Application.h"
#include "dabc/Basic.h"
#include "dabc/threads.h"
#include "dabc/MemoryPool.h"
#include "dabc/Manager.h"
#include "dabc/Module.h"

#include "roc/RocFactory.h"
#include "roc/RocDevice.h"

namespace roc {

   extern const char* xmlDoCalibr;
   extern const char* xmlRocIp;
   extern const char* xmlMbsServerKind;
   extern const char* xmlRawFile;
   extern const char* xmlRawFileLimit;
   extern const char* xmlCalibrFile;
   extern const char* xmlCalibrFileLimit;

   class ReadoutApplication : public dabc::Application {
      public:
         ReadoutApplication(const char* name);

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
         int OutFileLimit() const { return GetParInt(xmlRawFileLimit, 0); }
         std::string CalibrFileName() const { return GetParStr(xmlCalibrFile, ""); }
         int CalibrFileLimit() const { return GetParInt(xmlCalibrFileLimit, 0); }

         virtual bool CreateAppModules();

         virtual int SMCommandTimeout() const { return 20; }


      protected:

        /** Send configuration to connected roc*/
        bool ConfigureRoc(int index=0);

        bool WriteRocRegister(int rocid, int registr, int value);

        /** current full name of roc device (with leading "Device/") */
        std::string fFullDeviceName;
   };
}

#endif
