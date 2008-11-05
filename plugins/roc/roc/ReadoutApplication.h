#ifndef ROC_ReadoutApplication
#define ROC_ReadoutApplication

#include "dabc/Application.h"
#include "dabc/Basic.h"
#include "dabc/threads.h"
#include "dabc/MemoryPool.h"
#include "dabc/Manager.h"
#include "dabc/Module.h"

#include "roc/RocFactory.h"

#define DABC_ROC_PAR_DATASERVER      "DataServerKind"
#define DABC_ROC_PAR_ROCIP           "RocIp"
#define DABC_ROC_PAR_OUTFILE         "OutFile"
#define DABC_ROC_PAR_OUTFILELIMIT    "OutFileLimit"
#define DABC_ROC_PAR_DOCALIBR        "DoCalibr"
#define DABC_ROC_PAR_CALIBRFILE      "CalibrFile"
#define DABC_ROC_PAR_CALIBRFILELIMIT "CalibrFileLimit"

namespace roc {

   class ReadoutApplication : public dabc::Application {
      public:
         ReadoutApplication(const char* name);

         virtual bool IsModulesRunning();

         /** Number of ROCs connected to ReadoutModule*/
         int   NumRocs() const { return GetParInt(DABC_ROC_COMPAR_ROCSNUMBER, 1); }

         /** IP address for ROC of index*/
         const char* RocIp(int index = 0) const;

         /** id number of Mbs server (stream, transport)*/
         int DataServerKind() const;

         /** Total size of dabc buffer containing served Mbs events*/
         int BufferSize() const { return GetParInt(DABC_ROC_COMPAR_BUFSIZE, 8192);}

         int TransWindow() const { return GetParInt(DABC_ROC_COMPAR_TRANSWINDOW, 30);}

         /** Number of buffers each input/output port of readout module*/
         int   PortQueueLength() const {return GetParInt(DABC_ROC_COMPAR_QLENGTH,10);}

            /** Number of buffers each input/output port of readout module*/
         int   NumPoolBuffers() const {return GetParInt(DABC_ROC_COMPAR_POOL_SIZE,50);}

         dabc::String OutputFileName() const { return GetParStr(DABC_ROC_PAR_OUTFILE, ""); }
         int OutFileLimit() const { return GetParInt(DABC_ROC_PAR_OUTFILELIMIT, 0); }
         bool DoTaking() const { return GetParInt(DABC_ROC_PAR_DOCALIBR, 0) < 2; }
         bool DoCalibr() const { return GetParInt(DABC_ROC_PAR_DOCALIBR, 0) > 0; }
         dabc::String CalibrFileName() const { return GetParStr(DABC_ROC_PAR_CALIBRFILE, ""); }
         int CalibrFileLimit() const { return GetParInt(DABC_ROC_PAR_CALIBRFILELIMIT, 0); }

         virtual bool CreateAppModules();

         virtual int SMCommandTimeout() const { return 20; }


      protected:

        /** Send configuration to connected roc*/
        bool ConfigureRoc(int index=0);

        bool WriteRocRegister(int rocid, int registr, int value);

        /** current full name of roc device (with leading "Device/") */
        dabc::String fFullDeviceName;
   };
}

#endif
