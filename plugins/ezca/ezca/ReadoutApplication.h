
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
         std::string DataServerKind() const;

         std::string OutputFileName() const { return Par(mbs::xmlFileName).AsStdStr(); }

         /** Number of longword records to readout */
         int   NumLongRecs() const { return Par(ezca::xmlNumLongRecords).AsInt(0); }

         /** Number of double records to readout */
         int   NumDoubleRecs() const { return Par(ezca::xmlNumDoubleRecords).AsInt(0); }

         virtual bool CreateAppModules();

         virtual int SMCommandTimeout() const { return 20; }

         virtual int ExecuteCommand(dabc::Command cmd);

      protected:

   };
}

#endif
