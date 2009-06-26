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

#ifndef ROC_Commands
#define ROC_Commands


#ifndef DABC_Command
#include "dabc/Command.h"
#endif

#include <stdint.h>

namespace roc {

   extern const char* AddrPar;
   extern const char* ValuePar;
   extern const char* ErrNoPar;
   extern const char* TmoutPar;
   extern const char* RawDataPar;

   class CmdPut : public dabc::Command {
     public:

        static const char* CmdName() { return "CmdPut"; }

        CmdPut(uint32_t address, uint32_t value, double tmout = 5.) :
            dabc::Command(CmdName())
        {
           SetUInt(AddrPar, address);
           SetUInt(ValuePar, value);
           SetDouble(TmoutPar, tmout);
        }
   };

  class CmdGet : public dabc::Command {
     public:

        static const char* CmdName() { return "CmdGet"; }

        CmdGet(uint32_t address, double tmout = 5.) :
            dabc::Command(CmdName())
        {
           SetUInt(AddrPar, address);
           SetDouble(TmoutPar, tmout);
        }
   };

}

#endif

