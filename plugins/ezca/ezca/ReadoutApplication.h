
#ifndef EZCA_READOUTAPPLICATION_H
#define EZCA_READOUTAPPLICATION_H

#include "dabc/Application.h"
#include "mbs/MbsTypeDefs.h"
#include "ezca/Definitions.h"


namespace ezca {






   class ReadoutApplication : public dabc::Application {
      public:
         ReadoutApplication();


         /** id number of Mbs server (stream, transport)*/
         int DataServerKind() const;

         std::string OutputFileName() const { return GetParStr(mbs::xmlFileName, ""); }

         /** Number of longword records to readout */
         int   NumLongRecs() const { return GetParInt(ezca::xmlNumLongRecords, 0); }

         /** Number of double records to readout */
         int   NumDoubleRecs() const { return GetParInt(ezca::xmlNumDoubleRecords, 0); }


         virtual bool CreateAppModules();

         virtual int SMCommandTimeout() const { return 20; }

         virtual int ExecuteCommand(dabc::Command* cmd);

      protected:

   };
}

#endif
