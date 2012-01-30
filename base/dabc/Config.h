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

#ifndef DABC_Config
#define DABC_Config

#ifndef DABC_Record
#include "dabc/Record.h"
#endif

#ifndef DABC_Command
#include "dabc/Command.h"
#endif


namespace dabc {

   class Config;
   class Worker;
   class WorkerRef;

   class ConfigContainer : public RecordContainer {
      friend class Config;
      friend class Worker;
      friend class WorkerRef;

      protected:
         std::string   fName;     //!< name of config parameter

         Command       fCmd;      //!< value from the command

         Record        fPar;      //!< reference on parameter (if exists) of specified config name

         Reference     fWorker;   //!<  reference on the worker where config requested

         int           fReadFlag; //!< -1 no read, 0 - reading from config, 1 - read done

         ConfigContainer(const std::string& name, Command cmd, Reference worker);

         virtual bool SetField(const std::string& name, const char* value, const char* kind);
         virtual const char* GetField(const std::string& name, const char* dflt = 0);

      public:
   };


   class Config : public Record {

      DABC_REFERENCE(Config, Record, ConfigContainer)

   };

}


#endif
