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
/*
 * DimcCommands.h
 *
 *  Created on: 25.11.2008
 *      Author: J.Adamczewski-Musch
 */

#ifndef DIMCCOMMANDS_H_
#define DIMCCOMMANDS_H_

#include "dabc/Command.h"

#define DIMC_COMPAR_PAR      "Parameter"

#define _DIMC_COMMAND_SETPAR_ "SetParameter"
#define _DIMC_COMMAND_SETDIMPAR_ "SetDimParameter"
#define _DIMC_COMMAND_SHUTDOWN_ "Shutdown"
#define _DIMC_COMMAND_MANAGER_ "ManagerCommand"

#define _DIMC_SERVICE_MESSAGE_      "statusMessage"
#define _DIMC_SERVICE_FSMRECORD_    "RunStatus"
#define _DIMC_SERVICE_INFORECORD_   "InfoMessage"



namespace dimc{

/*
 * General wrapper for all incoming DIM commands
 */
class Command : public dabc::Command {
      public:
         Command(const char* name, const char* par) :
            dabc::Command(name)
            {
               SetStr(DIMC_COMPAR_PAR, par);
            }

         const char* DimPar()
            {
               return GetStr(DIMC_COMPAR_PAR,0);
            }
   };




}

#endif /* DIMCCOMMANDS_H_ */
